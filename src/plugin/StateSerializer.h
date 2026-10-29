#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace wg::plugin {

/** APVTS ValueTree ↔ host state blob with stateVersion (SPEC §9.4). */
class StateSerializer
{
public:
    static constexpr int kCurrentVersion = 1;

    static void getState(juce::AudioProcessorValueTreeState& apvts,
                         juce::MemoryBlock& dest,
                         const juce::String& lastPresetName = {});

    static void setState(juce::AudioProcessorValueTreeState& apvts,
                         const void* data,
                         int sizeInBytes);
};

} // namespace wg::plugin
