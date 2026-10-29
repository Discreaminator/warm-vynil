#include "RiaaDesign.h"

#include "dsp/Constants.h"
#include "dsp/util/MathUtil.h"

#include <array>
#include <cmath>

namespace wg::dsp {

namespace {

constexpr int kGridPoints = 600;
constexpr double kRefHz = 1000.0;

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
    const double cw = std::cos(w);
    const double sw = std::sin(w);
    const double c2w = std::cos(2.0 * w);
    const double s2w = std::sin(2.0 * w);

    const double nr = c.b0 + c.b1 * cw + c.b2 * c2w;
    const double ni = -c.b1 * sw - c.b2 * s2w;
    const double dr = 1.0 + c.a1 * cw + c.a2 * c2w;
    const double di = -c.a1 * sw - c.a2 * s2w;

    const double mag = std::sqrt((nr * nr + ni * ni) / std::max(dr * dr + di * di, 1.0e-300));
    return lin2dB(mag);
}

[[nodiscard]] BiquadCoeffs bilinearInitial(double fsi) noexcept
{
    const double fs = fsi;
    const double T1 = kRiaaT1Sec;
    const double T2 = kRiaaT2Sec;
    const double T3 = kRiaaT3Sec;

    const double a0s = T1 * T3;
    const double a1s = T1 + T3;
    const double b0s = 1.0;
    const double b1s = T2;

    const double k = 2.0 * fs;
    const auto bil = [&](double b0, double b1, double b2, double a0, double a1, double a2) {
        BiquadCoeffs c{};
        c.b0 = (b0 * k * k + b1 * k + b2) / (a0 * k * k + a1 * k + a2);
        c.b1 = (2.0 * b0 * k * k - 2.0 * b2) / (a0 * k * k + a1 * k + a2);
        c.b2 = (b0 * k * k - b1 * k + b2) / (a0 * k * k + a1 * k + a2);
        c.a1 = (2.0 * a0 * k * k - 2.0 * a2) / (a0 * k * k + a1 * k + a2);
        c.a2 = (a0 * k * k - a1 * k + a2) / (a0 * k * k + a1 * k + a2);
        return c;
    };

    return bil(b0s, b1s, 0.0, a0s, a1s, 1.0);
}

void normalizeAt1k(BiquadCoeffs& c, double fsi) noexcept
{
    const double g = dB2lin(-biquadMagDb(c, kRefHz, fsi));
    c.b0 *= g;
    c.b1 *= g;
    c.b2 *= g;
}

[[nodiscard]] BiquadCoeffs fitPlayBiquad(double fsi) noexcept
{
    const double fMax = std::min(0.49 * fsi, 40000.0);
    const double fMin = 10.0;
    const double logMin = std::log10(fMin);
    const double logMax = std::log10(fMax);

    std::array<double, kGridPoints> freqs{};
    std::array<double, kGridPoints> targetDb{};
    std::array<double, kGridPoints> weights{};
    for (int i = 0; i < kGridPoints; ++i)
    {
        const double t = static_cast<double>(i) / static_cast<double>(kGridPoints - 1);
        freqs[static_cast<std::size_t>(i)] = std::pow(10.0, logMin + t * (logMax - logMin));
        targetDb[static_cast<std::size_t>(i)] = analogPlayMagDb(freqs[static_cast<std::size_t>(i)]);
        const double f = freqs[static_cast<std::size_t>(i)];
        weights[static_cast<std::size_t>(i)] = (f >= 20.0 && f <= 20000.0) ? 1.0 : 0.2;
    }

    const double refTarget = analogPlayMagDb(kRefHz);
    for (int i = 0; i < kGridPoints; ++i)
        targetDb[static_cast<std::size_t>(i)] -= refTarget;

    BiquadCoeffs c = bilinearInitial(fsi);
    normalizeAt1k(c, fsi);

    std::array<double, 5> p{c.b0, c.b1, c.b2, c.a1, c.a2};
    double lambda = 1.0e-3;

    for (int iter = 0; iter < 80; ++iter)
    {
        BiquadCoeffs trial{};
        trial.b0 = p[0];
        trial.b1 = p[1];
        trial.b2 = p[2];
        trial.a1 = p[3];
        trial.a2 = p[4];

        std::array<double, 5> grad{};
        double cost = 0.0;

        for (int i = 0; i < kGridPoints; ++i)
        {
            const double err = biquadMagDb(trial, freqs[static_cast<std::size_t>(i)], fsi)
                               - targetDb[static_cast<std::size_t>(i)];
            const double w = weights[static_cast<std::size_t>(i)];
            cost += w * err * err;

            constexpr double eps = 1.0e-6;
            for (int j = 0; j < 5; ++j)
            {
                std::array<double, 5> pp = p;
                pp[static_cast<std::size_t>(j)] += eps;
                BiquadCoeffs pert{};
                pert.b0 = pp[0];
                pert.b1 = pp[1];
                pert.b2 = pp[2];
                pert.a1 = pp[3];
                pert.a2 = pp[4];
                const double errP = biquadMagDb(pert, freqs[static_cast<std::size_t>(i)], fsi)
                                    - targetDb[static_cast<std::size_t>(i)];
                grad[static_cast<std::size_t>(j)] += w * 2.0 * err * (errP - err) / eps;
            }
        }

        std::array<double, 5> step{};
        for (int j = 0; j < 5; ++j)
            step[static_cast<std::size_t>(j)] = -grad[static_cast<std::size_t>(j)]
                                                / (1.0 + lambda * (1.0 + std::abs(grad[static_cast<std::size_t>(j)])));

        std::array<double, 5> pNew = p;
        for (int j = 0; j < 5; ++j)
            pNew[static_cast<std::size_t>(j)] += step[static_cast<std::size_t>(j)];

        BiquadCoeffs trialNew{};
        trialNew.b0 = pNew[0];
        trialNew.b1 = pNew[1];
        trialNew.b2 = pNew[2];
        trialNew.a1 = pNew[3];
        trialNew.a2 = pNew[4];

        double costNew = 0.0;
        for (int i = 0; i < kGridPoints; ++i)
        {
            const double err = biquadMagDb(trialNew, freqs[static_cast<std::size_t>(i)], fsi)
                               - targetDb[static_cast<std::size_t>(i)];
            costNew += weights[static_cast<std::size_t>(i)] * err * err;
        }

        if (costNew < cost)
        {
            p = pNew;
            lambda *= 0.7;
        }
        else
        {
            lambda *= 2.0;
        }
    }

    BiquadCoeffs out{};
    out.b0 = p[0];
    out.b1 = p[1];
    out.b2 = p[2];
    out.a1 = p[3];
    out.a2 = p[4];
    normalizeAt1k(out, fsi);
    return forceMinimumPhase(out);
}

} // namespace

