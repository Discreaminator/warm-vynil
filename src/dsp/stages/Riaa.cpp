#include "Riaa.h"

#include "dsp/Constants.h"

namespace wg::dsp {

void Riaa::prepare(double fsi) noexcept
{
    fsi_ = fsi;
    const BiquadCoeffs play = designPlayBiquad(fsi);
    pre_.setCoeffs(invertCoeffs(play));
    de_.setCoeffs(play);
    tilt_.setTiltDb(fsi, tiltDb_);
    updateNeumann();
}

void Riaa::reset() noexcept
{
    pre_.reset();
    de_.reset();
    tilt_.reset();
    neumannX1_ = 0.0;
    neumannY1_ = 0.0;
}

void Riaa::setTargets(double tiltDb, double T4Sec) noexcept
{
    tiltDb_ = tiltDb;
    T4Sec_ = T4Sec;
    if (fsi_ > 0.0)
        tilt_.setTiltDb(fsi_, tiltDb_);
    updateNeumann();
}

void Riaa::updateNeumann() noexcept
{
    neumann_ = designNeumann(fsi_, T4Sec_);
}

double Riaa::processPreSample(double x) noexcept
{
    return pre_.processSample(x);
}

double Riaa::processNeumannSample(double x) noexcept
{
    if (neumann_.bypass)
        return x;
    const double y = neumann_.b0 * x + neumann_.b1 * neumannX1_ - neumann_.a1 * neumannY1_;
    neumannX1_ = x;
    neumannY1_ = y;
    return y;
}

double Riaa::processDeSample(double x) noexcept
{
    return de_.processSample(x);
}

double Riaa::processTiltSample(double x) noexcept
{
    return tilt_.processSample(x);
}

double Riaa::processPostChainSample(double xAfterTracing) noexcept
{
    return processTiltSample(processDeSample(xAfterTracing));
}

} // namespace wg::dsp
