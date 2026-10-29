#pragma once

#include "dsp/Constants.h"
#include "dsp/util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

namespace crosstalk_detail {

/** First-order high emphasis: ~0.3 at DC → 1.0 at HF, knee ~2 kHz (SPEC §4.4). */
class HighShelfEmphasis
{
public:
    void reset() noexcept { state_ = 0.0; }

    void setSampleRate(double sampleRate) noexcept
    {
        if (sampleRate <= 0.0)
        {
            g_ = 0.0;
            return;
        }
        constexpr double kFcHz = 2000.0;
        g_ = std::tan(kPi * kFcHz / sampleRate);
        g_ = g_ / (1.0 + g_);
    }

    [[nodiscard]] double processSample(double x) noexcept
    {
        const double lp = g_ * x + (1.0 - g_) * state_;
        state_ = lp;
        const double hp = x - lp;
        return 0.3 * x + 0.7 * hp;
    }

private:
    double g_ = 0.0;
    double state_ = 0.0;
};

} // namespace crosstalk_detail

/** Stereo crosstalk on Side (SPEC §4.4). */
class Crosstalk
{
public:
    void prepare(double fsi) noexcept { hs_.setSampleRate(fsi); }

    void reset() noexcept { hs_.reset(); }

    void setTargets(double crosstalkDb) noexcept
    {
        const double c = dB2lin(crosstalkDb);
        k_ = (c > -1.0) ? (2.0 * c / (1.0 + c)) : 0.0;
    }

    void processStereo(double& l, double& r) noexcept
    {
        const double M = 0.5 * (l + r);
        const double S = 0.5 * (l - r);
        const double sFiltered = S - k_ * hs_.processSample(S);
        l = M + sFiltered;
        r = M - sFiltered;
    }

    void processBlock(float* left, float* right, int numSamples) noexcept
    {
        for (int n = 0; n < numSamples; ++n)
        {
            double L = static_cast<double>(left[n]);
            double R = static_cast<double>(right[n]);
            processStereo(L, R);
            left[n] = static_cast<float>(L);
            right[n] = static_cast<float>(R);
        }
    }

private:
    double k_ = 0.0;
    crosstalk_detail::HighShelfEmphasis hs_{};
};

} // namespace wg::dsp
