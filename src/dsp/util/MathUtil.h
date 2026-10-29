#pragma once

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace wg::dsp {

[[nodiscard]] inline double dB2lin(double db) noexcept
{
    return std::pow(10.0, db / 20.0);
}

[[nodiscard]] inline double lin2dB(double linear) noexcept
{
    return 20.0 * std::log10(std::max(linear, 1.0e-300));
}

template<typename T>
[[nodiscard]] constexpr T clamp(T value, T lo, T hi) noexcept
{
    return std::min(hi, std::max(lo, value));
}

[[nodiscard]] inline double smoothstep(double t) noexcept
{
    const double x = clamp(t, 0.0, 1.0);
    return x * x * (3.0 - 2.0 * x);
}

[[nodiscard]] inline double ramp(double a, double a0, double a1) noexcept
{
    if (a1 <= a0)
        return a >= a1 ? 1.0 : 0.0;
    return clamp((a - a0) / (a1 - a0), 0.0, 1.0);
}

[[nodiscard]] inline double loginterp(double f0, double f1, double t) noexcept
{
    if (f0 <= 0.0 || f1 <= 0.0)
        return f0 + t * (f1 - f0);
    return f0 * std::pow(f1 / f0, t);
}

template<typename T>
[[nodiscard]] constexpr T lerp(T a, T b, double t) noexcept
{
    return static_cast<T>(static_cast<double>(a) + t * (static_cast<double>(b) - static_cast<double>(a)));
}

} // namespace wg::dsp
