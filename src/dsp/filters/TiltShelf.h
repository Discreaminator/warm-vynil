#pragma once

#include "dsp/Constants.h"
#include "dsp/util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

namespace detail {

/** First-order shelf (Orfanidis bilinear), one delay state. */
class FirstOrderShelf
{
public:
    void reset() noexcept
    {
        x1_ = 0.0;
        y1_ = 0.0;
    }

    void setLowShelf(double sampleRate, double fcHz, double gainDb) noexcept
    {
        const double A = dB2lin(gainDb);
        const double wc = 2.0 * kPi * fcHz / sampleRate;
        const double t = std::tan(wc * 0.5);
        const double rootA = std::sqrt(A);
        const double denom = 1.0 + t / rootA;
        b0_ = (1.0 + rootA * t) / denom;
        b1_ = (1.0 - rootA * t) / denom;
        a1_ = (1.0 - t / rootA) / denom;
    }

    void setHighShelf(double sampleRate, double fcHz, double gainDb) noexcept
    {
        const double A = dB2lin(gainDb);
        const double wc = 2.0 * kPi * fcHz / sampleRate;
        const double t = std::tan(wc * 0.5);
        const double rootA = std::sqrt(A);
        const double denom = 1.0 + rootA * t;
        b0_ = (rootA + t) / denom;
        b1_ = (rootA - t) / denom;
        a1_ = (t - rootA) / denom;
    }

    [[nodiscard]] double processSample(double x) noexcept
    {
        const double y = b0_ * x + b1_ * x1_ - a1_ * y1_;
        x1_ = x;
        y1_ = y;
        return y;
    }

private:
    double b0_ = 1.0;
    double b1_ = 0.0;
    double a1_ = 0.0;
    double x1_ = 0.0;
    double y1_ = 0.0;
};

} // namespace detail

/** RIAA mismatch tilt: LS 250 Hz +t/2 dB, HS 4 kHz −t/2 dB (SPEC §4.3). */
class TiltShelf
{
public:
    void reset() noexcept
    {
        low_.reset();
        high_.reset();
    }

    void setTiltDb(double sampleRate, double tiltDb) noexcept
    {
        low_.setLowShelf(sampleRate, kTiltLowShelfHz, tiltDb * 0.5);
        high_.setHighShelf(sampleRate, kTiltHighShelfHz, -tiltDb * 0.5);
    }

    [[nodiscard]] double processSample(double x) noexcept
    {
        return high_.processSample(low_.processSample(x));
    }

private:
    detail::FirstOrderShelf low_{};
    detail::FirstOrderShelf high_{};
};

} // namespace wg::dsp
