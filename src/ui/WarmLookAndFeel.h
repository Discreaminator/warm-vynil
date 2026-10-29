#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace wg::ui {

/** Shared L&F hooks (popup menus, alerts) for Variant B. */
class WarmLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    WarmLookAndFeel();

    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;
};

} // namespace wg::ui
