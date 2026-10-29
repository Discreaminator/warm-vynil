#include "MacroMap.h"

#include "Constants.h"
#include "util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

InternalTargets map(const EngineParams& p, double aSmoothed, double fsi) noexcept
{
    const double a = clamp(aSmoothed, 0.0, 1.0);

    InternalTargets t{};

    t.m = static_cast<double>(p.monoDepth) * smoothstep(ramp(a, 0.0, 0.4));

    if (p.riaaNeumann)
        t.T4_eff = a * kRiaaT4Sec;
    else
        t.T4_eff = 0.0;

    t.s_tr = std::pow(a, 1.5) * static_cast<double>(p.tracingTrim);
    t.tiltDb = a * static_cast<double>(p.riaaTilt);

    const double cartT = smoothstep(ramp(a, 0.0, 0.6));
    const double frMax = 0.45 * fsi;
    const double fr0 = std::min(40000.0, frMax);
    const double fr1 = std::min(static_cast<double>(p.cartFreq), frMax);
    t.cartFr = loginterp(fr0, fr1, cartT);
    t.cartQ = lerp(0.707, static_cast<double>(p.cartQ), cartT);

    t.crosstalkDb = lerp(-60.0, static_cast<double>(p.crosstalkDb), a);

    t.tubeDriveDb = a * 18.0 * static_cast<double>(p.tubeTrim);
    t.kDyn = a * 0.15 * static_cast<double>(p.tubeTrim);

    t.xfmrSx = std::pow(a, 1.2) * static_cast<double>(p.xfmrSat);

    const double xfmrT = smoothstep(ramp(a, 0.0, 0.5));
    const double fc0 = std::min(40000.0, frMax);
    const double fc1 = std::min(static_cast<double>(p.xfmrHF), frMax);
    t.xfmrFc = loginterp(fc0, fc1, xfmrT);

    return t;
}

} // namespace wg::dsp
