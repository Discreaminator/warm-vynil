#pragma once

#include "dsp/EngineParams.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace wg::plugin {

/**
 * Caches APVTS raw atomics once; snapshots EngineParams at the start of each block (SPEC §8.1).
 * Does not touch Engine — processor calls Engine::setTargets with the snapshot.
 */
class ParamBridge
{
public:
    explicit ParamBridge(juce::AudioProcessorValueTreeState& apvts);

    /** Bind / re-bind raw parameter pointers (safe on message thread). */
    void bind(juce::AudioProcessorValueTreeState& apvts);

    /** RT-safe: read atomics into POD once per processBlock. */
    [[nodiscard]] wg::dsp::EngineParams snapshot() const noexcept;

    /** True if OS / offlineHQ / ADAA differ from last applied structural targets. */
    [[nodiscard]] bool structuralChanged(const wg::dsp::EngineParams& next) const noexcept;

    void noteStructuralApplied(const wg::dsp::EngineParams& applied) noexcept;

private:
    std::atomic<float>* amount_ = nullptr;
    std::atomic<float>* outGain_ = nullptr;
    std::atomic<float>* autoGain_ = nullptr;
    std::atomic<float>* bypass_ = nullptr;
    std::atomic<float>* inGain_ = nullptr;
    std::atomic<float>* mix_ = nullptr;
    std::atomic<float>* monoFreq_ = nullptr;
    std::atomic<float>* monoDepth_ = nullptr;
    std::atomic<float>* tracingTrim_ = nullptr;
    std::atomic<float>* riaaTilt_ = nullptr;
    std::atomic<float>* riaaNeumann_ = nullptr;
    std::atomic<float>* cartFreq_ = nullptr;
    std::atomic<float>* cartQ_ = nullptr;
    std::atomic<float>* crosstalk_ = nullptr;
    std::atomic<float>* tubeTrim_ = nullptr;
    std::atomic<float>* tubeBias_ = nullptr;
    std::atomic<float>* tubeModel_ = nullptr;
    std::atomic<float>* xfmrSat_ = nullptr;
    std::atomic<float>* xfmrHF_ = nullptr;
    std::atomic<float>* osFactor_ = nullptr;
    std::atomic<float>* adaa_ = nullptr;
    std::atomic<float>* offlineHQ_ = nullptr;

    int lastOsFactor_ = 4;
    bool lastAdaa_ = true;
    bool lastOfflineHQ_ = false;
};

} // namespace wg::plugin
