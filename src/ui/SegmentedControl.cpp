#include "SegmentedControl.h"
#include "Fonts.h"
#include "Theme.h"

namespace wg::ui {

SegmentedControl::SegmentedControl(juce::AudioParameterChoice& param, juce::UndoManager* undo)
    : param_(param)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setTitle(param.getName(64));
    attachment_ = std::make_unique<juce::ParameterAttachment>(
        param_,
        [this](float) {
            index_ = param_.getIndex();
            repaint();
        },
        undo);
    attachment_->sendInitialUpdate();
}

void SegmentedControl::paint(juce::Graphics& g)
{
    if (dimmed_)
        g.beginTransparencyLayer(0.6f);

    auto r = getLocalBounds().toFloat();
    const float h = r.getHeight();
    const int n = param_.choices.size();
    const float segW = r.getWidth() / static_cast<float>(juce::jmax(1, n));

    g.setColour(theme::c(theme::surface));
    g.fillRoundedRectangle(r, h * 0.5f);
    g.setColour(theme::c(theme::line));
    g.drawRoundedRectangle(r.reduced(0.5f), h * 0.5f, 1.0f);

    // Selected pill
    if (n > 0)
    {
        auto sel = juce::Rectangle<float>(r.getX() + segW * static_cast<float>(index_) + 2.0f,
                                          r.getY() + 2.0f,
                                          segW - 4.0f,
                                          h - 4.0f);
        g.setColour(theme::c(theme::accent));
        g.fillRoundedRectangle(sel, sel.getHeight() * 0.5f);
    }

    // Dividers between unselected neighbours
    for (int i = 1; i < n; ++i)
    {
        if (i == index_ || i - 1 == index_)
            continue;
        const float x = r.getX() + segW * static_cast<float>(i);
        g.setColour(theme::c(theme::line));
        g.drawLine(x, r.getY() + 6.0f, x, r.getBottom() - 6.0f, 1.0f);
    }

    auto& fonts = Fonts::get();
    for (int i = 0; i < n; ++i)
    {
        const bool sel = i == index_;
        g.setFont(fonts.inter(11.5f, sel ? 600 : 500));
        g.setColour(sel ? theme::c(theme::bg) : theme::c(theme::textControl));
        auto cell = juce::Rectangle<float>(r.getX() + segW * static_cast<float>(i), r.getY(), segW, h);
        g.drawText(param_.choices[i], cell, juce::Justification::centred, false);
    }

    if (dimmed_)
        g.endTransparencyLayer();
}

void SegmentedControl::mouseUp(const juce::MouseEvent& e)
{
    const int n = param_.choices.size();
    if (n <= 0)
        return;
    const int i = juce::jlimit(0, n - 1, (int) (e.position.x / (getWidth() / (float) n)));
    // ParameterAttachment expects a denormalised value (choice index), then normalises internally.
    attachment_->setValueAsCompleteGesture(static_cast<float>(i));
}

} // namespace wg::ui
