#pragma once

#include "dsp/filters/SvfTpt.h"

#include <cmath>

namespace wg::dsp {

/** Linkwitz-Riley 4th order via two Butterworth SVF stages (Q = 1/√2). SPEC §4.2. */
class LinkwitzRiley4
{
public:
    static constexpr double kButterworthQ = 0.707106781186547524400844362104849039284835937688474036588;

    void reset() noexcept
    {
        lp1_.reset();
        lp2_.reset();
        hp1_.reset();
        hp2_.reset();
    }

    void setCutoff(double sampleRate, double cutoffHz) noexcept
    {
        lp1_.setParams(sampleRate, cutoffHz, kButterworthQ);
        lp2_.setParams(sampleRate, cutoffHz, kButterworthQ);
        hp1_.setParams(sampleRate, cutoffHz, kButterworthQ);
        hp2_.setParams(sampleRate, cutoffHz, kButterworthQ);
    }

    [[nodiscard]] double processLowpass(double x) noexcept
    {
        return lp2_.processLowpass2(lp1_.processLowpass2(x));
    }

    [[nodiscard]] double processHighpass(double x) noexcept
    {
        return hp2_.processHighpass2(hp1_.processHighpass2(x));
    }

    /** Complementary AP2: LR4 LP + LR4 HP (SPEC §4.2). */
    [[nodiscard]] double processAllpass2(double x) noexcept
    {
        return processLowpass(x) + processHighpass(x);
    }

private:
    SvfTpt lp1_{};
    SvfTpt lp2_{};
    SvfTpt hp1_{};
    SvfTpt hp2_{};
};

} // namespace wg::dsp
