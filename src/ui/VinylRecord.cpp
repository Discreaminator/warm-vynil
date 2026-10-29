#include "VinylRecord.h"
#include "Fonts.h"
#include "Strings.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <cmath>

namespace wg::ui {

namespace {
constexpr float kTargetOmega = 200.0f; // deg/s @ 33⅓
constexpr float kTauAccel = 0.35f;
constexpr float kTauDecel = 0.80f;
} // namespace

VinylRecord::VinylRecord()
{
    setOpaque(false);
    setInterceptsMouseClicks(false, false);
}

VinylRecord::~VinylRecord()
{
    stopTicker();
}

void VinylRecord::visibilityChanged()
{
    if (isShowing())
        startTicker();
    else
        stopTicker();
}

void VinylRecord::startTicker()
{
    lastMs_ = juce::Time::getMillisecondCounterHiRes();
    vblank_ = std::make_unique<juce::VBlankAttachment>(this, [this] { tick(); });
    if (vblank_ == nullptr)
        startTimerHz(60);
}

void VinylRecord::stopTicker()
{
    vblank_.reset();
    stopTimer();
}

void VinylRecord::timerCallback()
{
    tick();
}

void VinylRecord::tick()
{
    if (! isShowing())
        return;

    const double now = juce::Time::getMillisecondCounterHiRes();
    float dt = static_cast<float>((now - lastMs_) * 0.001);
    lastMs_ = now;
    dt = juce::jlimit(0.0f, 0.05f, dt);

    const bool playing = playing_ != nullptr && playing_->load(std::memory_order_relaxed);
    const float target = playing ? kTargetOmega : 0.0f;
    const float tau = (target > omegaDegPerSec_) ? kTauAccel : kTauDecel;
    const float a = 1.0f - std::exp(-dt / tau);
    omegaDegPerSec_ += (target - omegaDegPerSec_) * a;
    if (std::abs(omegaDegPerSec_) < 0.5f)
        omegaDegPerSec_ = 0.0f;

    if (omegaDegPerSec_ != 0.0f)
    {
        angleDeg_ = std::fmod(angleDeg_ + omegaDegPerSec_ * dt, 360.0f);
        if (angleDeg_ < 0.0f)
            angleDeg_ += 360.0f;
        repaint();
    }
}

void VinylRecord::paint(juce::Graphics& g)
{
    juce::Graphics::ScopedSaveState state(g);

    if (dimmed_)
        g.beginTransparencyLayer(0.6f);

    constexpr float cx = theme::vinylCx;
    constexpr float cy = theme::vinylCy;
    constexpr float R = theme::vinylR;

    paintDisc(g, cx, cy, R);
    paintLabel(g, cx, cy, R);

    // Spindle (does not rotate)
    g.setColour(theme::c(theme::bg));
    g.fillEllipse(cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
    g.setColour(theme::c(theme::knobDark));
    g.fillEllipse(cx - 1.2f, cy - 1.2f, 2.4f, 2.4f);

    paintTonearm(g);

    if (dimmed_)
        g.endTransparencyLayer();
}

void VinylRecord::paintDisc(juce::Graphics& g, float cx, float cy, float R) const
{
    g.setColour(theme::c(theme::vinyl));
    g.fillEllipse(cx - R, cy - R, R * 2.0f, R * 2.0f);

    const struct Band
    {
        float outer;
        float inner;
    } bands[] = {
        { 0.958f, 0.80f },
        { 0.785f, 0.625f },
        { 0.61f, 0.425f },
    };

    int grooveIndex = 0;
    for (const auto& b : bands)
    {
        for (float t = b.outer; t >= b.inner; t -= (1.5f / R))
        {
            const float rr = t * R;
            const float opacity = (grooveIndex++ % 2 == 0) ? 0.055f : 0.03f;
            g.setColour(juce::Colours::white.withAlpha(opacity));
            g.drawEllipse(cx - rr, cy - rr, rr * 2.0f, rr * 2.0f, 0.55f);
        }
        // Gap stroke between bands
        const float gapR = b.inner * R - 0.75f;
        if (gapR > 0.0f)
        {
            g.setColour(theme::c(theme::vinylGap));
            g.drawEllipse(cx - gapR, cy - gapR, gapR * 2.0f, gapR * 2.0f, 1.71f);
        }
    }

    g.setColour(theme::c(theme::vinylEdge));
    g.drawEllipse(cx - (R - 0.75f), cy - (R - 0.75f), (R - 0.75f) * 2.0f, (R - 0.75f) * 2.0f, 1.5f);

    // Static glare sectors (approximate SPEC peaks @ 315° and 135°)
    auto paintGlare = [&](float centerDeg, float halfWidth, float peakAlpha) {
        const int steps = 24;
        for (int i = 0; i < steps; ++i)
        {
            const float t0 = (static_cast<float>(i) / static_cast<float>(steps)) * 2.0f - 1.0f;
            const float t1 = (static_cast<float>(i + 1) / static_cast<float>(steps)) * 2.0f - 1.0f;
            const float a0 = centerDeg + t0 * halfWidth;
            const float a1 = centerDeg + t1 * halfWidth;
            const float mid = 0.5f * (t0 + t1);
            const float fall = std::pow(1.0f - mid * mid, 1.6f);
            const float alpha = peakAlpha * fall;
            if (alpha < 0.001f)
                continue;

            juce::Path wedge;
            const float rIn = 0.36f * R;
            const float rOut = R - 1.5f;
            const auto p0 = pointOnCircle(cx, cy, rOut, a0);
            const auto p1 = pointOnCircle(cx, cy, rOut, a1);
            const auto p2 = pointOnCircle(cx, cy, rIn, a1);
            const auto p3 = pointOnCircle(cx, cy, rIn, a0);
            wedge.startNewSubPath(p0);
            wedge.lineTo(p1);
            wedge.lineTo(p2);
            wedge.lineTo(p3);
            wedge.closeSubPath();
            g.setColour(juce::Colours::white.withAlpha(alpha));
            g.fillPath(wedge);
        }
    };

    paintGlare(315.0f, 24.0f, 0.075f);
    paintGlare(135.0f, 22.0f, 0.05f);
}

void VinylRecord::paintLabel(juce::Graphics& g, float cx, float cy, float R) const
{
    const float rl = 0.34f * R;
    juce::Graphics::ScopedSaveState ss(g);
    g.addTransform(juce::AffineTransform::rotation(degToRad(angleDeg_), cx, cy));

    // Subtle glow scales with Amount
    if (amount01_ > 0.01f)
    {
        g.setColour(theme::c(theme::accent).withAlpha(0.12f * amount01_));
        g.fillEllipse(cx - rl - 3.0f, cy - rl - 3.0f, (rl + 3.0f) * 2.0f, (rl + 3.0f) * 2.0f);
    }

    g.setColour(theme::c(theme::accent));
    g.fillEllipse(cx - rl, cy - rl, rl * 2.0f, rl * 2.0f);

    // Asymmetric mark (bg on accent)
    strokeArc(g, cx, cy, 0.6f * rl, 200.0f, 262.0f, 3.2f, theme::c(theme::bg));
    const auto dot = pointOnCircle(cx, cy, 0.6f * rl, 280.0f);
    g.setColour(theme::c(theme::bg));
    g.fillEllipse(dot.x - 1.6f, dot.y - 1.6f, 3.2f, 3.2f);

    // Circular brand text along r = rl - 9
    const juce::String brand = str::brand(); // WARM VYNIL — fits ~10 glyphs like WARM GROOVE
    const float textR = rl - 9.0f;
    auto& fonts = Fonts::get();
    g.setFont(fonts.interTight(6.8f, 700));
    g.setColour(theme::c(theme::bg));

    // Spread letters across ~110° arc (top of label)
    const float arcSpan = 110.0f;
    const float startA = -arcSpan * 0.5f;
    const int n = brand.length();
    for (int i = 0; i < n; ++i)
    {
        const float a = startA + (n > 1 ? (arcSpan * static_cast<float>(i) / static_cast<float>(n - 1)) : 0.0f);
        const auto p = pointOnCircle(cx, cy, textR, a);
        juce::Graphics::ScopedSaveState letter(g);
        g.addTransform(juce::AffineTransform::rotation(degToRad(a), p.x, p.y));
        const juce::String ch = brand.substring(i, i + 1);
        g.drawText(ch, juce::Rectangle<float>(p.x - 4.0f, p.y - 4.0f, 8.0f, 8.0f), juce::Justification::centred, false);
    }

    g.setFont(fonts.geist(11.0f, 600));
    g.setColour(theme::c(theme::bg));
    g.drawText(juce::CharPointer_UTF8("33\xe2\x85\x93"),
               juce::Rectangle<float>(cx + 8.0f, cy - 4.0f, 28.0f, 16.0f),
               juce::Justification::centredLeft,
               false);
}

void VinylRecord::paintTonearm(juce::Graphics& g) const
{
    constexpr float pivotX = 292.0f;
    constexpr float pivotY = 84.0f;

    // Counterweight behind pivot (~18 px at ~ -60° from vertical toward top-right)
    const float cwX = pivotX + 9.0f;
    const float cwY = pivotY - 15.59f;
    g.setColour(theme::c(theme::knobDark));
    g.drawLine(cwX, cwY, pivotX, pivotY, 3.0f);
    g.setColour(theme::c(theme::surface));
    g.fillEllipse(cwX - 6.0f, cwY - 6.0f, 12.0f, 12.0f);
    g.setColour(theme::c(theme::line));
    g.drawEllipse(cwX - 6.0f, cwY - 6.0f, 12.0f, 12.0f, 1.2f);

    // Tube toward (202, 240), shortened 12 px
    const float tipX = 202.0f;
    const float tipY = 240.0f;
    const float dx = tipX - pivotX;
    const float dy = tipY - pivotY;
    const float len = std::sqrt(dx * dx + dy * dy);
    const float ux = dx / len;
    const float uy = dy / len;
    const float endX = tipX - ux * 12.0f;
    const float endY = tipY - uy * 12.0f;

    g.setColour(theme::c(theme::armGrey));
    g.drawLine(pivotX, pivotY, endX, endY, 2.6f);

    // Head +24° from tube direction, 18 px, thickness 7
    const float headAng = std::atan2(ux, -uy) + degToRad(24.0f); // from up
    // Simpler: head along tube direction
    const float hx = endX + ux * 18.0f;
    const float hy = endY + uy * 18.0f;
    g.drawLine(endX, endY, hx, hy, 7.0f);

    // Amber stripe on head (8…15 px along head)
    const float s0x = endX + ux * 8.0f;
    const float s0y = endY + uy * 8.0f;
    const float s1x = endX + ux * 15.0f;
    const float s1y = endY + uy * 15.0f;
    g.setColour(theme::c(theme::accent));
    g.drawLine(s0x, s0y, s1x, s1y, 2.2f);

    // Pivot
    g.setColour(theme::c(theme::surface));
    g.fillEllipse(pivotX - 11.0f, pivotY - 11.0f, 22.0f, 22.0f);
    g.setColour(theme::c(theme::line));
    g.drawEllipse(pivotX - 11.0f, pivotY - 11.0f, 22.0f, 22.0f, 1.2f);
    g.setColour(theme::c(theme::textSecondary));
    g.fillEllipse(pivotX - 3.2f, pivotY - 3.2f, 6.4f, 6.4f);

    juce::ignoreUnused(headAng);
}

} // namespace wg::ui
