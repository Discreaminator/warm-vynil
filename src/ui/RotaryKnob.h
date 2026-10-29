#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace wg::ui {

/** Small ring knob used in footer / advanced cards — SPEC §9.1.5 / §9.1.6. */
class RotaryKnob final : public juce::Component
{
public:
    enum class Style
    {
        OutGain, // r=16 arc, face 10.5
        Small    // r=13.5 arc, for cards
    };

    RotaryKnob(juce::RangedAudioParameter& param,
               Style style,
               std::function<juce::String(float)> formatter,
               juce::UndoManager* undo = nullptr);
    ~RotaryKnob() override = default;

    void setCaption(juce::String caption) { caption_ = std::move(caption); }
    void setShowCaption(bool show) noexcept { showCaption_ = show; }
    void setDimmed(bool dim) noexcept
    {
        dimmed_ = dim;
        repaint();
    }
    void setEnabledUi(bool en) noexcept
    {
        setEnabled(en);
        repaint();
    }

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    juce::String currentText() const;

private:
    void setNorm(float n01, bool complete);

    juce::RangedAudioParameter& param_;
    std::unique_ptr<juce::ParameterAttachment> attachment_;
    std::function<juce::String(float)> formatter_;
    Style style_;
    juce::String caption_;
    bool showCaption_ = true;
    float value01_ = 0.0f;
    bool dimmed_ = false;
    bool dragging_ = false;
    bool hover_ = false;
    float dragStartNorm_ = 0.0f;
    float dragStartY_ = 0.0f;
    juce::Point<int> mouseHidePos_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RotaryKnob)
};

} // namespace wg::ui
