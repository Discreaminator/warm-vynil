#pragma once

#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>

namespace wg::ui {

/** SPEC angles: 0° = up, clockwise. Matches juce Path::addCentredArc. */
inline float degToRad(float deg) noexcept
{
    return juce::degreesToRadians(deg);
}

inline juce::Point<float> pointOnCircle(float cx, float cy, float r, float degFromUpCw) noexcept
{
    const float rad = degToRad(degFromUpCw);
    return { cx + r * std::sin(rad), cy - r * std::cos(rad) };
}

inline void strokeArc(juce::Graphics& g,
                      float cx,
                      float cy,
                      float r,
                      float startDeg,
                      float endDeg,
                      float thickness,
                      juce::Colour colour)
{
    juce::Path p;
    p.addCentredArc(cx, cy, r, r, 0.0f, degToRad(startDeg), degToRad(endDeg), true);
    g.setColour(colour);
    g.strokePath(p, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

inline void drawChevronLeft(juce::Graphics& g, float cx, float cy, float size, juce::Colour colour)
{
    juce::Path p;
    const float h = size * 0.5f;
    p.startNewSubPath(cx + h * 0.55f, cy - h);
    p.lineTo(cx - h * 0.45f, cy);
    p.lineTo(cx + h * 0.55f, cy + h);
    g.setColour(colour);
    g.strokePath(p, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

inline void drawChevronRight(juce::Graphics& g, float cx, float cy, float size, juce::Colour colour)
{
    juce::Path p;
    const float h = size * 0.5f;
    p.startNewSubPath(cx - h * 0.55f, cy - h);
    p.lineTo(cx + h * 0.45f, cy);
    p.lineTo(cx - h * 0.55f, cy + h);
    g.setColour(colour);
    g.strokePath(p, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

inline void drawChevronDown(juce::Graphics& g, float cx, float cy, float size, juce::Colour colour)
{
    juce::Path p;
    const float h = size * 0.5f;
    p.startNewSubPath(cx - h, cy - h * 0.35f);
    p.lineTo(cx, cy + h * 0.45f);
    p.lineTo(cx + h, cy - h * 0.35f);
    g.setColour(colour);
    g.strokePath(p, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

inline void drawChevronRightSmall(juce::Graphics& g, float cx, float cy, float size, juce::Colour colour)
{
    juce::Path p;
    const float h = size * 0.5f;
    p.startNewSubPath(cx - h * 0.35f, cy - h);
    p.lineTo(cx + h * 0.45f, cy);
    p.lineTo(cx - h * 0.35f, cy + h);
    g.setColour(colour);
    g.strokePath(p, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

} // namespace wg::ui
