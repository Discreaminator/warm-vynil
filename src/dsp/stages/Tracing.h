#pragma once

#include "dsp/Constants.h"
#include "dsp/util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

/**
 * Tracing nonlinearity (SPEC §4.3) with optional first-order ADAA (M4).
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

    void reset() noexcept { uPrev_ = 0.0; }

    void setTargets(double s_tr) noexcept { s_tr_ = clamp(s_tr, 0.0, 2.0); }

    void setAdaa(bool enabled) noexcept { adaa_ = enabled; }

    [[nodiscard]] double processSample(double x_pre) noexcept
    {
        if (s_tr_ <= 1.0e-12)
        {
            uPrev_ = 0.0;
            return x_pre;
        }

        const double c2 = kTracingC2Scale * s_tr_;
        const double c3 = kTracingC3Scale * s_tr_;
        const double u = kGtr * x_pre;

        const double yU = adaa_ ? processAdaa(u, c2, c3) : evalF(u, c2, c3);
        return yU / kGtr;
    }

private:
    [[nodiscard]] static double evalF(double u, double c2, double c3) noexcept
    {
        if (std::abs(u) <= 1.0)
            return u + c2 * u * u + c3 * u * u * u;

        const double uc = u > 0.0 ? 1.0 : -1.0;
        const double f1 = uc + c2 * uc * uc + c3 * uc * uc * uc;
        const double fp1 = 1.0 + 2.0 * c2 * uc + 3.0 * c3 * uc * uc;
        return f1 + fp1 * (u - uc);
    }

    /** Antiderivative of evalF, C¹-stitched outside |u|≤1 (SPEC §4.3). */
    [[nodiscard]] static double evalAntiderivative(double u, double c2, double c3) noexcept
    {
        if (std::abs(u) <= 1.0)
            return 0.5 * u * u + (c2 / 3.0) * u * u * u + (c3 / 4.0) * u * u * u * u;

        if (u > 1.0)
        {
            const double F1 = 0.5 + c2 / 3.0 + c3 / 4.0;
            const double f1 = 1.0 + c2 + c3;
            const double fp1 = 1.0 + 2.0 * c2 + 3.0 * c3;
            const double du = u - 1.0;
            return F1 + f1 * du + 0.5 * fp1 * du * du;
        }

        const double Fm = 0.5 - c2 / 3.0 + c3 / 4.0;
        const double fm = -1.0 + c2 - c3;
        const double fpm = 1.0 - 2.0 * c2 + 3.0 * c3;
        const double du = u + 1.0;
        return Fm + fm * du + 0.5 * fpm * du * du;
    }

    [[nodiscard]] double processAdaa(double u, double c2, double c3) noexcept
    {
        const double du = u - uPrev_;
        double y = 0.0;
        if (std::abs(du) < kAdaaDeltaEps)
            y = evalF(0.5 * (u + uPrev_), c2, c3);
        else
            y = (evalAntiderivative(u, c2, c3) - evalAntiderivative(uPrev_, c2, c3)) * (1.0 / du);
        uPrev_ = u;
        return y;
    }

    double s_tr_ = 0.0;
    bool adaa_ = true;
    double uPrev_ = 0.0;
};

} // namespace wg::dsp
