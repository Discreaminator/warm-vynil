#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace wg::ui {

/** Large «Глубина» / amount knob — SPEC §9.1.4. */
class MacroKnob final : public juce::Component,
                        private juce::Timer
{
public:
    MacroKnob(juce::RangedAudioParameter& param, juce::UndoManager* undo = nullptr);
    ~MacroKnob() override;

    void setDimmed(bool dim) noexcept
    {
        dimmed_ = dim;
        repaint();
    }

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool hitTest(int x, int y) override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    void timerCallback() override;
    void setNorm(float n01, bool notifyHost);
    float getNorm() const;
    float getDenorm() const;

    juce::RangedAudioParameter& param_;
    std::unique_ptr<juce::ParameterAttachment> attachment_;
    float value01_ = 0.5f;
    bool dimmed_ = false;
    bool dragging_ = false;
    bool hover_ = false;
    float dragStartNorm_ = 0.0f;
    juce::Point<float> dragStartScreen_;
    juce::Point<int> mouseHidePos_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroKnob)
};

} // namespace wg::ui
