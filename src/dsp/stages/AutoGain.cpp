#include "AutoGain.h"

#include "dsp/Constants.h"
#include "dsp/util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

namespace {
// Calibrated by wg_render --calibrate (pink −18 LUFS, 48 kHz, OS 4x, 2026-10-29 M4).
constexpr double kStaticGainDb[AutoGain::kStaticTableSize] = {
    -0.000071, 0.941383, 0.935443, 0.929491, 0.854458, 0.854939,
    0.871393, 0.888314, 0.905754, 0.923757, 0.942371
};
} // namespace

const double* AutoGain::staticTableDb() noexcept
{
    return kStaticGainDb;
}

void AutoGain::prepare(double sampleRate) noexcept
{
    sampleRate_ = sampleRate;
    kIn_.setSampleRate(sampleRate);
    kOut_.setSampleRate(sampleRate);
    msCoeff_ = 1.0 - std::exp(-1.0 / (kAutoGainDynamicTauMs * 0.001 * sampleRate));
    smoothCoeff_ = 1.0 - std::exp(-1.0 / (kAutoGainSmoothTauMs * 0.001 * sampleRate));
    reset();
}

void AutoGain::reset() noexcept
{
    kIn_.reset();
    kOut_.reset();
    pin_ = 1.0e-12;
    pout_ = 1.0e-12;
    dynGainDb_ = 0.0;
}

double AutoGain::staticGainLinear(double amount01) const noexcept
{
    const double a = clamp(amount01, 0.0, 1.0);
    const double pos = a * static_cast<double>(kStaticTableSize - 1);
    const auto i0 = static_cast<int>(pos);
    const int i1 = std::min(i0 + 1, kStaticTableSize - 1);
    const double f = pos - static_cast<double>(i0);
    const double db = lerp(kStaticGainDb[i0], kStaticGainDb[i1], f);
    return dB2lin(db);
}

double AutoGain::processGain(double dryPreInGain, double wet) noexcept
{
    if (mode_ == AutoGainMode::Off)
        return 1.0;

    if (mode_ == AutoGainMode::Static)
        return staticGainLinear(amount_);

    const double kin = kIn_.processSample(dryPreInGain);
    const double kout = kOut_.processSample(wet);
    pin_ += (kin * kin - pin_) * msCoeff_;
    pout_ += (kout * kout - pout_) * msCoeff_;

    const double lufsIn = -0.691 + 10.0 * std::log10(std::max(pin_, 1.0e-30));
    if (lufsIn < kAutoGainGateLufs)
        return dB2lin(dynGainDb_);

    const double ratio = std::sqrt(std::max(pin_, 1.0e-30) / std::max(pout_, 1.0e-30));
    double targetDb = lin2dB(ratio);
    targetDb = clamp(targetDb, -12.0, 12.0);
    dynGainDb_ += (targetDb - dynGainDb_) * smoothCoeff_;
    return dB2lin(dynGainDb_);
}

} // namespace wg::dsp
