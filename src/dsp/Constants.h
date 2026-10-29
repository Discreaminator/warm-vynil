#pragma once

namespace wg::dsp {

// --- RIAA time constants (IEC 60098 / SPEC §4.3, Appendix A) ---
inline constexpr double kRiaaT1Sec = 3180.0e-6; // 3180 µs
inline constexpr double kRiaaT2Sec = 318.0e-6;  // 318 µs
inline constexpr double kRiaaT3Sec = 75.0e-6;   // 75 µs
inline constexpr double kRiaaT4Sec = 3.18e-6;   // Neumann optional pole ≈ 50 kHz

// --- DC blocker (SPEC §4.1, Appendix A) ---
inline constexpr double kDcBlockHz = 5.0;

// --- Transformer / misc filter corners (SPEC §4.6) ---
inline constexpr double kXfmrLfSatHz = 25.0;
inline constexpr double kXfmrHpfHz = 6.0;

// --- Tilt mismatch shelves (SPEC §4.3) ---
inline constexpr double kTiltLowShelfHz = 250.0;
inline constexpr double kTiltHighShelfHz = 4000.0;

// --- Parameter coefficient update grid (SPEC §8.3) ---
inline constexpr int kCoeffUpdateGrid = 32;

// --- Default parameter smoothing times, ms (SPEC §7, §8.2) ---
inline constexpr double kSmoothAmountMs = 50.0;
inline constexpr double kSmoothGainMs = 20.0;
inline constexpr double kSmoothBypassMs = 20.0;
inline constexpr double kSmoothAutoGainModeMs = 50.0;
inline constexpr double kSmoothDefaultAdvancedMs = 50.0;
inline constexpr double kSmoothTubeBiasMs = 100.0;

// --- Auto-gain dynamic (SPEC §4.7) ---
inline constexpr double kAutoGainDynamicTauMs = 400.0;
inline constexpr double kAutoGainSmoothTauMs = 1500.0;
inline constexpr double kAutoGainGateLufs = -50.0;

// --- Engine bypass hysteresis (SPEC §4.7) ---
inline constexpr double kEngineBypassOff = 0.003;
inline constexpr double kEngineBypassOn = 0.007;

// --- Tracing polynomial (SPEC §4.3) ---
inline constexpr double kTracingC2Scale = 0.15;
inline constexpr double kTracingC3Scale = -0.03;

// --- Numerical epsilons ---
inline constexpr double kAdaaDeltaEps = 1.0e-6;
inline constexpr double kNeumannBypassTSec = 1.0e-9;

inline constexpr double kPi = 3.141592653589793238462643383279502884;

} // namespace wg::dsp
