#include "MacroMap.h"

#include "Constants.h"
#include "util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

namespace {
// SPEC §5 lists 18 dB / 0.15 k_dyn; M1 used 10 / 0.12 for T5 headroom.
// Tone pass: richer harmonics while staying in T4 0.5–3 % / T5 < −80 dB.
constexpr double kTubeDriveDbAtFull = 13.0;
constexpr double kTubeKDynAtFull = 0.14;
// Extra warmth bias on top of user riaaTilt (dB at Amount=1).
constexpr double kTiltBiasDb = 0.35;
constexpr double kTiltUserScale = 1.25;
// Darker top: pull cart / xfmr corners slightly below user targets.
constexpr double kCartFrDark = 0.78;
constexpr double kXfmrFcDark = 0.88;
constexpr double kCartQBoost = 1.18;
} // namespace

InternalTargets map(const EngineParams& p, double aSmoothed, double fsi) noexcept
{
    const double a = clamp(aSmoothed, 0.0, 1.0);

    InternalTargets t{};

    t.m = static_cast<double>(p.monoDepth) * smoothstep(ramp(a, 0.0, 0.4));

    if (p.riaaNeumann)
        t.T4_eff = a * kRiaaT4Sec;
    else
        t.T4_eff = 0.0;

    // SPEC §5: s_tr = a^1.5 · tracingTrim — slightly softer exponent for earlier richness.
    t.s_tr = std::pow(a, 1.35) * static_cast<double>(p.tracingTrim);

    // Stronger mismatch tilt: darker air + more body/upper-mid relative lift as Amount rises.
    t.tiltDb = a * (kTiltBiasDb + kTiltUserScale * static_cast<double>(p.riaaTilt));

    // Cartridge engages earlier; Fr pulled down (darker HF); Q pushed up (presence peak).
    const double cartT = smoothstep(ramp(a, 0.0, 0.45));
    const double frMax = 0.45 * fsi;
    const double fr0 = std::min(40000.0, frMax);
    const double fr1 = std::min(static_cast<double>(p.cartFreq) * kCartFrDark, frMax);
    t.cartFr = loginterp(fr0, fr1, cartT);
    const double qTarget = std::min(3.0, static_cast<double>(p.cartQ) * kCartQBoost);
    t.cartQ = lerp(0.707, qTarget, cartT);

    t.crosstalkDb = lerp(-60.0, static_cast<double>(p.crosstalkDb), a);

    t.tubeDriveDb = a * kTubeDriveDbAtFull * static_cast<double>(p.tubeTrim);
    t.kDyn = a * kTubeKDynAtFull * static_cast<double>(p.tubeTrim);

    t.xfmrSx = std::pow(a, 1.15) * static_cast<double>(p.xfmrSat);

    const double xfmrT = smoothstep(ramp(a, 0.0, 0.35));
    const double fc0 = std::min(40000.0, frMax);
    const double fc1 = std::min(static_cast<double>(p.xfmrHF) * kXfmrFcDark, frMax);
    t.xfmrFc = loginterp(fc0, fc1, xfmrT);

    return t;
}

} // namespace wg::dsp
