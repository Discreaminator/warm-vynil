#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace wg::plugin {

/** Builds APVTS layout from SPEC §7 (IDs, ranges, defaults). */
[[nodiscard]] juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace wg::plugin
