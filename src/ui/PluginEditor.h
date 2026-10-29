#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace wg::plugin {
class PluginProcessor;
}

namespace wg::ui {

class PluginEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor(wg::plugin::PluginProcessor& processor);
    ~PluginEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};

} // namespace wg::ui
