#include "Fonts.h"

#include "UiAssets.h"

namespace wg::ui {
namespace {

juce::Typeface::Ptr loadTypeface(const char* data, int size)
{
    if (data == nullptr || size <= 0)
        return {};
    return juce::Typeface::createSystemTypefaceFor(data, static_cast<size_t>(size));
}

juce::Font makeFont(const juce::Typeface::Ptr& tf, float sizePx)
{
    if (tf != nullptr)
        return juce::Font(juce::FontOptions{ tf }.withHeight(sizePx));
    return juce::Font(juce::FontOptions().withHeight(sizePx));
}

} // namespace

Fonts& Fonts::get()
{
    static Fonts instance;
    return instance;
}

Fonts::Fonts()
{
    interRegular_ = loadTypeface(UiAssets::InterRegular_ttf, UiAssets::InterRegular_ttfSize);
    interMedium_ = loadTypeface(UiAssets::InterMedium_ttf, UiAssets::InterMedium_ttfSize);
    interSemiBold_ = loadTypeface(UiAssets::InterSemiBold_ttf, UiAssets::InterSemiBold_ttfSize);
    interBold_ = loadTypeface(UiAssets::InterBold_ttf, UiAssets::InterBold_ttfSize);
    interTight_ = loadTypeface(UiAssets::InterTightVariable_ttf, UiAssets::InterTightVariable_ttfSize);
    geistLight_ = loadTypeface(UiAssets::GeistLight_ttf, UiAssets::GeistLight_ttfSize);
    geistRegular_ = loadTypeface(UiAssets::GeistRegular_ttf, UiAssets::GeistRegular_ttfSize);
    geistSemiBold_ = loadTypeface(UiAssets::GeistSemiBold_ttf, UiAssets::GeistSemiBold_ttfSize);
    geistMonoRegular_ = loadTypeface(UiAssets::GeistMonoRegular_ttf, UiAssets::GeistMonoRegular_ttfSize);
    geistMonoMedium_ = loadTypeface(UiAssets::GeistMonoMedium_ttf, UiAssets::GeistMonoMedium_ttfSize);
}

juce::Font Fonts::inter(float sizePx, int weight) const
{
    if (weight >= 700)
        return makeFont(interBold_ != nullptr ? interBold_ : interSemiBold_, sizePx);
    if (weight >= 600)
        return makeFont(interSemiBold_ != nullptr ? interSemiBold_ : interMedium_, sizePx);
    if (weight >= 500)
        return makeFont(interMedium_ != nullptr ? interMedium_ : interRegular_, sizePx);
    return makeFont(interRegular_, sizePx);
}

juce::Font Fonts::interTight(float sizePx, int weight) const
{
    // Variable Inter Tight preferred; fall back to Inter SemiBold/Bold (same OFL family role).
    if (interTight_ != nullptr)
        return makeFont(interTight_, sizePx);
    return inter(sizePx, weight >= 700 ? 700 : 600);
}

juce::Font Fonts::geist(float sizePx, int weight) const
{
    if (weight >= 600)
        return makeFont(geistSemiBold_ != nullptr ? geistSemiBold_ : geistRegular_, sizePx);
    if (weight >= 400)
        return makeFont(geistRegular_ != nullptr ? geistRegular_ : geistLight_, sizePx);
    return makeFont(geistLight_ != nullptr ? geistLight_ : geistRegular_, sizePx);
}

juce::Font Fonts::geistMono(float sizePx, int weight) const
{
    if (weight >= 500)
        return makeFont(geistMonoMedium_ != nullptr ? geistMonoMedium_ : geistMonoRegular_, sizePx);
    return makeFont(geistMonoRegular_, sizePx);
}

} // namespace wg::ui
