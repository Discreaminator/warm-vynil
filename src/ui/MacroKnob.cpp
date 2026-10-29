#include "MacroKnob.h"
#include "Fonts.h"
#include "Strings.h"
#include "Theme.h"
#include "UiHelpers.h"

namespace wg::ui {

MacroKnob::MacroKnob(juce::RangedAudioParameter& param, juce::UndoManager* undo)
    : param_(param)
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setTitle(param.getName(64));
    setDescription(str::depth());

    attachment_ = std::make_unique<juce::ParameterAttachment>(
        param_,
        [this](float) {
            value01_ = param_.getValue();
            repaint();
        },
        undo);

    attachment_->sendInitialUpdate();
    startTimerHz(30); // hover transition polling not needed; kept for pressed fade if extended
}

MacroKnob::~MacroKnob()
{
    stopTimer();
}

void MacroKnob::timerCallback() {}

float MacroKnob::getNorm() const
{
    return value01_;
}

float MacroKnob::getDenorm() const
{
    return param_.convertFrom0to1(value01_);
}

void MacroKnob::setNorm(float n01, bool notifyHost)
{
    value01_ = juce::jlimit(0.0f, 1.0f, n01);
    if (notifyHost && attachment_ != nullptr)
        attachment_->setValueAsCompleteGesture(param_.convertFrom0to1(value01_));
    repaint();
}

void MacroKnob::resized() {}

bool MacroKnob::hitTest(int x, int y)
{
    const float dx = (float) x - theme::macroCx;
    const float dy = (float) y - theme::macroCy;
    return (dx * dx + dy * dy) <= (theme::macroHoverR * theme::macroHoverR);
}

void MacroKnob::paint(juce::Graphics& g)
{
    if (dimmed_)
        g.beginTransparencyLayer(0.6f);

    constexpr float cx = theme::macroCx;
    constexpr float cy = theme::macroCy;
    auto& fonts = Fonts::get();

    // Label
    g.setColour(theme::c(theme::textSecondary));
    g.setFont(fonts.interTight(11.0f, 600));
    g.drawText(str::depth(), juce::Rectangle<float>(cx - 80.0f, 78.0f, 160.0f, 20.0f), juce::Justification::centred, false);

    const float v = getNorm();
    const juce::Colour accentCol = hover_ ? theme::c(theme::accentHover) : theme::c(theme::accent);

    // Scale dots
    for (int i = 0; i <= 10; ++i)
    {
        const float t = static_cast<float>(i) / 10.0f;
        const float deg = theme::macroStartDeg + theme::macroSweepDeg * t;
        const float rr = (i == 0 || i == 5 || i == 10) ? 1.5f : 1.1f;
        const auto p = pointOnCircle(cx, cy, theme::macroHoverR, deg);
        const bool active = t <= v + 1.0e-4f;
        g.setColour(active ? accentCol : theme::c(theme::knobDark));
        g.fillEllipse(p.x - rr, p.y - rr, rr * 2.0f, rr * 2.0f);
    }

    // Arcs
    strokeArc(g, cx, cy, theme::macroArcR, theme::macroStartDeg, theme::macroStartDeg + theme::macroSweepDeg, 5.0f, theme::c(theme::track));
    if (v > 0.0001f)
        strokeArc(g, cx, cy, theme::macroArcR, theme::macroStartDeg, theme::macroStartDeg + theme::macroSweepDeg * v, 5.0f, accentCol);

    // Face
    const juce::Colour face = dragging_ ? theme::c(theme::pressedFace) : theme::c(theme::surface);
    g.setColour(face);
    g.fillEllipse(cx - theme::macroFaceR, cy - theme::macroFaceR, theme::macroFaceR * 2.0f, theme::macroFaceR * 2.0f);
    g.setColour(theme::c(hover_ ? theme::knobDark : theme::line));
    g.drawEllipse(cx - theme::macroFaceR, cy - theme::macroFaceR, theme::macroFaceR * 2.0f, theme::macroFaceR * 2.0f, 1.0f);

    strokeArc(g, cx, cy, 59.5f, -70.0f, 70.0f, 1.0f, theme::c(theme::faceHighlight));

    // Pointer
    const float pdeg = theme::macroStartDeg + theme::macroSweepDeg * v;
    const auto tip = pointOnCircle(cx, cy, theme::macroPointerR, pdeg);
    g.setColour(accentCol);
    g.fillEllipse(tip.x - 3.4f, tip.y - 3.4f, 6.8f, 6.8f);

    // Value
    const int pct = juce::roundToInt(getDenorm());
    g.setFont(fonts.geist(34.0f, 300));
    g.setColour(theme::c(theme::textValue));
    const auto valueBounds = juce::Rectangle<float>(cx - 50.0f, cy - 8.0f, 100.0f, 40.0f);
    g.drawText(juce::String(pct), valueBounds.translated(-6.0f, 0.0f), juce::Justification::centred, false);

    g.setFont(fonts.geist(14.0f, 400));
    g.setColour(theme::c(theme::textSecondary));
    const float numW = juce::GlyphArrangement::getStringWidth(fonts.geist(34.0f, 300), juce::String(pct));
    g.drawText("%", juce::Rectangle<float>(cx + numW * 0.5f - 4.0f, cy + 4.0f, 24.0f, 20.0f), juce::Justification::centredLeft, false);

    // 0 / 100
    g.setFont(fonts.geistMono(10.0f, 400));
    g.setColour(theme::c(theme::textDim));
    const auto p0 = pointOnCircle(cx, cy, theme::macroArcR, theme::macroStartDeg);
    const auto p100 = pointOnCircle(cx, cy, theme::macroArcR, theme::macroStartDeg + theme::macroSweepDeg);
    g.drawText("0", juce::Rectangle<float>(p0.x - 12.0f, p0.y + 8.0f, 24.0f, 14.0f), juce::Justification::centred, false);
    g.drawText("100", juce::Rectangle<float>(p100.x - 16.0f, p100.y + 8.0f, 32.0f, 14.0f), juce::Justification::centred, false);

    if (dimmed_)
        g.endTransparencyLayer();
}