BiquadCoeffs forceMinimumPhase(BiquadCoeffs c) noexcept
{
    if (std::abs(c.b0) < 1.0e-30)
        return c;

    const double a = c.b2 / c.b0;
    const double b = c.b1 / c.b0;
    const double disc = b * b - 4.0 * a;
    if (disc < 0.0)
        return c;

    const double sqrtDisc = std::sqrt(disc);
    double u1 = (-b + sqrtDisc) * 0.5;
    double u2 = (-b - sqrtDisc) * 0.5;

    auto reflectZero = [](double uInv) -> double {
        if (std::abs(uInv) < 1.0e-14)
            return uInv;
        const double z = 1.0 / uInv;
        if (std::abs(z) > 1.0 + 1.0e-6)
            return 1.0 / z;
        return uInv;
    };

    double gain = c.b0;
    const double zA = 1.0 / u1;
    if (std::abs(zA) > 1.0 + 1.0e-6)
        gain *= std::abs(zA);
    const double zB = 1.0 / u2;
    if (std::abs(zB) > 1.0 + 1.0e-6)
        gain *= std::abs(zB);

    u1 = reflectZero(u1);
    u2 = reflectZero(u2);

    c.b0 = gain;
    c.b1 = -gain * (u1 + u2);
    c.b2 = gain * u1 * u2;
    return c;
}

BiquadCoeffs designPlayBiquad(double fsi) noexcept
{
    static double cachedFsi = 0.0;
    static BiquadCoeffs cached{};

    if (fsi <= 0.0)
        return BiquadCoeffs{};

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
