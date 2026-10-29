#include "RotaryKnob.h"
#include "Fonts.h"
#include "Theme.h"
#include "UiHelpers.h"

namespace wg::ui {

RotaryKnob::RotaryKnob(juce::RangedAudioParameter& param,
                       Style style,
                       std::function<juce::String(float)> formatter,
                       juce::UndoManager* undo)
    : param_(param)
    , formatter_(std::move(formatter))
    , style_(style)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setTitle(param.getName(64));
    attachment_ = std::make_unique<juce::ParameterAttachment>(
        param_,
        [this](float) {
            value01_ = param_.getValue();
            repaint();
        },
        undo);
    attachment_->sendInitialUpdate();
}

juce::String RotaryKnob::currentText() const
{
    if (formatter_)
        return formatter_(param_.convertFrom0to1(value01_));
    return param_.getText(value01_, 8);
}

void RotaryKnob::setNorm(float n01, bool complete)
{
    value01_ = juce::jlimit(0.0f, 1.0f, n01);
    const float den = param_.convertFrom0to1(value01_);
    if (attachment_ == nullptr)
        return;
    if (complete)
        attachment_->setValueAsCompleteGesture(den);
    else
        attachment_->setValueAsPartOfGesture(den);
    repaint();
}

void RotaryKnob::paint(juce::Graphics& g)
{
    if (dimmed_ || ! isEnabled())
        g.beginTransparencyLayer(dimmed_ ? 0.6f : 0.4f);

    const auto b = getLocalBounds().toFloat();
    const float cx = b.getCentreX();
    const float cy = style_ == Style::OutGain ? b.getCentreY() : b.getY() + 13.5f;
    const float arcR = style_ == Style::OutGain ? 16.0f : 13.5f;
    const float faceR = style_ == Style::OutGain ? 10.5f : 8.5f;
    const float thick = style_ == Style::OutGain ? 3.0f : 2.5f;
    const float start = -135.0f;
    const float sweep = 270.0f;

    const juce::Colour accent = hover_ ? theme::c(theme::accentHover) : theme::c(theme::accent);

    strokeArc(g, cx, cy, arcR, start, start + sweep, thick, theme::c(theme::track));
    if (value01_ > 0.0001f)
        strokeArc(g, cx, cy, arcR, start, start + sweep * value01_, thick, accent);

    g.setColour(dragging_ ? theme::c(theme::pressedFace) : theme::c(theme::surface));
    g.fillEllipse(cx - faceR, cy - faceR, faceR * 2.0f, faceR * 2.0f);
    g.setColour(theme::c(theme::line));
    g.drawEllipse(cx - faceR, cy - faceR, faceR * 2.0f, faceR * 2.0f, 1.0f);

    const float pdeg = start + sweep * value01_;
    const float pin0 = faceR * 0.25f;
    const float pin1 = faceR * 0.78f;
    const auto a = pointOnCircle(cx, cy, pin0, pdeg);
    const auto bpt = pointOnCircle(cx, cy, pin1, pdeg);
    g.setColour(theme::c(theme::textPrimary));
    g.drawLine(a.x, a.y, bpt.x, bpt.y, style_ == Style::OutGain ? 1.6f : 1.4f);

    if (showCaption_ && caption_.isNotEmpty())
    {
        auto& fonts = Fonts::get();
        g.setFont(fonts.inter(10.0f, 500));
        g.setColour(theme::c(theme::textLabel));
        g.drawText(caption_, juce::Rectangle<float>(b.getX(), cy + faceR + 6.0f, b.getWidth(), 12.0f), juce::Justification::centred, false);
        g.setFont(fonts.geistMono(9.0f, 400));
        g.setColour(theme::c(theme::textSecondary));
        g.drawText(currentText(), juce::Rectangle<float>(b.getX(), cy + faceR + 18.0f, b.getWidth(), 12.0f), juce::Justification::centred, false);
    }

    if (dimmed_ || ! isEnabled())
        g.endTransparencyLayer();
}

void RotaryKnob::mouseDown(const juce::MouseEvent& e)
{
    if (! isEnabled())
        return;
    if (e.mods.isAltDown())
    {
        setNorm(param_.getDefaultValue(), true);
        return;
    }
    dragging_ = true;
    dragStartNorm_ = value01_;
    dragStartY_ = e.position.y;
    mouseHidePos_ = e.getScreenPosition();
    juce::Desktop::getInstance().getMainMouseSource().enableUnboundedMouseMovement(true, true);
    attachment_->beginGesture();
    repaint();
}

void RotaryKnob::mouseDrag(const juce::MouseEvent& e)
{
    if (! dragging_)
        return;
    const float pixels = style_ == Style::OutGain ? 200.0f : 150.0f;
    const float dy = dragStartY_ - e.position.y;
    const bool fine = e.mods.isShiftDown() || e.mods.isCommandDown() || e.mods.isCtrlDown();
    const float sens = fine ? 0.1f : 1.0f;
    setNorm(dragStartNorm_ + (dy / pixels) * sens, false);
}

void RotaryKnob::mouseUp(const juce::MouseEvent&)
{
    if (! dragging_)
        return;
    dragging_ = false;
    juce::Desktop::getInstance().getMainMouseSource().enableUnboundedMouseMovement(false);
    juce::Desktop::setMousePosition(mouseHidePos_);
    attachment_->endGesture();
    repaint();
}

void RotaryKnob::mouseDoubleClick(const juce::MouseEvent&)
{
    if (isEnabled())
        setNorm(param_.getDefaultValue(), true);
}

void RotaryKnob::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (! isEnabled())
        return;
    const bool fine = e.mods.isShiftDown() || e.mods.isCommandDown() || e.mods.isCtrlDown();
    const float step = fine ? 0.001f : 0.01f;
    setNorm(value01_ + (wheel.deltaY >= 0 ? step : -step), true);
}

} // namespace wg::ui
