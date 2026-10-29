#pragma once

#include <array>
#include <cmath>

namespace wg::dsp {

struct BiquadCoeffs
{
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
};

/** Transposed Direct Form II biquad in double (SPEC §4.3 RIAA). */
class Biquad
{
public:
    void reset() noexcept
    {
        s1_ = 0.0;
        s2_ = 0.0;
    }

    void setCoeffs(const BiquadCoeffs& c) noexcept { coeffs_ = c; }

    [[nodiscard]] const BiquadCoeffs& coeffs() const noexcept { return coeffs_; }

    [[nodiscard]] double processSample(double x) noexcept
    {
        const double y = coeffs_.b0 * x + s1_;
        s1_ = coeffs_.b1 * x - coeffs_.a1 * y + s2_;
        s2_ = coeffs_.b2 * x - coeffs_.a2 * y;
        return y;
    }

    void processBlock(double* samples, int numSamples) noexcept
    {
        for (int n = 0; n < numSamples; ++n)
            samples[n] = processSample(samples[n]);
    }

private:
    BiquadCoeffs coeffs_{};
    double s1_ = 0.0;
    double s2_ = 0.0;
};

/** Swap numerator/denominator and normalize so the new b0 = 1 (RIAA pre-emphasis from play). */
[[nodiscard]] inline BiquadCoeffs invertCoeffs(const BiquadCoeffs& play) noexcept
{
    BiquadCoeffs inv{};
    inv.b0 = 1.0;
    inv.b1 = play.a1;
    inv.b2 = play.a2;
    inv.a1 = play.b1;
    inv.a2 = play.b2;
    if (std::abs(play.b0) > 1.0e-30)
    {
        inv.b1 /= play.b0;
        inv.b2 /= play.b0;
        inv.a1 /= play.b0;
        inv.a2 /= play.b0;
    }
    return inv;
}

} // namespace wg::dsp
