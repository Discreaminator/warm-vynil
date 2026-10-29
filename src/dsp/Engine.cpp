#include "Engine.h"

#include "Constants.h"
#include "util/MathUtil.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace wg::dsp {

void Engine::prepare(const Spec& spec) noexcept
{
    spec_ = spec;
    const int ch = std::max(1, spec.numChannels);
    const int maxBlock = std::max(1, spec.maxBlock);

    oversampler_.prepare(spec.sampleRate, maxBlock, targets_.osFactor, ch);
    latencySamples_ = oversampler_.latencySamples();

    amountSm_.setTimeMs(spec.sampleRate, kSmoothAmountMs);
    inGainSm_.setTimeMs(spec.sampleRate, kSmoothGainMs);
    outGainSm_.setTimeMs(spec.sampleRate, kSmoothGainMs);
    mixSm_.setTimeMs(spec.sampleRate, kSmoothGainMs);
    wetEnableSm_.setTimeMs(spec.sampleRate, kSmoothBypassMs);
    hostBypassSm_.setTimeMs(spec.sampleRate, kSmoothBypassMs);

    amountSm_.reset(targets_.amount);
    inGainSm_.reset(targets_.inGainDb);
    outGainSm_.reset(targets_.outGainDb);
    mixSm_.reset(targets_.mix);
    hostBypassSm_.reset(targets_.bypass ? 1.0 : 0.0);
    // Snap engine-bypass wet weight to match Amount (avoid T2 tail from 20 ms ramp).
    engineWetActive_ = !(static_cast<double>(targets_.amount) < kEngineBypassOff);
    wetEnableSm_.reset(engineWetActive_ ? 1.0 : 0.0);

    for (int c = 0; c < 2; ++c)
    {
        dcIn_[c].setSampleRate(spec.sampleRate);
        dcOut_[c].setSampleRate(spec.sampleRate);
    }

    monoBass_.prepare(spec.sampleRate);

    const double fsi = oversampler_.internalRate();
    for (int c = 0; c < 2; ++c)
    {
        riaa_[static_cast<std::size_t>(c)].prepare(fsi);
        cartridge_[static_cast<std::size_t>(c)].prepare(fsi);
        tube_[static_cast<std::size_t>(c)].prepare(fsi);
        xfmr_[static_cast<std::size_t>(c)].prepare(fsi);
    }
    crosstalk_.prepare(fsi);
    autoGain_.prepare(spec.sampleRate);
    (void)ch;

    dryDelayL_.prepare(static_cast<std::size_t>(std::max(latencySamples_, 1) + 8));
    dryDelayR_.prepare(static_cast<std::size_t>(std::max(latencySamples_, 1) + 8));

    const int maxOs = maxBlock * std::max(oversampler_.factor(), 1);
    osL_.assign(static_cast<std::size_t>(maxOs), 0.0f);
    osR_.assign(static_cast<std::size_t>(maxOs), 0.0f);
    dryL_.assign(static_cast<std::size_t>(maxBlock), 0.0f);
    dryR_.assign(static_cast<std::size_t>(maxBlock), 0.0f);

    sampleCounter_ = 0;
    reset();
    updateSmoothersAndCoeffs();
}

void Engine::reset() noexcept
{
    sampleCounter_ = 0;
    for (int c = 0; c < 2; ++c)
    {
        dcIn_[c].reset();
        dcOut_[c].reset();
        riaa_[static_cast<std::size_t>(c)].reset();
        tracing_[static_cast<std::size_t>(c)].reset();
        cartridge_[static_cast<std::size_t>(c)].reset();
        tube_[static_cast<std::size_t>(c)].reset();
        xfmr_[static_cast<std::size_t>(c)].reset();
    }
    monoBass_.reset();
    crosstalk_.reset();
    oversampler_.reset();
    autoGain_.reset();
    dryDelayL_.reset();
    dryDelayR_.reset();
    engineWetActive_ = true;
}

