#include "ToggleSwitch.h"
#include "Theme.h"

namespace wg::ui {

ToggleSwitch::ToggleSwitch(juce::AudioParameterBool& param, juce::UndoManager* undo)
    : param_(param)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setTitle(param.getName(64));
    setSize(28, 15);
    attachment_ = std::make_unique<juce::ParameterAttachment>(
        param_,
        [this](float) {
            on_ = param_.get();
            repaint();
        },
        undo);
    attachment_->sendInitialUpdate();
}

void ToggleSwitch::paint(juce::Graphics& g)
{
    if (dimmed_ || ! isEnabled())
        g.beginTransparencyLayer(dimmed_ ? 0.6f : 0.4f);

    auto r = juce::Rectangle<float>(0, 0, 28.0f, 15.0f);
    g.setColour(on_ ? theme::c(theme::accent) : theme::c(theme::track));
    g.fillRoundedRectangle(r, 7.5f);

    const float knobR = 5.3f;
    const float kx = on_ ? (28.0f - 2.0f - knobR) : (2.0f + knobR);
    const float ky = 7.5f;
    g.setColour(on_ ? theme::c(theme::bg) : theme::c(theme::textSecondary));
    g.fillEllipse(kx - knobR, ky - knobR, knobR * 2.0f, knobR * 2.0f);

    if (dimmed_ || ! isEnabled())
        g.endTransparencyLayer();
}

void ToggleSwitch::mouseUp(const juce::MouseEvent&)
{
    if (! isEnabled())
        return;
    attachment_->setValueAsCompleteGesture(on_ ? 0.0f : 1.0f);
}

} // namespace wg::ui
