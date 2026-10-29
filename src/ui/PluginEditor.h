#pragma once

#include "AdvancedPanel.h"
#include "MacroKnob.h"
#include "PresetBar.h"
#include "RotaryKnob.h"
#include "SegmentedControl.h"
#include "VinylRecord.h"
#include "WarmLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace wg::plugin {
class PluginProcessor;
}

namespace wg::ui {

class PluginEditor final : public juce::AudioProcessorEditor,
                           private juce::AudioProcessorValueTreeState::Listener,
                           private juce::Timer
{
public:
    explicit PluginEditor(wg::plugin::PluginProcessor& processor);
    ~PluginEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setUiScale(float scale);
    float getUiScale() const noexcept { return uiScale_; }
    void setAdvancedOpen(bool open);
    bool isAdvancedOpen() const noexcept { return advancedOpen_; }

private:
    class Content final : public juce::Component
    {
    public:
        explicit Content(PluginEditor& owner);
        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseUp(const juce::MouseEvent& e) override;

        PluginEditor& owner;
        PresetBar presetBar;
        VinylRecord vinyl;
        MacroKnob macro;
        RotaryKnob outGain;
        SegmentedControl autoGain;
        AdvancedPanel advanced;
    };

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void timerCallback() override;
    void applyWindowSize();
    void updateBypassDim();

    wg::plugin::PluginProcessor& processor_;
    WarmLookAndFeel lnf_;
    Content content_;

    float uiScale_ = 1.0f;
    bool advancedOpen_ = false;
    bool bypassOn_ = false;

    std::unique_ptr<juce::ParameterAttachment> amountWatch_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};

} // namespace wg::ui
