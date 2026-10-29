#pragma once

#include <juce_graphics/juce_graphics.h>

namespace wg::ui {

/** Embedded OFL typefaces (BinaryData UiAssets). */
class Fonts
{
public:
    static Fonts& get();

    juce::Font inter(float sizePx, int weight = 500) const;
    juce::Font interTight(float sizePx, int weight = 600) const;
    juce::Font geist(float sizePx, int weight = 300) const;
    juce::Font geistMono(float sizePx, int weight = 400) const;

private:
    Fonts();

    juce::Typeface::Ptr interRegular_;
    juce::Typeface::Ptr interMedium_;
    juce::Typeface::Ptr interSemiBold_;
    juce::Typeface::Ptr interBold_;
    juce::Typeface::Ptr interTight_;
    juce::Typeface::Ptr geistLight_;
    juce::Typeface::Ptr geistRegular_;
    juce::Typeface::Ptr geistSemiBold_;
    juce::Typeface::Ptr geistMonoRegular_;
    juce::Typeface::Ptr geistMonoMedium_;
};

} // namespace wg::ui
