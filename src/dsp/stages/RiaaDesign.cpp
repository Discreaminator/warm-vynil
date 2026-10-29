#include "RiaaDesign.h"
#include "RiaaCoeffs.generated.h"

#include "dsp/Constants.h"
#include "dsp/util/MathUtil.h"

#include <array>
#include <cmath>

namespace wg::dsp {

namespace {

constexpr int kGridPoints = 400;
constexpr double kRefHz = 1000.0;
constexpr double kMaxRadius = 1.0 - 1.0e-6;

[[nodiscard]] double analogPlayMagDb(double freqHz) noexcept
{
    if (freqHz <= 0.0)
        return 0.0;
    const double w = 2.0 * kPi * freqHz;
    const double num = std::sqrt(1.0 + (w * kRiaaT2Sec) * (w * kRiaaT2Sec));
    const double d1 = std::sqrt(1.0 + (w * kRiaaT1Sec) * (w * kRiaaT1Sec));
    const double d2 = std::sqrt(1.0 + (w * kRiaaT3Sec) * (w * kRiaaT3Sec));
    return lin2dB(num / (d1 * d2));
}

[[nodiscard]] double biquadMagDb(const BiquadCoeffs& c, double freqHz, double fsi) noexcept
{
    const double w = 2.0 * kPi * freqHz / fsi;
    const double cw = std::cos(w), sw = std::sin(w);
    const double c2w = std::cos(2.0 * w), s2w = std::sin(2.0 * w);
    const double nr = c.b0 + c.b1 * cw + c.b2 * c2w;
    const double ni = -c.b1 * sw - c.b2 * s2w;
    const double dr = 1.0 + c.a1 * cw + c.a2 * c2w;
    const double di = -c.a1 * sw - c.a2 * s2w;
    return lin2dB(std::sqrt((nr * nr + ni * ni) / std::max(dr * dr + di * di, 1.0e-300)));
}

[[nodiscard]] double maxPoleRadius(const BiquadCoeffs& c) noexcept
{
    const double disc = c.a1 * c.a1 - 4.0 * c.a2;
    if (disc < 0.0)
    {
        const double re = -0.5 * c.a1;
        const double im = 0.5 * std::sqrt(-disc);
        return std::sqrt(re * re + im * im);
    }
    const double s = std::sqrt(disc);
    return std::max(std::abs((-c.a1 + s) * 0.5), std::abs((-c.a1 - s) * 0.5));
}

void normalizeAt1k(BiquadCoeffs& c, double fsi) noexcept
{
    const double g = dB2lin(-biquadMagDb(c, kRefHz, fsi));
    c.b0 *= g;
    c.b1 *= g;
    c.b2 *= g;
}

[[nodiscard]] BiquadCoeffs matchedZPlay(double fsi) noexcept
{
    const double zz = std::exp(-1.0 / (kRiaaT2Sec * fsi));
    const double p1 = std::exp(-1.0 / (kRiaaT1Sec * fsi));
    const double p3 = std::exp(-1.0 / (kRiaaT3Sec * fsi));
    BiquadCoeffs c{};
    c.b0 = 1.0;
    c.b1 = -zz;
    c.b2 = 0.0;
    c.a1 = -(p1 + p3);
    c.a2 = p1 * p3;
    return c;
}

[[nodiscard]] BiquadCoeffs lookupTable(double fsi) noexcept
{
    using namespace riaa_table;
    if (fsi <= kFs[0])
        return BiquadCoeffs{ kB0[0], kB1[0], kB2[0], kA1[0], kA2[0] };
    if (fsi >= kFs[kN - 1])
        return BiquadCoeffs{ kB0[kN - 1], kB1[kN - 1], kB2[kN - 1], kA1[kN - 1], kA2[kN - 1] };

    std::size_t lo = 0;
    std::size_t hi = kN - 1;
    while (hi - lo > 1)
    {
        const std::size_t mid = lo + (hi - lo) / 2;
        if (kFs[mid] <= fsi)
            lo = mid;
        else
            hi = mid;
    }
    const double t = (std::log(fsi) - std::log(kFs[lo])) / (std::log(kFs[hi]) - std::log(kFs[lo]));
    auto lerp = [t](double a, double b) { return a + t * (b - a); };
    BiquadCoeffs c{};
    c.b0 = lerp(kB0[lo], kB0[hi]);
    c.b1 = lerp(kB1[lo], kB1[hi]);
    c.b2 = lerp(kB2[lo], kB2[hi]);
    c.a1 = lerp(kA1[lo], kA1[hi]);
    c.a2 = lerp(kA2[lo], kA2[hi]);
    return c;
}

/** Refine zero ratios with 2×2 LM; poles stay from init (table / matched-z). */
[[nodiscard]] BiquadCoeffs refineZeros(BiquadCoeffs c, double fsi) noexcept
{
    normalizeAt1k(c, fsi);
    if (std::abs(c.b0) < 1.0e-30)
        return c;

    const double a1 = c.a1;
    const double a2 = c.a2;
    double zb1 = c.b1 / c.b0;
    double zb2 = c.b2 / c.b0;

    const double fMax = std::min(0.49 * fsi, 40000.0);
    std::array<double, kGridPoints> freqs{}, targetDb{}, weights{};
    for (int i = 0; i < kGridPoints; ++i)
    {
        const double u = static_cast<double>(i) / static_cast<double>(kGridPoints - 1);
        const double f = std::pow(10.0, std::log10(10.0) + u * (std::log10(fMax) - std::log10(10.0)));
        freqs[static_cast<std::size_t>(i)] = f;
        targetDb[static_cast<std::size_t>(i)] = analogPlayMagDb(f) - analogPlayMagDb(kRefHz);
        weights[static_cast<std::size_t>(i)] = (f >= 20.0 && f <= 20000.0) ? 1.0 : 0.25;
    }

    auto make = [&](double z1, double z2) {
        BiquadCoeffs t{ 1.0, z1, z2, a1, a2 };
        normalizeAt1k(t, fsi);
        return t;
    };
    auto costOf = [&](double z1, double z2) {
        const BiquadCoeffs t = make(z1, z2);
        double s = 0.0;
        for (int i = 0; i < kGridPoints; ++i)
        {
            const double err = biquadMagDb(t, freqs[static_cast<std::size_t>(i)], fsi)
                               - targetDb[static_cast<std::size_t>(i)];
            s += weights[static_cast<std::size_t>(i)] * err * err;
        }
        return s;
    };

    double best = costOf(zb1, zb2);
    double lambda = 1.0e-3;
    for (int iter = 0; iter < 40; ++iter)
    {
        const BiquadCoeffs base = make(zb1, zb2);
        double g1 = 0, g2 = 0, h11 = 0, h12 = 0, h22 = 0;
        for (int i = 0; i < kGridPoints; ++i)
        {
            const double err = biquadMagDb(base, freqs[static_cast<std::size_t>(i)], fsi)
                               - targetDb[static_cast<std::size_t>(i)];
            const double w = weights[static_cast<std::size_t>(i)];
            constexpr double eps = 1.0e-6;
            const double j1 = (biquadMagDb(make(zb1 + eps, zb2), freqs[static_cast<std::size_t>(i)], fsi)
                               - targetDb[static_cast<std::size_t>(i)] - err)
                              / eps;
            const double j2 = (biquadMagDb(make(zb1, zb2 + eps), freqs[static_cast<std::size_t>(i)], fsi)
                               - targetDb[static_cast<std::size_t>(i)] - err)
                              / eps;
            g1 += w * err * j1;
            g2 += w * err * j2;
            h11 += w * j1 * j1;
            h12 += w * j1 * j2;
            h22 += w * j2 * j2;
        }

        bool ok = false;
        for (int attempt = 0; attempt < 8 && !ok; ++attempt)
        {
            const double A11 = h11 + lambda;
            const double A22 = h22 + lambda;
            const double det = A11 * A22 - h12 * h12;
            if (std::abs(det) < 1.0e-30)
            {
                lambda *= 10.0;
                continue;
            }
            const double nz1 = zb1 - (A22 * g1 - h12 * g2) / det;
            const double nz2 = zb2 - (-h12 * g1 + A11 * g2) / det;
            const double cNew = costOf(nz1, nz2);
            if (cNew < best)
            {
                zb1 = nz1;
                zb2 = nz2;
                best = cNew;
                lambda = std::max(lambda * 0.3, 1.0e-12);
                ok = true;
            }
            else
                lambda *= 6.0;
        }
        if (!ok && lambda > 1.0e8)
            break;
        if (best < 1.0e-10)
            break;
    }
    return make(zb1, zb2);
}

[[nodiscard]] BiquadCoeffs fitPlayBiquad(double fsi) noexcept
{
    // Prefer offline LS table (riaa_reference.py); refine only outside / between sparsely.
    const bool inTable = fsi >= riaa_table::kFs[0] && fsi <= riaa_table::kFs[riaa_table::kN - 1];
    BiquadCoeffs c = inTable ? lookupTable(fsi) : matchedZPlay(fsi);

    if (!inTable)
    {
        const auto mz = matchedZPlay(fsi);
        c.a1 = mz.a1;
        c.a2 = mz.a2;
        c = refineZeros(c, fsi);
    }

    normalizeAt1k(c, fsi);
    if (maxPoleRadius(c) >= kMaxRadius)
    {
        c = matchedZPlay(fsi);
        normalizeAt1k(c, fsi);
    }
    // Table coeffs are already min-phase; skip forceMinimumPhase there so T1b
    // inversion stays bit-exact and T1 keeps the LS magnitude fit.
    if (!inTable)
        c = forceMinimumPhase(c);
    return c;
}

} // namespace

BiquadCoeffs forceMinimumPhase(BiquadCoeffs c) noexcept
{
    if (std::abs(c.b0) < 1.0e-30)
        return c;

    const double bb = c.b1 / c.b0;
    const double aa = c.b2 / c.b0;
    const double disc = bb * bb - 4.0 * aa;
    double gain = c.b0;

    auto pull = [&](double z) {
        double az = std::abs(z);
        if (az > 1.0 + 1.0e-6)
        {
            gain *= az;
            z = 1.0 / z;
            az = std::abs(z);
        }
        if (az >= kMaxRadius && az > 1.0e-18)
            z *= (kMaxRadius * 0.999999) / az;
        return z;
    };

    if (disc < 0.0)
    {
        double re = -0.5 * bb;
        double im = 0.5 * std::sqrt(-disc);
        double r = std::sqrt(re * re + im * im);
        if (r > 1.0 + 1.0e-6)
        {
            gain *= r * r;
            re /= (r * r);
            im /= (r * r);
            r = std::sqrt(re * re + im * im);
        }
        if (r >= kMaxRadius && r > 1.0e-18)
        {
            const double s = (kMaxRadius * 0.999999) / r;
            re *= s;
            im *= s;
        }
        c.b0 = gain;
        c.b1 = -2.0 * gain * re;
        c.b2 = gain * (re * re + im * im);
        return c;
    }

    double z1 = pull((-bb + std::sqrt(disc)) * 0.5);
    double z2 = pull((-bb - std::sqrt(disc)) * 0.5);
    c.b0 = gain;
    c.b1 = -gain * (z1 + z2);
    c.b2 = gain * z1 * z2;
    return c;
}

BiquadCoeffs designPlayBiquad(double fsi) noexcept
{
    static double cachedFsi = 0.0;
    static BiquadCoeffs cached{};
    if (fsi <= 0.0)
        return {};
    if (cachedFsi == fsi && cached.b0 != 0.0)
        return cached;
    cached = fitPlayBiquad(fsi);
    cachedFsi = fsi;
    return cached;
}

NeumannCoeffs designNeumann(double fsi, double T4Sec) noexcept
{
    NeumannCoeffs n{};
    if (T4Sec < kNeumannBypassTSec || fsi <= 0.0)
    {
        n.bypass = true;
        return n;
    }
    n.bypass = false;
    n.a1 = -std::exp(-1.0 / (T4Sec * fsi));
    const double magNy = 1.0 / std::sqrt(1.0 + (kPi * fsi * T4Sec) * (kPi * fsi * T4Sec));
    n.b0 = 0.5 * ((1.0 + n.a1) + (1.0 - n.a1) * magNy);
    n.b1 = 0.5 * ((1.0 + n.a1) - (1.0 - n.a1) * magNy);
    return n;
}

} // namespace wg::dsp
