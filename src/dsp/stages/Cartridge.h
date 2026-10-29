#pragma once

#include "dsp/filters/SvfTpt.h"
#include "dsp/util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

/** Cartridge resonance SVF LP2 (SPEC §4.4). */
class Cartridge
{
public:
    void prepare(double fsi) noexcept { fsi_ = fsi; }

    void reset() noexcept { svf_.reset(); }

    void setTargets(double frHz, double q) noexcept
    {
        fr_ = frHz;
        q_ = q;
        updateCoeffs();
    }

    [[nodiscard]] double processSample(double x) noexcept { return svf_.processLowpass2(x); }

private:
    void updateCoeffs() noexcept
    {
        if (fsi_ <= 0.0)
            return;
        const double frMax = 0.45 * fsi_;
        const double fc = clamp(fr_, 1.0, frMax);
        svf_.setParams(fsi_, fc, q_);
    }

    double fsi_ = 48000.0;
    double fr_ = 14000.0;
    double q_ = 1.2;
    SvfTpt svf_{};
};

} // namespace wg::dsp
