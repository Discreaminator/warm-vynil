#include "TubeLut.h"

#include "dsp/util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

namespace {

constexpr double kMu = 100.0;
constexpr double kEx = 1.4;
constexpr double kKg1 = 1060.0;
constexpr double kKp = 600.0;
constexpr double kKvb = 300.0;
constexpr double kBplus = 250.0;
constexpr double kRa = 100000.0;

[[nodiscard]] double effectiveVgk(double vgk) noexcept
{
    if (vgk <= 0.0)
        return vgk;
    return 0.7 * std::tanh(vgk / 0.7);
}

[[nodiscard]] double plateCurrent(double vgkEff, double vp) noexcept
{
    const double arg = kKp * (1.0 / kMu + vgkEff / std::sqrt(kKvb + vp * vp));
    const double e1 = (vp / kKp) * std::log1p(std::exp(arg));
    if (e1 <= 0.0)
        return 0.0;
    return 2.0 * std::pow(e1, kEx) / kKg1;
}

} // namespace

double TubeLut::solveVp(double vgkEff) noexcept
{
    double lo = 0.0;
    double hi = kBplus;
    for (int i = 0; i < 60; ++i)
    {
        const double mid = 0.5 * (lo + hi);
        const double ip = plateCurrent(vgkEff, mid);
        const double f = kBplus - kRa * ip - mid;
        if (f > 0.0)
            lo = mid;
        else
            hi = mid;
    }
    return 0.5 * (lo + hi);
}

void TubeLut::prepare() noexcept
{
    const double step = (kVgkMax - kVgkMin) / static_cast<double>(kNumPoints - 1);
    for (int i = 0; i < kNumPoints; ++i)
    {
        const double vgk = kVgkMin + static_cast<double>(i) * step;
        const double vgkEff = effectiveVgk(vgk);
        vp_[static_cast<std::size_t>(i)] = solveVp(vgkEff);
    }

    for (int i = 0; i < kNumPoints; ++i)
    {
        if (i == 0)
            a0_[0] = (vp_[1] - vp_[0]) / step;
        else if (i == kNumPoints - 1)
            a0_[static_cast<std::size_t>(i)] = (vp_[static_cast<std::size_t>(i)] - vp_[static_cast<std::size_t>(i - 1)]) / step;
        else
            a0_[static_cast<std::size_t>(i)] =
                (vp_[static_cast<std::size_t>(i + 1)] - vp_[static_cast<std::size_t>(i - 1)]) / (2.0 * step);
    }

    ready_ = true;
}

namespace {

[[nodiscard]] double hermiteInterp(double x0, double x1, double y0, double y1, double m0, double m1, double x) noexcept
{
    const double t = (x - x0) / (x1 - x0);
    const double t2 = t * t;
    const double t3 = t2 * t;
    const double h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
    const double h10 = t3 - 2.0 * t2 + t;
    const double h01 = -2.0 * t3 + 3.0 * t2;
    const double h11 = t3 - t2;
    return h00 * y0 + h10 * (x1 - x0) * m0 + h01 * y1 + h11 * (x1 - x0) * m1;
}

template<typename Table>
[[nodiscard]] double lookupHermite(const Table& y, const Table& m, double vgk) noexcept
{
    const double step = (TubeLut::kVgkMax - TubeLut::kVgkMin) / static_cast<double>(TubeLut::kNumPoints - 1);
    const double pos = (vgk - TubeLut::kVgkMin) / step;
    const int i0 = clamp(static_cast<int>(std::floor(pos)), 0, TubeLut::kNumPoints - 2);
    const int i1 = i0 + 1;
    const double x0 = TubeLut::kVgkMin + static_cast<double>(i0) * step;
    const double x1 = TubeLut::kVgkMin + static_cast<double>(i1) * step;
    return hermiteInterp(x0, x1, y[static_cast<std::size_t>(i0)], y[static_cast<std::size_t>(i1)],
                         m[static_cast<std::size_t>(i0)], m[static_cast<std::size_t>(i1)], vgk);
}

} // namespace

double TubeLut::evaluateVp(double vgk) const noexcept
{
    if (!ready_)
        return 0.0;
    const double v = clamp(vgk, kVgkMin, kVgkMax);
    return lookupHermite(vp_, a0_, v);
}

double TubeLut::evaluateA0(double vgk) const noexcept
{
    if (!ready_)
        return -1.0;
    const double v = clamp(vgk, kVgkMin, kVgkMax);
    return lookupHermite(a0_, a0_, v);
}

} // namespace wg::dsp
