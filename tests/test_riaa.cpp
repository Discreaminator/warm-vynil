#include "dsp/filters/Biquad.h"
#include "dsp/stages/Riaa.h"
#include "dsp/stages/RiaaDesign.h"
#include "dsp/Constants.h"
#include "dsp/util/MathUtil.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <complex>
#include <vector>

namespace {

[[nodiscard]] double analogPlayDb(double f) noexcept
{
    const double w = 2.0 * wg::dsp::kPi * f;
    const double num = std::sqrt(1.0 + (w * wg::dsp::kRiaaT2Sec) * (w * wg::dsp::kRiaaT2Sec));
    const double d1 = std::sqrt(1.0 + (w * wg::dsp::kRiaaT1Sec) * (w * wg::dsp::kRiaaT1Sec));
    const double d2 = std::sqrt(1.0 + (w * wg::dsp::kRiaaT3Sec) * (w * wg::dsp::kRiaaT3Sec));
    return wg::dsp::lin2dB(num / (d1 * d2));
}

[[nodiscard]] double biquadMagDb(const wg::dsp::BiquadCoeffs& c, double f, double fs) noexcept
{
    const double w = 2.0 * wg::dsp::kPi * f / fs;
    const double cw = std::cos(w), sw = std::sin(w);
    const double c2 = std::cos(2 * w), s2 = std::sin(2 * w);
    const double nr = c.b0 + c.b1 * cw + c.b2 * c2;
    const double ni = -(c.b1 * sw + c.b2 * s2);
    const double dr = 1.0 + c.a1 * cw + c.a2 * c2;
    const double di = -(c.a1 * sw + c.a2 * s2);
    return wg::dsp::lin2dB(std::sqrt((nr * nr + ni * ni) / std::max(dr * dr + di * di, 1e-300)));
}

[[nodiscard]] double maxRootRadius(double a, double b) noexcept
{
    // Roots of z^2 + b z + a = 0
    const double disc = b * b - 4.0 * a;
    if (disc < 0.0)
    {
        const double re = -b * 0.5;
        const double im = std::sqrt(-disc) * 0.5;
        return std::sqrt(re * re + im * im);
    }
    const double s = std::sqrt(disc);
    return std::max(std::abs((-b + s) * 0.5), std::abs((-b - s) * 0.5));
}

[[nodiscard]] bool polesZerosInside(const wg::dsp::BiquadCoeffs& c) noexcept
{
    if (std::abs(c.b0) < 1e-30)
        return false;
    const double zR = maxRootRadius(c.b2 / c.b0, c.b1 / c.b0);
    const double pR = maxRootRadius(c.a2, c.a1);
    INFO("zR=" << zR << " pR=" << pR);
    return zR < 1.0 && pR < 1.0;
}

} // namespace

TEST_CASE("T1 RIAA de-emphasis vs analog within 0.1 dB", "[t1][riaa]")
{
    const std::vector<double> rates = { 88200, 96000, 176400, 192000, 352800, 384000 };
    for (double fsi : rates)
    {
        INFO("fsi=" << fsi);
        const auto play = wg::dsp::designPlayBiquad(fsi);
        const double zR = maxRootRadius(play.b2 / play.b0, play.b1 / play.b0);
        const double pR = maxRootRadius(play.a2, play.a1);
        INFO("zR=" << zR << " pR=" << pR << " b0=" << play.b0);
        REQUIRE(zR < 1.0);
        REQUIRE(pR < 1.0);

        const double refD = biquadMagDb(play, 1000.0, fsi);
        const double refA = analogPlayDb(1000.0);
        double maxErr = 0.0;
        for (double f = 20.0; f <= 20000.0; f *= 1.05)
        {
            const double err = (biquadMagDb(play, f, fsi) - refD) - (analogPlayDb(f) - refA);
            maxErr = std::max(maxErr, std::abs(err));
        }
        REQUIRE(maxErr <= 0.1);
    }
}

TEST_CASE("T1b pre-de cancels to unity", "[t1b][riaa]")
{
    constexpr double fsi = 192000.0;
    wg::dsp::Riaa riaa;
    riaa.prepare(fsi);
    riaa.setTargets(0.0, 0.0); // no tilt, no Neumann
    riaa.reset();

    // Impulse through pre → de (no tracing)
    constexpr int N = 4096;
    std::vector<double> y(static_cast<std::size_t>(N), 0.0);
    for (int i = 0; i < N; ++i)
    {
        double x = (i == 0) ? 1.0 : 0.0;
        x = riaa.processPreSample(x);
        x = riaa.processNeumannSample(x);
        x = riaa.processDeSample(x);
        y[static_cast<std::size_t>(i)] = x;
    }

    REQUIRE(std::abs(y[0] - 1.0) < 1e-9);
    double maxTail = 0.0;
    for (int i = 1; i < N; ++i)
        maxTail = std::max(maxTail, std::abs(y[static_cast<std::size_t>(i)]));
    REQUIRE(maxTail < 1e-9);
}
