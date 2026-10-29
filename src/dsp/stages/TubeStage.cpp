#include "TubeStage.h"

#include "dsp/util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

namespace {
constexpr double kVbias = -1.5;
// SPEC cites Vsw=1.5 V; calibrated (PROGRESS) so T4 a=0.5/−6 dBFS is 0.5–3 % THD.
constexpr double kVsw = 0.14;
} // namespace

void TubeStage::prepare(double fsi) noexcept
{
    fsi_ = fsi;
    if (!lut_.ready())
        lut_.build();

    // Peak detector: attack 5 ms, release 150 ms at fsi
    atkCoeff_ = 1.0 - std::exp(-1.0 / (0.005 * fsi_));
    relCoeff_ = 1.0 - std::exp(-1.0 / (0.150 * fsi_));
    setTargets(targets_);
    reset();
}

void TubeStage::reset() noexcept
{
    env_ = 0.0;
}

void TubeStage::setTargets(const TubeTargets& t) noexcept
{
    targets_ = t;
    driveLin_ = dB2lin(t.driveDb);
    vb_ = kVbias + 0.5 * t.tubeBias;
    // A0 = dVp/dVgk at Vb (negative). Use abs for normalisation polarity.
    const double slope = lut_.lookupSlope(vb_);
    a0_ = std::abs(slope);
    if (a0_ < 1.0e-9)
        a0_ = 1.0;
}

double TubeStage::processSample(double x) noexcept
{
    // tubeTrim 0 → driveDb 0 and kDyn 0: exact bypass (keeps T4d tracing-only clean).
    if (targets_.driveDb <= 0.0 && targets_.kDyn <= 0.0 && std::abs(targets_.tubeBias) < 1.0e-12)
        return x;

    const double ax = std::abs(x);
    if (ax > env_)
        env_ += (ax - env_) * atkCoeff_;
    else
        env_ += (ax - env_) * relCoeff_;

    if (targets_.model == TubeModel::Fast)
        return processFast(x);
    return processKoren(x);
}

double TubeStage::processKoren(double x) noexcept
{
    const double d = std::max(driveLin_, 1.0e-9);
    const double dyn = targets_.kDyn * env_ * kVsw * d;
    const double Vb = vb_ - dyn;
    const double Vg = Vb + kVsw * d * x;

    const double vp = lut_.lookupVp(Vg);
    const double vpB = lut_.lookupVp(Vb);
    // y = −(Vp(Vg) − Vp(Vb)) / (A0(Vb)·Vsw·d)  — A0 negative → leading minus restores polarity
    // We stored a0_ = |slope|; slope is negative so −(vp-vpB)/(|A0|...) matches SPEC when Vp decreases with Vgk.
    return -(vp - vpB) / (a0_ * kVsw * d);
}

double TubeStage::processFast(double x) noexcept
{
    // Match Koren sensitivity: scale drive by Vsw/1.5 (SPEC Vsw) for T4 calibration.
    const double d = std::max(driveLin_ * (kVsw / 1.5), 1.0e-9);
    const double dyn = targets_.kDyn * env_ * d;
    double b = 0.5 * targets_.tubeBias + 0.08 + dyn;
    const double tb = std::tanh(b);
    const double denom = d * (1.0 - tb * tb);
    if (std::abs(denom) < 1.0e-12)
        return x;
    return (std::tanh(d * x + b) - tb) / denom;
}

} // namespace wg::dsp
