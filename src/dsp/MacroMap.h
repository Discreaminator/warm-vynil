#pragma once

#include "EngineParams.h"

namespace wg::dsp {

/** Effective internal targets after Amount macro mapping (SPEC §5). */
struct InternalTargets
{
    double m = 0.0;
    double T4_eff = 0.0;
    double s_tr = 0.0;
    double tiltDb = 0.0;
    double cartFr = 40000.0;
    double cartQ = 0.707;
    double crosstalkDb = -60.0;
    double tubeDriveDb = 0.0;
    double kDyn = 0.0;
    double xfmrSx = 0.0;
    double xfmrFc = 40000.0;
};

/** Map smoothed Amount `aSmoothed` ∈ [0,1] and user params to internal targets. `fsi` = internal rate (Hz). */
[[nodiscard]] InternalTargets map(const EngineParams& p, double aSmoothed, double fsi) noexcept;

} // namespace wg::dsp
