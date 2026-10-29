#pragma once

#include "dsp/Constants.h"
#include "dsp/filters/OnePole.h"
#include "dsp/filters/SvfTpt.h"
#include "dsp/util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

/** Output transformer LF sat + HPF + HF LP2 (SPEC §4.6). */
class Transformer
{
public:
    static constexpr double kLp2Q = 0.65;

    void prepare(double fsi) noexcept
    {
        fsi_ = fsi;
        lfPole_.setCutoff(fsi, kXfmrLfSatHz);
        hpf_.setCutoff(fsi, kXfmrHpfHz);
        updateLp();
    }

    void reset() noexcept
    {
        lfPole_.reset();
        hpf_.reset();
        lp2_.reset();
    }

    void setTargets(double satSx, double fcHz) noexcept
    {
        sx_ = clamp(satSx, 0.0, 1.0);
        fc_ = fcHz;
        updateLp();
    }

    [[nodiscard]] double processSample(double x) noexcept
    {
        const double phi = lfPole_.processLowpass(x);
        const double L = dB2lin(6.0 - 18.0 * sx_);
        const double sat = L * std::tanh(phi / L);
        double y = x - (phi - sat);
        y = hpf_.processHighpass(y);
        y = lp2_.processLowpass2(y);
        return y;
    }

private:
    void updateLp() noexcept
    {
        if (fsi_ <= 0.0)
            return;
        const double fc = clamp(fc_, 1.0, 0.45 * fsi_);
        lp2_.setParams(fsi_, fc, kLp2Q);
    }

    double fsi_ = 48000.0;
    double sx_ = 0.0;
    double fc_ = 20000.0;
    OnePole lfPole_{};
    OnePole hpf_{};
    SvfTpt lp2_{};
};

} // namespace wg::dsp
