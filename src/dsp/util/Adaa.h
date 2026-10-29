#pragma once

#include "dsp/util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

/**
 * First-order ADAA helper (SPEC §6.2, M4). M1 stub — optional use in tracing/tube paths.
 * F is the antiderivative of f; when |du| is tiny, falls back to f at the midpoint.
 */
template<typename F, typename Antiderivative>
[[nodiscard]] inline double adaa1Sample(double u, double uPrev, F f, Antiderivative Fint,
                                        double deltaEps = 1.0e-6) noexcept
{
    const double du = u - uPrev;
    if (std::abs(du) < deltaEps)
        return f(0.5 * (u + uPrev));
    return (Fint(u) - Fint(uPrev)) / du;
}

/** Stateful first-order ADAA processor (no heap). */
template<typename F, typename Antiderivative>
class Adaa1Processor
{
public:
    void reset(double initialU = 0.0) noexcept { uPrev_ = initialU; }

    [[nodiscard]] double processSample(double u, F f, Antiderivative Fint,
                                       double deltaEps = 1.0e-6) noexcept
    {
        const double y = adaa1Sample(u, uPrev_, f, Fint, deltaEps);
        uPrev_ = u;
        return y;
    }

private:
    double uPrev_ = 0.0;
};

} // namespace wg::dsp
