#pragma once

#include "dsp/Constants.h"

#include <cmath>

namespace wg::dsp {

/** First-order TPT lowpass / highpass (SPEC §4.6 transformer paths). */
class OnePole
{
public:
    void reset() noexcept { state_ = 0.0; }

    void setCutoff(double sampleRate, double cutoffHz) noexcept
    {
        if (sampleRate <= 0.0)
        {
            g_ = 0.0;
            G_ = 0.0;
            return;
        }
        const double fc = std::max(cutoffHz, 1.0e-6);
        g_ = std::tan(kPi * fc / sampleRate);
        G_ = g_ / (1.0 + g_);
    }

    [[nodiscard]] double processLowpass(double x) noexcept
    {
        const double v = G_ * (x - state_);
        const double lp = v + state_;
        state_ = lp + v;
        return lp;
    }

    [[nodiscard]] double processHighpass(double x) noexcept
    {
        const double v = G_ * (x - state_);
        const double hp = (x - state_) - v;
        state_ = state_ + v + v;
        return hp;
    }

private:
    double g_ = 0.0;
    double G_ = 0.0;
    double state_ = 0.0;
};

} // namespace wg::dsp
