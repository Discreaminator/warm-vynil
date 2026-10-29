#pragma once

#include "dsp/Constants.h"
#include "dsp/util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

/** Tracing nonlinearity (SPEC §4.3). ADAA ignored in M1. */
class Tracing
{
public:
    void reset() noexcept {}

    void setTargets(double s_tr) noexcept { s_tr_ = clamp(s_tr, 0.0, 2.0); }

    [[nodiscard]] double processSample(double x_pre) const noexcept
    {
        constexpr double kGtr = 1.0;
        const double c2 = kTracingC2Scale * s_tr_;
        const double c3 = kTracingC3Scale * s_tr_;

        const double u = kGtr * x_pre;
        if (std::abs(u) <= 1.0)
        {
            const double uc = u;
            const double y = uc + c2 * uc * uc + c3 * uc * uc * uc;
            return y / kGtr;
        }

        const double uc = u > 0.0 ? 1.0 : -1.0;
        const double f1 = uc + c2 * uc * uc + c3 * uc * uc * uc;
        const double fp1 = 1.0 + 2.0 * c2 * uc + 3.0 * c3 * uc * uc;
        const double y = f1 + fp1 * (u - uc);
        return y / kGtr;
    }

private:
    double s_tr_ = 0.0;
};

} // namespace wg::dsp
