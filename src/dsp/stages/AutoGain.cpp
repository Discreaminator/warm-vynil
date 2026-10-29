#include "AutoGain.h"

#include "dsp/Constants.h"
#include "dsp/util/MathUtil.h"

#include <array>
#include <cmath>

namespace wg::dsp {

namespace {

// Placeholder calibration (SPEC §4.7) — replace via wg_render --calibrate.
constexpr std::array<double, 11> kStaticGainDbTable{
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
};

[[nodiscard]] double lufsFromMeanSquare(double ms) noexcept
{
    if (ms <= 1.0e-20)
        return -100.0;
    return -0.691 + 10.0 * std::log10(ms);
}

} // namespace

void AutoGain::prepare(double sampleRate, int numChannels) noexcept
{
    fs_ = sampleRate;
    numChannels_ = numChannels;
    kIn_.setSampleRate(fs_);
    kOut_.setSampleRate(fs_);
    if (fs_ > 0.0)
    {
        const double msTau = fs_ * kAutoGainDynamicTauMs * 0.001;
        msCoeff_ = 1.0 - std::exp(-1.0 / msTau);
        const double gTau = fs_ * kAutoGainSmoothTauMs * 0.001;
        gainSmoothCoeff_ = 1.0 - std::exp(-1.0 / gTau);
    }
}

void AutoGain::reset() noexcept
{
    gainLinear_ = 1.0;
    gainDbSmooth_ = 0.0;
    inMs_ = 0.0;
    outMs_ = 0.0;
    gateFrozen_ = false;
    kIn_.reset();
    kOut_.reset();
}

double AutoGain::staticGainDb() const noexcept
{
    const double a = clamp(amount_, 0.0, 1.0);
    const double idx = a * 10.0;
    const int i0 = static_cast<int>(std::floor(idx));
    const int i1 = std::min(i0 + 1, 10);
    const double frac = idx - static_cast<double>(i0);
    return lerp(kStaticGainDbTable[static_cast<std::size_t>(i0)],
                kStaticGainDbTable[static_cast<std::size_t>(i1)], frac);
}

void AutoGain::updateGate(double inWeightedMs) noexcept
{
    const double lufs = lufsFromMeanSquare(inWeightedMs);
    gateFrozen_ = lufs < kAutoGainGateLufs;
}

double AutoGain::processSample(double inSample, double wetSample) noexcept
{
    (void)numChannels_;

    if (mode_ == AutoGainMode::Off)
    {
        gainLinear_ = 1.0;
        return gainLinear_;
    }

    if (mode_ == AutoGainMode::Static)
    {
        gainLinear_ = dB2lin(staticGainDb());
        return gainLinear_;
    }

    const double inW = kIn_.processSample(inSample);
    const double outW = kOut_.processSample(wetSample);
    inMs_ += (inW * inW - inMs_) * msCoeff_;
    outMs_ += (outW * outW - outMs_) * msCoeff_;
    updateGate(inMs_);

    double targetDb = 0.0;
    if (!gateFrozen_ && outMs_ > 1.0e-20)
    {
        const double g = std::sqrt(inMs_ / outMs_);
        targetDb = clamp(lin2dB(g), -12.0, 12.0);
    }

    if (!gateFrozen_)
        gainDbSmooth_ += (targetDb - gainDbSmooth_) * gainSmoothCoeff_;

    gainLinear_ = dB2lin(gainDbSmooth_);
    return gainLinear_;
}

} // namespace wg::dsp
