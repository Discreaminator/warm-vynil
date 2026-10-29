#pragma once

#include <juce_graphics/juce_graphics.h>

/** Variant B design tokens — SPEC §9.1.1 (logical px @ 100%). */
namespace wg::ui::theme {

inline constexpr int kWidth = 560;
inline constexpr int kHeightCollapsed = 420;
inline constexpr int kHeightExpanded = 650;
inline constexpr int kAdvancedExtra = 230;

inline constexpr float kScaleMin = 0.75f;
inline constexpr float kScaleMax = 2.0f;

inline juce::Colour c(juce::uint32 argb) noexcept
{
    return juce::Colour(argb);
}

inline constexpr juce::uint32 bg = 0xff16171a;
inline constexpr juce::uint32 panelBg = 0xff1a1b1f;
inline constexpr juce::uint32 surface = 0xff1f2125;
inline constexpr juce::uint32 line = 0xff2c2f35;
inline constexpr juce::uint32 divider = 0xff24262b;
inline constexpr juce::uint32 track = 0xff2a2c31;
inline constexpr juce::uint32 textPrimary = 0xffe8e4dc;
inline constexpr juce::uint32 textValue = 0xfff1ede6;
inline constexpr juce::uint32 textControl = 0xffb8b6b0;
inline constexpr juce::uint32 textLabel = 0xffc9c6bf;
inline constexpr juce::uint32 textSecondary = 0xff8a8d94;
inline constexpr juce::uint32 textCardTitle = 0xff6f727a;
inline constexpr juce::uint32 textDim = 0xff5d6068;
inline constexpr juce::uint32 knobDark = 0xff3a3d43;
inline constexpr juce::uint32 armGrey = 0xff4a4d54;
inline constexpr juce::uint32 faceHighlight = 0xff33363c;
inline constexpr juce::uint32 accent = 0xffffb000;
inline constexpr juce::uint32 accentHover = 0xffffc033;
inline constexpr juce::uint32 vinyl = 0xff0d0e10;
inline constexpr juce::uint32 vinylGap = 0xff0a0a0c;
inline constexpr juce::uint32 vinylEdge = 0xff25272c;
inline constexpr juce::uint32 pressedFace = 0xff25272c;

// Vinyl geometry
inline constexpr float vinylCx = 152.0f;
inline constexpr float vinylCy = 192.0f;
inline constexpr float vinylR = 114.0f;

// Macro knob
inline constexpr float macroCx = 434.0f;
inline constexpr float macroCy = 188.0f;
inline constexpr float macroHoverR = 86.0f;
inline constexpr float macroArcR = 76.0f;
inline constexpr float macroFaceR = 60.0f;
inline constexpr float macroPointerR = 50.0f;
inline constexpr float macroStartDeg = -135.0f;
inline constexpr float macroSweepDeg = 270.0f;

} // namespace wg::ui::theme
