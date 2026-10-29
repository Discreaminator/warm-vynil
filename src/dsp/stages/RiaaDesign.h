#pragma once

#include "dsp/filters/Biquad.h"

namespace wg::dsp {

/** LS-fit RIAA play biquad + Neumann pole design (SPEC §4.3). Cached by `fsi`. */
[[nodiscard]] BiquadCoeffs designPlayBiquad(double fsi) noexcept;

struct NeumannCoeffs
{
    double b0 = 1.0;
    double b1 = 0.0;
    double a1 = 0.0;
    bool bypass = true;
};

[[nodiscard]] NeumannCoeffs designNeumann(double fsi, double T4Sec) noexcept;

/** Reflect biquad zeros outside unit circle; scale gain (SPEC §4.3). */
[[nodiscard]] BiquadCoeffs forceMinimumPhase(BiquadCoeffs c) noexcept;

} // namespace wg::dsp
