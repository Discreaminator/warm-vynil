#include "TubeLut.h"

#include <cmath>

namespace wg::dsp {

namespace {
constexpr double kMu = 100.0;
constexpr double kEx = 1.4;
constexpr double kKg1 = 1060.0;
constexpr double kKp = 600.0;
constexpr double kKvb = 300.0;
constexpr double kBp = 250.0;
constexpr double kRa = 100000.0;
} // namespace

double TubeLut::korenIp(double vgk, double vp) noexcept
{
    const double e1 = (vp / kKp)
                      * std::log(1.0 + std::exp(kKp * (1.0 / kMu + vgk / std::sqrt(kKvb + vp * vp))));
    if (e1 <= 0.0)
        return 0.0;
    return 2.0 * std::pow(e1, kEx) / kKg1;
}

double TubeLut::solveVp(double vgk) noexcept
{
    // Effective grid with soft clip for Vgk > 0
    double vg = vgk;
    if (vg > 0.0)
        vg = 0.7 * std::tanh(vg / 0.7);

    double lo = 0.0;
    double hi = kBp;
    for (int i = 0; i < 60; ++i)
    {
        const double mid = 0.5 * (lo + hi);
        const double ip = korenIp(vg, mid);
        const double vpLoad = kBp - kRa * ip;
        if (vpLoad > mid)
            lo = mid;
        else
            hi = mid;
    }
    return 0.5 * (lo + hi);
}

void TubeLut::build() noexcept
{
    const double span = kVgkMax - kVgkMin;
    const double dx = span / static_cast<double>(kNumPoints - 1);

    for (std::size_t i = 0; i < kNumPoints; ++i)
    {
        const double vgk = kVgkMin + dx * static_cast<double>(i);
        vp_[i] = solveVp(vgk);
    }

    // Cumulative trapezoid antiderivative P, P(VgkMin)=0
    p_[0] = 0.0;
    for (std::size_t i = 1; i < kNumPoints; ++i)
        p_[i] = p_[i - 1] + 0.5 * (vp_[i] + vp_[i - 1]) * dx;

    ready_ = true;
}

double TubeLut::lookupVp(double vgk) const noexcept
{
    const double span = kVgkMax - kVgkMin;
    double t = (vgk - kVgkMin) / span;
    if (t <= 0.0)
        return vp_.front();
    if (t >= 1.0)
        return vp_.back();

    const double pos = t * static_cast<double>(kNumPoints - 1);
    const auto i0 = static_cast<std::size_t>(pos);
    const std::size_t i1 = std::min(i0 + 1u, kNumPoints - 1u);
    const double f = pos - static_cast<double>(i0);

    // Cubic Hermite using neighbouring slopes
    const auto im = i0 > 0 ? i0 - 1u : 0u;
    const auto ip = std::min(i1 + 1u, kNumPoints - 1u);
    const double y0 = vp_[i0];
    const double y1 = vp_[i1];
    const double m0 = 0.5 * (vp_[i1] - vp_[im]);
    const double m1 = 0.5 * (vp_[ip] - vp_[i0]);
    const double f2 = f * f;
    const double f3 = f2 * f;
    return (2.0 * f3 - 3.0 * f2 + 1.0) * y0 + (f3 - 2.0 * f2 + f) * m0
           + (-2.0 * f3 + 3.0 * f2) * y1 + (f3 - f2) * m1;
}

double TubeLut::lookupSlope(double vgk) const noexcept
{
    const double span = kVgkMax - kVgkMin;
    const double dx = span / static_cast<double>(kNumPoints - 1);
    double t = (vgk - kVgkMin) / span;
    if (t <= 0.0)
        return (vp_[1] - vp_[0]) / dx;
    if (t >= 1.0)
        return (vp_.back() - vp_[kNumPoints - 2]) / dx;

    const double pos = t * static_cast<double>(kNumPoints - 1);
    const auto i0 = static_cast<std::size_t>(pos);
    const std::size_t i1 = std::min(i0 + 1u, kNumPoints - 1u);
    return (vp_[i1] - vp_[i0]) / dx;
}

double TubeLut::lookupP(double vgk) const noexcept
{
    const double span = kVgkMax - kVgkMin;
    double t = (vgk - kVgkMin) / span;
    if (t <= 0.0)
        return p_.front();
    if (t >= 1.0)
        return p_.back();
    const double pos = t * static_cast<double>(kNumPoints - 1);
    const auto i0 = static_cast<std::size_t>(pos);
    const std::size_t i1 = std::min(i0 + 1u, kNumPoints - 1u);
    const double f = pos - static_cast<double>(i0);
    return p_[i0] + f * (p_[i1] - p_[i0]);
}

} // namespace wg::dsp