void MacroKnob::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isAltDown())
    {
        attachment_->setValueAsCompleteGesture(param_.convertFrom0to1(param_.getDefaultValue()));
        return;
    }

    dragging_ = true;
    dragStartNorm_ = getNorm();
    dragStartScreen_ = e.getScreenPosition().toFloat();
    mouseHidePos_ = e.getScreenPosition();
    juce::Desktop::getInstance().getMainMouseSource().enableUnboundedMouseMovement(true, true);
    if (attachment_ != nullptr)
        attachment_->beginGesture();
    repaint();
}

void MacroKnob::mouseDrag(const juce::MouseEvent& e)
{
    if (! dragging_)
        return;

    const float dy = dragStartScreen_.y - e.getScreenPosition().toFloat().y;
    const bool fine = e.mods.isShiftDown() || e.mods.isCommandDown() || e.mods.isCtrlDown();
    const float sens = fine ? 0.1f : 1.0f;
    const float delta = (dy / 200.0f) * sens;
    value01_ = juce::jlimit(0.0f, 1.0f, dragStartNorm_ + delta);
    if (attachment_ != nullptr)
        attachment_->setValueAsPartOfGesture(param_.convertFrom0to1(value01_));
    repaint();
}

void MacroKnob::mouseUp(const juce::MouseEvent&)
{
    if (! dragging_)
        return;
    dragging_ = false;
    juce::Desktop::getInstance().getMainMouseSource().enableUnboundedMouseMovement(false);
    juce::Desktop::setMousePosition(mouseHidePos_);
    if (attachment_ != nullptr)
        attachment_->endGesture();
    repaint();
}

void MacroKnob::mouseDoubleClick(const juce::MouseEvent&)
{
    if (attachment_ != nullptr)
        attachment_->setValueAsCompleteGesture(param_.convertFrom0to1(param_.getDefaultValue()));
}

void MacroKnob::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const bool fine = e.mods.isShiftDown() || e.mods.isCommandDown() || e.mods.isCtrlDown();
    const float step = fine ? 0.001f : 0.01f;
    const float delta = wheel.deltaY >= 0 ? step : -step;
    if (attachment_ != nullptr)
        attachment_->setValueAsCompleteGesture(param_.convertFrom0to1(juce::jlimit(0.0f, 1.0f, getNorm() + delta)));
}

bool MacroKnob::keyPressed(const juce::KeyPress& key)
{
    const float step = key.getModifiers().isShiftDown() ? 0.001f : 0.01f;
    if (key == juce::KeyPress::upKey || key == juce::KeyPress::rightKey)
    {
        if (attachment_ != nullptr)
            attachment_->setValueAsCompleteGesture(param_.convertFrom0to1(juce::jlimit(0.0f, 1.0f, getNorm() + step)));
        return true;
    }
    if (key == juce::KeyPress::downKey || key == juce::KeyPress::leftKey)
    {
        if (attachment_ != nullptr)
            attachment_->setValueAsCompleteGesture(param_.convertFrom0to1(juce::jlimit(0.0f, 1.0f, getNorm() - step)));
        return true;
    }
    return false;
}

} // namespace wg::ui
