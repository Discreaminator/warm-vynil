#pragma once

#include "dsp/Constants.h"

#include <cmath>

namespace wg::dsp {

/** DC blocker: y = x - x_z1 + R*y_z1, R = exp(-2π·5/fs) (SPEC §4.1). */
template<typename Sample = double>
class DcBlocker
{
public:
    void reset() noexcept
    {
        xPrev_ = Sample(0);
        yPrev_ = Sample(0);
    }

    void setSampleRate(double sampleRate) noexcept
    {
        if (sampleRate <= 0.0)
        {
            r_ = Sample(0);
            return;
        }
        r_ = static_cast<Sample>(std::exp(-2.0 * kPi * kDcBlockHz / sampleRate));
    }

    [[nodiscard]] Sample processSample(Sample x) noexcept
    {
        const Sample y = x - xPrev_ + r_ * yPrev_;
        xPrev_ = x;
        yPrev_ = y;
        return y;
    }

private:
    Sample r_{Sample(0)};
    Sample xPrev_{Sample(0)};
    Sample yPrev_{Sample(0)};
};

} // namespace wg::dsp
