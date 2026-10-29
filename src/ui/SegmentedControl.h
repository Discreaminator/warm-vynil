#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace wg::ui {

class SegmentedControl final : public juce::Component
{
public:
    SegmentedControl(juce::AudioParameterChoice& param, juce::UndoManager* undo = nullptr);
    ~SegmentedControl() override = default;

    void setDimmed(bool dim) noexcept
    {
        dimmed_ = dim;
        repaint();
    }

    void paint(juce::Graphics& g) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    juce::AudioParameterChoice& param_;
    std::unique_ptr<juce::ParameterAttachment> attachment_;
    int index_ = 0;
    bool dimmed_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SegmentedControl)
};

} // namespace wg::ui
