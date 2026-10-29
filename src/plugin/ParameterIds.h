#pragma once

/** Stable APVTS parameter IDs (SPEC §7). Never rename after release. */
namespace wg::plugin::param {

inline constexpr const char* amount = "amount";
inline constexpr const char* outGain = "outGain";
inline constexpr const char* autoGain = "autoGain";
inline constexpr const char* bypass = "bypass";
inline constexpr const char* inGain = "inGain";
inline constexpr const char* mix = "mix";
inline constexpr const char* monoFreq = "monoFreq";
inline constexpr const char* monoDepth = "monoDepth";
inline constexpr const char* tracingTrim = "tracingTrim";
inline constexpr const char* riaaTilt = "riaaTilt";
inline constexpr const char* riaaNeumann = "riaaNeumann";
inline constexpr const char* cartFreq = "cartFreq";
inline constexpr const char* cartQ = "cartQ";
inline constexpr const char* crosstalk = "crosstalk";
inline constexpr const char* tubeTrim = "tubeTrim";
inline constexpr const char* tubeBias = "tubeBias";
inline constexpr const char* tubeModel = "tubeModel";
inline constexpr const char* xfmrSat = "xfmrSat";
inline constexpr const char* xfmrHF = "xfmrHF";
inline constexpr const char* osFactor = "osFactor";
inline constexpr const char* adaa = "adaa";
inline constexpr const char* offlineHQ = "offlineHQ";

inline constexpr int kStateVersion = 1;

} // namespace wg::plugin::param