void Engine::setTargets(const EngineParams& params) noexcept
{
    // OS / topology changes must go through prepare() on a non-RT thread (M2+).
    // Storing osFactor here only; Oversampler keeps the last prepared factor until prepare().
    targets_ = params;
    amountSm_.setTarget(static_cast<double>(params.amount));
    inGainSm_.setTargetDb(static_cast<double>(params.inGainDb));
    outGainSm_.setTargetDb(static_cast<double>(params.outGainDb));
    mixSm_.setTarget(static_cast<double>(params.mix));
    hostBypassSm_.setTarget(params.bypass ? 1.0 : 0.0);
    autoGain_.setMode(params.autoGain);
}

void Engine::updateSmoothersAndCoeffs() noexcept
{
    const double a = amountSm_.current();
    if (engineWetActive_)
    {
        if (a < kEngineBypassOff)
            engineWetActive_ = false;
    }
    else if (a > kEngineBypassOn)
    {
        engineWetActive_ = true;
    }
    wetEnableSm_.setTarget(engineWetActive_ ? 1.0 : 0.0);

    internal_ = map(targets_, a, oversampler_.internalRate());

    MonoBassTargets mb{};
    mb.monoFreqHz = targets_.monoFreq;
    mb.depthM = internal_.m;
    monoBass_.setTargets(mb);

    for (int c = 0; c < 2; ++c)
    {
        riaa_[static_cast<std::size_t>(c)].setTargets(internal_.tiltDb, internal_.T4_eff);
        tracing_[static_cast<std::size_t>(c)].setTargets(internal_.s_tr);
        cartridge_[static_cast<std::size_t>(c)].setTargets(internal_.cartFr, internal_.cartQ);

        TubeTargets tt{};
        tt.driveDb = internal_.tubeDriveDb;
        tt.tubeBias = static_cast<double>(targets_.tubeBias);
        tt.kDyn = internal_.kDyn;
        tt.model = targets_.tubeModel;
        tube_[static_cast<std::size_t>(c)].setTargets(tt);
        xfmr_[static_cast<std::size_t>(c)].setTargets(internal_.xfmrSx, internal_.xfmrFc);
    }
    crosstalk_.setTargets(internal_.crosstalkDb);
    autoGain_.setAmount(a);
}

void Engine::processOsChain(float* leftOs, float* rightOs, int numOs, int numChannels) noexcept
{
    for (int i = 0; i < numOs; ++i)
    {
        for (int c = 0; c < numChannels; ++c)
        {
            float* buf = (c == 0) ? leftOs : rightOs;
            double x = static_cast<double>(buf[i]);
            x = riaa_[static_cast<std::size_t>(c)].processPreSample(x);
            x = riaa_[static_cast<std::size_t>(c)].processNeumannSample(x);
            x = tracing_[static_cast<std::size_t>(c)].processSample(x);
            x = riaa_[static_cast<std::size_t>(c)].processPostChainSample(x);
            x = cartridge_[static_cast<std::size_t>(c)].processSample(x);
            buf[i] = static_cast<float>(x);
        }

        if (numChannels > 1)
        {
            double L = static_cast<double>(leftOs[i]);
            double R = static_cast<double>(rightOs[i]);
            crosstalk_.processStereo(L, R);
            leftOs[i] = static_cast<float>(L);
            rightOs[i] = static_cast<float>(R);
        }

        for (int c = 0; c < numChannels; ++c)
        {
            float* buf = (c == 0) ? leftOs : rightOs;
            double x = static_cast<double>(buf[i]);
            x = tube_[static_cast<std::size_t>(c)].processSample(x);
            x = xfmr_[static_cast<std::size_t>(c)].processSample(x);
            buf[i] = static_cast<float>(x);
        }
    }
}

void Engine::processChunk(float* const* /*channels*/, int /*numChannels*/, int /*numSamples*/) noexcept {}

