#include "WarmLookAndFeel.h"
#include "Fonts.h"
#include "Theme.h"

namespace wg::ui {

WarmLookAndFeel::WarmLookAndFeel()
{
    setColour(juce::PopupMenu::backgroundColourId, theme::c(theme::surface));
    setColour(juce::PopupMenu::textColourId, theme::c(theme::textPrimary));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, theme::c(theme::accent));
    setColour(juce::PopupMenu::highlightedTextColourId, theme::c(theme::bg));
    setColour(juce::AlertWindow::backgroundColourId, theme::c(theme::panelBg));
    setColour(juce::AlertWindow::textColourId, theme::c(theme::textPrimary));
    setColour(juce::TextEditor::backgroundColourId, theme::c(theme::bg));
    setColour(juce::TextEditor::textColourId, theme::c(theme::textPrimary));
    setColour(juce::TextEditor::outlineColourId, theme::c(theme::line));
}

juce::Font WarmLookAndFeel::getPopupMenuFont()
{
    return Fonts::get().inter(13.0f, 500);
}

void WarmLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.setColour(theme::c(theme::surface));
    g.fillRoundedRectangle(0.0f, 0.0f, (float) width, (float) height, 8.0f);
    g.setColour(theme::c(theme::line));
    g.drawRoundedRectangle(0.5f, 0.5f, (float) width - 1.0f, (float) height - 1.0f, 8.0f, 1.0f);
}

} // namespace wg::ui
