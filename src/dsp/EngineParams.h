#pragma once

#include <cstdint>

namespace wg::dsp {

enum class AutoGainMode : std::uint8_t
{
    Off = 0,
    Static = 1,
    Dynamic = 2,
};

enum class TubeModel : std::uint8_t
{
    Koren = 0,
    Fast = 1,
};

/** POD snapshot of user-facing parameters (SPEC §7 defaults). Filled by ParamBridge in M2. */
struct EngineParams
{
    float amount = 0.5f;          // 0…1 (UI 0–100 %)
    float outGainDb = 0.0f;       // −24…+12 dB
    AutoGainMode autoGain = AutoGainMode::Static;
    bool bypass = false;
    float inGainDb = 0.0f;        // −24…+24 dB
    float mix = 1.0f;             // 0…1 (UI 0–100 %)
    float monoFreq = 150.0f;      // Hz
    float monoDepth = 1.0f;       // 0…1 (UI 0–100 %)
    float tracingTrim = 1.0f;     // 0…2 (UI 0–200 %)
    float riaaTilt = 0.5f;        // dB
    bool riaaNeumann = true;
    float cartFreq = 14000.0f;    // Hz
    float cartQ = 1.2f;
    float crosstalkDb = -30.0f;   // dB at 10 kHz
    float tubeTrim = 1.0f;        // 0…2 (UI 0–200 %)
    float tubeBias = 0.0f;        // −1…+1
    TubeModel tubeModel = TubeModel::Koren;
    float xfmrSat = 0.4f;         // 0…1 (UI 0–100 %)
    float xfmrHF = 20000.0f;      // Hz
    int osFactor = 4;             // 2, 4, or 8
    bool adaa = true;
    bool offlineHQ = false;
};

} // namespace wg::dsp
