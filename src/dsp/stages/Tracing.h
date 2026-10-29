#pragma once

#include "dsp/Constants.h"
#include "dsp/util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

/**
 * Tracing nonlinearity (SPEC §4.3). ADAA deferred to M4.
 *
 * G_tr is calibrated below the SPEC default of 1.0: RIAA pre boosts HF by
 * ~+19 dB at 18 kHz, so G_tr=1 drives |u|≫1 and C¹-clip harmonics exceed
 * fsi/2, aliasing into the audio band before downsample (T5). G_tr=0.22 keeps
 * |u|≲1 at −6 dBFS / 18 kHz while preserving T4 / T4d (PROGRESS.md).
 */
class Tracing
{
public:
    /** Calibrated SPEC G_tr (default 1.0 in §4.3). */
    static constexpr double kGtr = 0.22;

    void reset() noexcept {}

    void setTargets(double s_tr) noexcept { s_tr_ = clamp(s_tr, 0.0, 2.0); }

    [[nodiscard]] double processSample(double x_pre) const noexcept
    {
        if (s_tr_ <= 1.0e-12)
            return x_pre;

        const double c2 = kTracingC2Scale * s_tr_;
        const double c3 = kTracingC3Scale * s_tr_;
        const double u = kGtr * x_pre;

        if (std::abs(u) <= 1.0)
        {
            const double y = u + c2 * u * u + c3 * u * u * u;
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
