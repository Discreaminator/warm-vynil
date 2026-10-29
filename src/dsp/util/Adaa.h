#pragma once

#include "dsp/util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

/**
 * First-order ADAA helper (SPEC §6.2, M4).
 * F is the antiderivative of f; when |du| is tiny, falls back to f at the midpoint.
 */
template<typename F, typename Antiderivative>
[[nodiscard]] inline double adaa1Sample(double u, double uPrev, F f, Antiderivative Fint,
                                        double deltaEps = 1.0e-6) noexcept
{
    const double du = u - uPrev;
    if (std::abs(du) < deltaEps)
        return f(0.5 * (u + uPrev));

    const double num = Fint(u) - Fint(uPrev);
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4723) // guarded |du| >= eps above
#endif
    const double y = num / du;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    return y;
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