void Engine::process(float* const* channels, int numChannels, int numSamples) noexcept
{
    ScopedFlushDenormals guard;
    if (channels == nullptr || numSamples <= 0 || numChannels <= 0)
        return;

    numChannels = std::min(numChannels, 2);

    bool bad = false;
    for (int c = 0; c < numChannels && !bad; ++c)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            if (!std::isfinite(channels[c][n]))
            {
                bad = true;
                break;
            }
        }
    }
    if (bad)
    {
        reset();
        for (int c = 0; c < numChannels; ++c)
            std::memset(channels[c], 0, static_cast<std::size_t>(numSamples) * sizeof(float));
        return;
    }

    // Flush pathological subnormals at input (SPEC §8.4 / T9)
    for (int c = 0; c < numChannels; ++c)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            float& v = channels[c][n];
            if (v != 0.0f && std::abs(v) < 1.0e-20f)
                v = 0.0f;
        }
    }

    // Chunk if host exceeds prepared maxBlock
    int remaining = numSamples;
    int base = 0;
    while (remaining > 0)
    {
        const int block = std::min(remaining, std::max(1, spec_.maxBlock));
        float* blockCh[2] = {
            channels[0] + base,
            (numChannels > 1) ? channels[1] + base : nullptr,
        };

        int offset = 0;
        while (offset < block)
        {
            const int gridPos = static_cast<int>(sampleCounter_ % static_cast<std::uint64_t>(kCoeffUpdateGrid));
            const int untilGrid = (gridPos == 0) ? kCoeffUpdateGrid : (kCoeffUpdateGrid - gridPos);
            const int n = std::min(untilGrid, block - offset);

            if (gridPos == 0)
                updateSmoothersAndCoeffs();

            float* chPtr[2] = {
                blockCh[0] + offset,
                (numChannels > 1 && blockCh[1] != nullptr) ? blockCh[1] + offset : nullptr,
            };

            const float delay = static_cast<float>(latencySamples_);
            const int osFactor = oversampler_.factor();
            const int numOs = n * osFactor;

            for (int i = 0; i < n; ++i)
            {
                dryL_[static_cast<std::size_t>(i)] = chPtr[0][i];
                if (numChannels > 1 && chPtr[1] != nullptr)
                    dryR_[static_cast<std::size_t>(i)] = chPtr[1][i];
            }

            for (int i = 0; i < n; ++i)
            {
                const double inG = inGainSm_.processSample();
                for (int c = 0; c < numChannels; ++c)
                {
                    if (chPtr[c] == nullptr)
                        continue;
                    double x = static_cast<double>(chPtr[c][i]) * inG;
                    x = dcIn_[c].processSample(x);
                    chPtr[c][i] = static_cast<float>(x);
                }
            }

            if (numChannels > 1 && chPtr[1] != nullptr)
                monoBass_.processBlock(chPtr[0], chPtr[1], n);

            oversampler_.upsample(0, chPtr[0], osL_.data(), n);
            if (numChannels > 1 && chPtr[1] != nullptr)
                oversampler_.upsample(1, chPtr[1], osR_.data(), n);

            processOsChain(osL_.data(), osR_.data(), numOs, numChannels);

            oversampler_.downsample(0, osL_.data(), chPtr[0], n);
            if (numChannels > 1 && chPtr[1] != nullptr)
                oversampler_.downsample(1, osR_.data(), chPtr[1], n);

            for (int i = 0; i < n; ++i)
            {
                const double mixW = mixSm_.processSample();
                const double wetEn = wetEnableSm_.processSample();
                const double bypass = hostBypassSm_.processSample();
                const double outG = outGainSm_.processSample();
                (void)amountSm_.processSample();
                const double wetMix = clamp(mixW * wetEn, 0.0, 1.0);

                for (int c = 0; c < numChannels; ++c)
                {
                    if (chPtr[c] == nullptr)
                        continue;

                    double wet = dcOut_[c].processSample(static_cast<double>(chPtr[c][i]));
                    const double dryIn = (c == 0) ? static_cast<double>(dryL_[static_cast<std::size_t>(i)])
                                                 : static_cast<double>(dryR_[static_cast<std::size_t>(i)]);
                    DelayLine& dl = (c == 0) ? dryDelayL_ : dryDelayR_;
                    const double dry = (latencySamples_ > 0)
                                           ? static_cast<double>(dl.processSample(static_cast<float>(dryIn), delay))
                                           : dryIn;

                    if (c == 0)
                        autoGainLin_ = autoGain_.processGain(dryIn, wet);
                    wet *= autoGainLin_;

                    double y = (1.0 - wetMix) * dry + wetMix * wet;
                    y = (1.0 - bypass) * y + bypass * dry;
                    y *= outG;
                    chPtr[c][i] = static_cast<float>(y);
                }
            }

            sampleCounter_ += static_cast<std::uint64_t>(n);
            offset += n;
        }

        base += block;
        remaining -= block;
    }
}

} // namespace wg::dsp
