#pragma once

#include "dsp/filters/Biquad.h"
#include "dsp/Constants.h"

#include <cmath>

namespace wg::dsp {

namespace detail {

[[nodiscard]] inline BiquadCoeffs designHighShelf(double sampleRate, double fcHz, double gainDb,
                                                  double q) noexcept
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * kPi * fcHz / sampleRate;
    const double cosW0 = std::cos(w0);
    const double sinW0 = std::sin(w0);
    const double alpha = sinW0 / (2.0 * q);

    const double Ap1 = A + 1.0;
    const double Am1 = A - 1.0;
    const double twoSqrtAalpha = 2.0 * std::sqrt(A) * alpha;

    BiquadCoeffs c{};
    const double a0 = Ap1 - Am1 * cosW0 + twoSqrtAalpha;
    c.b0 = A * (Ap1 + Am1 * cosW0 + twoSqrtAalpha) / a0;
    c.b1 = -2.0 * A * (Am1 + Ap1 * cosW0) / a0;
    c.b2 = A * (Ap1 + Am1 * cosW0 - twoSqrtAalpha) / a0;
    c.a1 = 2.0 * (Am1 - Ap1 * cosW0) / a0;
    c.a2 = (Ap1 - Am1 * cosW0 - twoSqrtAalpha) / a0;
    return c;
}

[[nodiscard]] inline BiquadCoeffs designHighpass(double sampleRate, double fcHz, double q) noexcept
{
    const double w0 = 2.0 * kPi * fcHz / sampleRate;
    const double cosW0 = std::cos(w0);
    const double sinW0 = std::sin(w0);
    const double alpha = sinW0 / (2.0 * q);

    BiquadCoeffs c{};
    const double a0 = 1.0 + alpha;
    c.b0 = (1.0 + cosW0) * 0.5 / a0;
    c.b1 = -(1.0 + cosW0) / a0;
    c.b2 = c.b0;
    c.a1 = -2.0 * cosW0 / a0;
    c.a2 = (1.0 - alpha) / a0;
    return c;
}

} // namespace detail

/**
 * BS.1770-4 K-weighting for loudness measurement (SPEC §4.7 Dynamic auto-gain).
 * Stage 1: +4 dB high shelf (1681.97 Hz, Q≈0.707455).
 * Stage 2: 2nd-order HPF (38.135 Hz, Q≈0.500327).
 */
class KWeighting
{
public:
    static constexpr double kShelfFcHz = 1681.974450955533;
    static constexpr double kShelfGainDb = 4.0;
    static constexpr double kShelfQ = 0.707455177833239;
    static constexpr double kHpfFcHz = 38.135470876024;
    static constexpr double kHpfQ = 0.500327337433432;

    void reset() noexcept
    {
        shelf_.reset();
        hpf_.reset();
    }

    void setSampleRate(double sampleRate) noexcept
    {
        shelf_.setCoeffs(detail::designHighShelf(sampleRate, kShelfFcHz, kShelfGainDb, kShelfQ));
        hpf_.setCoeffs(detail::designHighpass(sampleRate, kHpfFcHz, kHpfQ));
    }

    [[nodiscard]] double processSample(double x) noexcept
    {
        return hpf_.processSample(shelf_.processSample(x));
    }

    void processBlock(double* samples, int numSamples) noexcept
    {
        for (int n = 0; n < numSamples; ++n)
            samples[n] = processSample(samples[n]);
    }

private:
    Biquad shelf_{};
    Biquad hpf_{};
};

} // namespace wg::dsp
