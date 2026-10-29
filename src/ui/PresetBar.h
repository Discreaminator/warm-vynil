#pragma once

#include "plugin/PresetManager.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace wg::plugin {
class PluginProcessor;
}

namespace wg::ui {

class PresetBar final : public juce::Component,
                        private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit PresetBar(wg::plugin::PluginProcessor& processor);
    ~PresetBar() override;

    void setDimmed(bool dim) noexcept
    {
        dimmed_ = dim;
        repaint();
    }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    void parameterChanged(const juce::String&, float) override;
    void refreshName();
    void showPresetMenu();
    void prevPreset();
    void nextPreset();
    void savePreset();
    void requestScale(float scale);

    wg::plugin::PluginProcessor& processor_;
    juce::String presetName_;
    bool dimmed_ = false;
    bool bypassOn_ = false;

    juce::Rectangle<float> prevBounds_, nextBounds_, nameBounds_, menuBounds_, saveBounds_, bypassBounds_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBar)
};

} // namespace wg::ui
