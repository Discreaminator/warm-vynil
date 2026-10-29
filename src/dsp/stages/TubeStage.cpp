#include "TubeStage.h"

#include "dsp/util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

void TubeStage::prepare(double fsi, TubeLut& lut) noexcept
{
    fsi_ = fsi;
    lut_ = &lut;
    updateEnvCoeffs();
}

void TubeStage::updateEnvCoeffs() noexcept
{
    if (fsi_ <= 0.0)
    {
        attackCoeff_ = 1.0;
        releaseCoeff_ = 1.0;
        return;
    }
    const auto tauToCoeff = [this](double ms) {
        const double tau = fsi_ * ms * 0.001;
        return 1.0 - std::exp(-1.0 / tau);
    };
    attackCoeff_ = tauToCoeff(5.0);
    releaseCoeff_ = tauToCoeff(150.0);
}

double TubeStage::processKoren(double x) noexcept
{
    if (lut_ == nullptr || !lut_->isReady())
        return x;

    const double d = dB2lin(targets_.driveDb);
    const double dyn = targets_.kDyn * env_ * kVsw * d;
    const double vb = TubeLut::kVbias + 0.5 * targets_.tubeBias - dyn;
    const double vg = vb + kVsw * d * x;

    const double vp = lut_->evaluateVp(vg);
    const double vpBias = lut_->evaluateVp(vb);
    const double a0 = lut_->evaluateA0(vb);
    const double denom = a0 * kVsw * d;
    if (std::abs(denom) < 1.0e-12)
        return x;

    return -(vp - vpBias) / denom;
}

double TubeStage::processFast(double x) noexcept
{
    const double d = dB2lin(targets_.driveDb);
    const double dyn = targets_.kDyn * env_ * kVsw * d;
    const double b = 0.5 * targets_.tubeBias + 0.2 - dyn;
    const double tb = std::tanh(b);
    const double num = std::tanh(d * x + b) - tb;
    const double denom = d * (1.0 - tb * tb);
    if (std::abs(denom) < 1.0e-12)
        return x;
    return num / denom;
}

double TubeStage::processSample(double x) noexcept
{
    const double ax = std::abs(x);
    const double coeff = ax > env_ ? attackCoeff_ : releaseCoeff_;
    env_ += (ax - env_) * coeff;

    if (targets_.model == TubeModel::Fast)
        return processFast(x);
    return processKoren(x);
}

} // namespace wg::dsp
