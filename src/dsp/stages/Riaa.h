#pragma once

#include "RiaaDesign.h"
#include "dsp/filters/Biquad.h"
#include "dsp/filters/TiltShelf.h"

namespace wg::dsp {

/** RIAA pre (invert play) → Neumann → de (play) → tilt (SPEC §4.3). Tracing is external. */
class Riaa
{
public:
    void prepare(double fsi) noexcept;

    void reset() noexcept;

    void setTargets(double tiltDb, double T4Sec) noexcept;

    [[nodiscard]] double processPreSample(double x) noexcept;

    [[nodiscard]] double processNeumannSample(double x) noexcept;

    [[nodiscard]] double processDeSample(double x) noexcept;

    [[nodiscard]] double processTiltSample(double x) noexcept;

    [[nodiscard]] double processPostChainSample(double xAfterTracing) noexcept;

private:
    void updateNeumann() noexcept;

    double fsi_ = 48000.0;
    double tiltDb_ = 0.0;
    double T4Sec_ = 0.0;

    Biquad pre_{};
    Biquad de_{};
    TiltShelf tilt_{};

    NeumannCoeffs neumann_{};
    double neumannX1_ = 0.0;
    double neumannY1_ = 0.0;
};

} // namespace wg::dsp
