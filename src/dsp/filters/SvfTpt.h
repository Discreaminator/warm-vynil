#pragma once

#include "dsp/Constants.h"

#include <cmath>

namespace wg::dsp {

/** Trapezoidal SVF (Andrew Simper / Cytomic TPT). SPEC §4.2, §4.4, §4.6. */
class SvfTpt
{
public:
    void reset() noexcept
    {
        ic1eq_ = 0.0;
        ic2eq_ = 0.0;
        ic1eqR_ = 0.0;
        ic2eqR_ = 0.0;
    }

    void setParams(double sampleRate, double cutoffHz, double q) noexcept
    {
        if (sampleRate <= 0.0)
        {
            a1_ = a2_ = a3_ = k_ = 0.0;
            return;
        }
        const double fc = std::max(cutoffHz, 1.0e-6);
        const double g = std::tan(kPi * fc / sampleRate);
        k_ = 1.0 / std::max(q, 1.0e-6);
        a1_ = 1.0 / (1.0 + g * (g + k_));
        a2_ = g * a1_;
        a3_ = g * a2_;
    }

    [[nodiscard]] double processLowpass2(double x) noexcept
    {
        const double v3 = x - ic2eq_;
        const double v1 = a1_ * ic1eq_ + a2_ * v3;
        const double v2 = ic2eq_ + a2_ * ic1eq_ + a3_ * v3;
        ic1eq_ = 2.0 * v1 - ic1eq_;
        ic2eq_ = 2.0 * v2 - ic2eq_;
        return v2;
    }

    [[nodiscard]] double processHighpass2(double x) noexcept
    {
        const double v3 = x - ic2eq_;
        const double v1 = a1_ * ic1eq_ + a2_ * v3;
        const double v2 = ic2eq_ + a2_ * ic1eq_ + a3_ * v3;
        ic1eq_ = 2.0 * v1 - ic1eq_;
        ic2eq_ = 2.0 * v2 - ic2eq_;
        return x - k_ * v1 - v2;
    }

    /** LR4 allpass sum component: LP2 + HP2 at same fc/Q (SPEC §4.2). */
    [[nodiscard]] double processAllpass2(double x) noexcept
    {
        const double v3 = x - ic2eq_;
        const double v1 = a1_ * ic1eq_ + a2_ * v3;
        const double v2 = ic2eq_ + a2_ * ic1eq_ + a3_ * v3;
        ic1eq_ = 2.0 * v1 - ic1eq_;
        ic2eq_ = 2.0 * v2 - ic2eq_;
        const double hp = x - k_ * v1 - v2;
        return v2 + hp;
    }

    /** First-order shelf stub — use TiltShelf for RIAA mismatch. */
    [[nodiscard]] double processLowShelfStub(double x, double /*gainLinear*/) noexcept
    {
        return processLowpass2(x);
    }

    [[nodiscard]] double processHighShelfStub(double x, double /*gainLinear*/) noexcept
    {
        return processHighpass2(x);
    }

    void processStereoLowpass2(double& l, double& r) noexcept
    {
        l = processLowpass2(l);
        r = processLowpass2OnRight(r);
    }

    void processStereoHighpass2(double& l, double& r) noexcept
    {
        l = processHighpass2(l);
        r = processHighpass2OnRight(r);
    }

private:
    [[nodiscard]] double processLowpass2OnRight(double x) noexcept
    {
        const double v3 = x - ic2eqR_;
        const double v1 = a1_ * ic1eqR_ + a2_ * v3;
        const double v2 = ic2eqR_ + a2_ * ic1eqR_ + a3_ * v3;
        ic1eqR_ = 2.0 * v1 - ic1eqR_;
        ic2eqR_ = 2.0 * v2 - ic2eqR_;
        return v2;
    }

    [[nodiscard]] double processHighpass2OnRight(double x) noexcept
    {
        const double v3 = x - ic2eqR_;
        const double v1 = a1_ * ic1eqR_ + a2_ * v3;
        const double v2 = ic2eqR_ + a2_ * ic1eqR_ + a3_ * v3;
        ic1eqR_ = 2.0 * v1 - ic1eqR_;
        ic2eqR_ = 2.0 * v2 - ic2eqR_;
        return x - k_ * v1 - v2;
    }

    double k_ = 0.0;
    double a1_ = 0.0;
    double a2_ = 0.0;
    double a3_ = 0.0;
    double ic1eq_ = 0.0;
    double ic2eq_ = 0.0;
    double ic1eqR_ = 0.0;
    double ic2eqR_ = 0.0;
};

} // namespace wg::dsp
