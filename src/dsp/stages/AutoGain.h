#pragma once

#include "dsp/EngineParams.h"
#include "dsp/filters/KWeighting.h"

namespace wg::dsp {

/** Static table + dynamic K-weighted auto-gain (SPEC §4.7). */
class AutoGain
{
public:
    static constexpr int kStaticTableSize = 11;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setMode(AutoGainMode mode) noexcept { mode_ = mode; }
    void setAmount(double amount01) noexcept { amount_ = amount01; }

    /** Static compensation for current amount (linear gain). */
    [[nodiscard]] double staticGainLinear(double amount01) const noexcept;

    /**
     * Process one sample of wet (and matching pre-InGain dry for dynamic).
     * Returns gain to apply to wet (linear).
     */
    [[nodiscard]] double processGain(double dryPreInGain, double wet) noexcept;

    /** Calibration table accessor (dB), size kStaticTableSize. */
    [[nodiscard]] static const double* staticTableDb() noexcept;

private:
    double sampleRate_ = 48000.0;
    AutoGainMode mode_ = AutoGainMode::Static;
    double amount_ = 0.5;

    KWeighting kIn_{};
    KWeighting kOut_{};
    double pin_ = 1.0e-12;
    double pout_ = 1.0e-12;
    double dynGainDb_ = 0.0;
    double msCoeff_ = 0.0;
    double smoothCoeff_ = 0.0;
};

} // namespace wg::dsp
