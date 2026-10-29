#include "Oversampler.h"

#include <cstring>

namespace wg::dsp::os {
namespace {

template<std::size_t FullN, std::size_t HalfN>
std::vector<double> expandCoeffs(const std::array<double, HalfN>& half)
{
    // half = { h(M), h(M+1), h(M+3), ... }
    std::vector<double> h(FullN, 0.0);
    const int mid = static_cast<int>(FullN - 1) / 2;
    h[static_cast<std::size_t>(mid)] = half[0];
    int idx = 1;
    for (int off = 1; mid + off < static_cast<int>(FullN); off += 2)
    {
        const double v = half[static_cast<std::size_t>(idx++)];
        h[static_cast<std::size_t>(mid + off)] = v;
        h[static_cast<std::size_t>(mid - off)] = v;
    }
    return h;
}

} // namespace

void Oversampler::buildStages(int numStages)
{
    stages_.clear();
    stages_.reserve(static_cast<std::size_t>(numStages));
    for (int s = 0; s < numStages; ++s)
    {
        Stage st;
        if (s == 0)
            st.coeffs = expandCoeffs<kHb1FullLength, kHb1Length>(kHb1Coeffs);
        else if (s == 1)
            st.coeffs = expandCoeffs<kHb2FullLength, kHb2Length>(kHb2Coeffs);
        else
            st.coeffs = expandCoeffs<kHb3FullLength, kHb3Length>(kHb3Coeffs);
        st.length = static_cast<int>(st.coeffs.size());
        st.stateLen = st.length - 1;
        st.zUp.assign(static_cast<std::size_t>(st.stateLen * numChannels_), 0.0);
        st.zDown.assign(static_cast<std::size_t>(st.stateLen * numChannels_), 0.0);
        stages_.push_back(std::move(st));
    }
}

void Oversampler::prepare(double sampleRate, int maxBlock, int osFactorChoice, int numChannels)
{
    sampleRate_ = sampleRate;
    maxBlock_ = std::max(1, maxBlock);
    numChannels_ = std::max(1, std::min(2, numChannels));
    factor_ = effectiveOsFactor(sampleRate, osFactorChoice);

    int numStages = 0;
    for (int f = factor_; f > 1; f /= 2)
        ++numStages;
    buildStages(numStages);

    // Up+down each: (N-1)/2 at high rate → base (N-1)/2^(s+1). Measured peak is often −1.
    double latencyExact = 0.0;
    for (int s = 0; s < numStages; ++s)
    {
        const double N = static_cast<double>(stages_[static_cast<std::size_t>(s)].length);
        latencyExact += (N - 1.0) / static_cast<double>(1 << (s + 1));
    }
    // Exact group delay in base samples; report nearest int matching impulse peak.
    latencySamples_ = static_cast<int>(std::lround(latencyExact));

    const int maxHigh = maxBlock_ * std::max(factor_, 1) + 64;
    scratch0_.assign(static_cast<std::size_t>(maxHigh), 0.0f);
    scratch1_.assign(static_cast<std::size_t>(maxHigh), 0.0f);
    reset();
}

void Oversampler::reset() noexcept
{
    for (auto& st : stages_)
    {
        std::fill(st.zUp.begin(), st.zUp.end(), 0.0);
        std::fill(st.zDown.begin(), st.zDown.end(), 0.0);
    }
}

void Oversampler::upsampleStage(Stage& st, int channel, const float* in, float* out, int nIn) noexcept
{
    double* z = st.zUp.data() + static_cast<std::size_t>(channel * st.stateLen);
    const double* h = st.coeffs.data();
    int outIdx = 0;
    for (int n = 0; n < nIn; ++n)
    {
        for (int phase = 0; phase < 2; ++phase)
        {
            const double x = (phase == 0) ? (2.0 * static_cast<double>(in[n])) : 0.0;
            double acc = h[0] * x;
            for (int k = 0; k < st.stateLen; ++k)
                acc += h[k + 1] * z[k];
            out[outIdx++] = static_cast<float>(acc);
            for (int k = st.stateLen - 1; k > 0; --k)
                z[k] = z[k - 1];
            if (st.stateLen > 0)
                z[0] = x;
        }
    }
}

void Oversampler::downsampleStage(Stage& st, int channel, const float* in, float* out, int nOut) noexcept
{
    double* z = st.zDown.data() + static_cast<std::size_t>(channel * st.stateLen);
    const double* h = st.coeffs.data();
    for (int n = 0; n < nOut; ++n)
    {
        double yKeep = 0.0;
        for (int phase = 0; phase < 2; ++phase)
        {
            const double x = static_cast<double>(in[2 * n + phase]);
            double acc = h[0] * x;
            for (int k = 0; k < st.stateLen; ++k)
                acc += h[k + 1] * z[k];
            for (int k = st.stateLen - 1; k > 0; --k)
                z[k] = z[k - 1];
            if (st.stateLen > 0)
                z[0] = x;
            if (phase == 0)
                yKeep = acc;
        }
        out[n] = static_cast<float>(yKeep);
    }
}

void Oversampler::upsample(int channel, const float* in, float* out, int numSamples) noexcept
{
    if (stages_.empty())
    {
        std::memcpy(out, in, static_cast<std::size_t>(numSamples) * sizeof(float));
        return;
    }
    int n = numSamples;
    const float* src = in;
    for (std::size_t s = 0; s < stages_.size(); ++s)
    {
        const bool last = (s + 1u == stages_.size());
        float* dest = last ? out : ((s % 2u == 0u) ? scratch0_.data() : scratch1_.data());
        upsampleStage(stages_[s], channel, src, dest, n);
        n *= 2;
        src = dest;
    }
}

void Oversampler::downsample(int channel, const float* in, float* out, int numSamples) noexcept
{
    if (stages_.empty())
    {
        std::memcpy(out, in, static_cast<std::size_t>(numSamples) * sizeof(float));
        return;
    }
    int nHigh = numSamples * factor_;
    const float* src = in;
    for (int si = static_cast<int>(stages_.size()) - 1; si >= 0; --si)
    {
        const bool last = (si == 0);
        const int nOut = nHigh / 2;
        const int depth = static_cast<int>(stages_.size()) - 1 - si;
        float* dest = last ? out : ((depth % 2 == 0) ? scratch0_.data() : scratch1_.data());
        downsampleStage(stages_[static_cast<std::size_t>(si)], channel, src, dest, nOut);
        nHigh = nOut;
        src = dest;
    }
}

} // namespace wg::dsp::os
