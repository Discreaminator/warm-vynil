#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <atomic>
#include <memory>

namespace wg::ui {

/** Rotating vinyl platter — SPEC §9.1.3 / §9.2. Label rotates; grooves & glare static. */
class VinylRecord final : public juce::Component,
                          private juce::Timer
{
public:
    VinylRecord();
    ~VinylRecord() override;

    void setPlayingSource(std::atomic<bool>* playing) noexcept { playing_ = playing; }
    void setAmount01(float amount01) noexcept { amount01_ = juce::jlimit(0.0f, 1.0f, amount01); }
    void setDimmed(bool dim) noexcept
    {
        dimmed_ = dim;
        repaint();
    }

    /** Force label angle (degrees) for screenshot / UI-1 (e.g. 28°). */
    void setLabelAngleDegrees(float deg) noexcept
    {
        angleDeg_ = deg;
        repaint();
    }

    void paint(juce::Graphics& g) override;
    void visibilityChanged() override;

private:
    void timerCallback() override;
    void startTicker();
    void stopTicker();
    void tick();

    void paintDisc(juce::Graphics& g, float cx, float cy, float R) const;
    void paintLabel(juce::Graphics& g, float cx, float cy, float R) const;
    void paintTonearm(juce::Graphics& g) const;

    std::atomic<bool>* playing_ = nullptr;
    float amount01_ = 0.55f;
    bool dimmed_ = false;

    float angleDeg_ = 28.0f;
    float omegaDegPerSec_ = 0.0f;
    double lastMs_ = 0.0;

    std::unique_ptr<juce::VBlankAttachment> vblank_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VinylRecord)
};

} // namespace wg::ui
