#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace wg::ui {

class ToggleSwitch final : public juce::Component
{
public:
    ToggleSwitch(juce::AudioParameterBool& param, juce::UndoManager* undo = nullptr);
    ~ToggleSwitch() override = default;

    void setDimmed(bool dim) noexcept
    {
        dimmed_ = dim;
        repaint();
    }

    void paint(juce::Graphics& g) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    juce::AudioParameterBool& param_;
    std::unique_ptr<juce::ParameterAttachment> attachment_;
    bool on_ = false;
    bool dimmed_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToggleSwitch)
};

} // namespace wg::ui
