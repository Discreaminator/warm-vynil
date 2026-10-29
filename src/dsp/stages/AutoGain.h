#pragma once

#include "dsp/EngineParams.h"
#include "dsp/filters/KWeighting.h"

namespace wg::dsp {

/** Auto-gain Off / Static / Dynamic (SPEC §4.7). */
class AutoGain
{
public:
    void prepare(double sampleRate, int numChannels) noexcept;

    void reset() noexcept;

    void setMode(AutoGainMode mode) noexcept { mode_ = mode; }

    void setAmount(double amount0to1) noexcept { amount_ = amount0to1; }

    /** `inSample` is pre-effect input; `wetSample` is processed wet. Returns gain to apply to wet. */
    [[nodiscard]] double processSample(double inSample, double wetSample) noexcept;

    [[nodiscard]] double currentGainLinear() const noexcept { return gainLinear_; }

private:
    [[nodiscard]] double staticGainDb() const noexcept;

    void updateGate(double inWeightedMs) noexcept;

    double fs_ = 48000.0;
    int numChannels_ = 2;
    AutoGainMode mode_ = AutoGainMode::Static;
    double amount_ = 0.5;

    double gainLinear_ = 1.0;
    double gainDbSmooth_ = 0.0;

    double inMs_ = 0.0;
    double outMs_ = 0.0;
    double msCoeff_ = 0.0;
    double gainSmoothCoeff_ = 0.0;

    bool gateFrozen_ = false;

    KWeighting kIn_{};
    KWeighting kOut_{};
};

} // namespace wg::dsp
