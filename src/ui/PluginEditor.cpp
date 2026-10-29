#include "PluginEditor.h"
#include "Fonts.h"
#include "Strings.h"
#include "Theme.h"
#include "UiHelpers.h"
#include "plugin/ParameterIds.h"
#include "plugin/PluginProcessor.h"

namespace wg::ui {
namespace {

juce::RangedAudioParameter& ranged(juce::AudioProcessorValueTreeState& a, const char* id)
{
    auto* p = dynamic_cast<juce::RangedAudioParameter*>(a.getParameter(id));
    jassert(p != nullptr);
    return *p;
}

juce::AudioParameterChoice& choice(juce::AudioProcessorValueTreeState& a, const char* id)
{
    auto* p = dynamic_cast<juce::AudioParameterChoice*>(a.getParameter(id));
    jassert(p != nullptr);
    return *p;
}

} // namespace

PluginEditor::Content::Content(PluginEditor& o)
    : owner(o)
    , presetBar(o.processor_)
    , macro(ranged(o.processor_.getAPVTS(), wg::plugin::param::amount))
    , outGain(ranged(o.processor_.getAPVTS(), wg::plugin::param::outGain),
              RotaryKnob::Style::OutGain,
              [](float v) { return str::formatDb(v); })
    , autoGain(choice(o.processor_.getAPVTS(), wg::plugin::param::autoGain))
    , advanced(o.processor_)
{
    setOpaque(true);
    addAndMakeVisible(presetBar);
    addAndMakeVisible(vinyl);
    addAndMakeVisible(macro);
    addAndMakeVisible(outGain);
    addAndMakeVisible(autoGain);
    addChildComponent(advanced);

    outGain.setShowCaption(false);
    vinyl.setPlayingSource(&o.processor_.getTransportState().isPlaying);
    vinyl.setLabelAngleDegrees(28.0f);
    vinyl.setInterceptsMouseClicks(false, false);
}

void PluginEditor::Content::paint(juce::Graphics& g)
{
    g.fillAll(theme::c(theme::bg));
    auto& fonts = Fonts::get();
    const bool dim = owner.bypassOn_;

    auto drawMainChrome = [&](juce::Graphics& gg) {
        gg.setFont(fonts.interTight(10.0f, 600));
        gg.setColour(theme::c(theme::textSecondary));
        gg.drawText(str::out(), juce::Rectangle<float>(68.0f, 346.0f, 80.0f, 14.0f), juce::Justification::centredLeft, false);
        gg.setFont(fonts.inter(11.5f, 500));
        gg.setColour(theme::c(theme::textPrimary));
        gg.drawText(outGain.currentText(), juce::Rectangle<float>(68.0f, 362.0f, 120.0f, 16.0f), juce::Justification::centredLeft, false);

        gg.setFont(fonts.interTight(10.0f, 600));
        gg.setColour(theme::c(theme::textSecondary));
        gg.drawText(str::autoLevel(), juce::Rectangle<float>(318.0f, 332.0f, 160.0f, 14.0f), juce::Justification::centredLeft, false);

        gg.setColour(theme::c(theme::divider));
        gg.drawLine(0.0f, 392.5f, 560.0f, 392.5f, 1.0f);

        if (owner.advancedOpen_)
            drawChevronDown(gg, 24.0f, 406.5f, 7.0f, theme::c(theme::textSecondary));
        else
            drawChevronRightSmall(gg, 24.0f, 406.5f, 7.0f, theme::c(theme::textSecondary));

        gg.setFont(fonts.inter(11.5f, 500));
        gg.setColour(theme::c(theme::textControl));
        gg.drawText(str::advanced(), juce::Rectangle<float>(36.0f, 396.0f, 200.0f, 20.0f), juce::Justification::centredLeft, false);
    };

    if (dim)
    {
        g.beginTransparencyLayer(0.6f);
        drawMainChrome(g);
        g.endTransparencyLayer();
    }
    else
    {
        drawMainChrome(g);
    }
}

void PluginEditor::Content::resized()
{
    presetBar.setBounds(0, 0, theme::kWidth, 49);
    // Full logical canvas so Theme absolute coords match SPEC §9.1
    vinyl.setBounds(0, 0, theme::kWidth, theme::kHeightCollapsed);
    macro.setBounds(0, 0, theme::kWidth, theme::kHeightCollapsed);
    outGain.setBounds(22, 342, 40, 40);
    autoGain.setBounds(318, 353, 228, 25);
    advanced.setBounds(0, 420, theme::kWidth, theme::kAdvancedExtra);
    advanced.setVisible(owner.advancedOpen_);
}

void PluginEditor::Content::mouseUp(const juce::MouseEvent& e)
{
    if (e.y >= 392 && e.y < 420)
        owner.setAdvancedOpen(! owner.advancedOpen_);
}

PluginEditor::PluginEditor(wg::plugin::PluginProcessor& processor)
    : AudioProcessorEditor(processor)
    , processor_(processor)
    , content_(*this)
{
    setLookAndFeel(&lnf_);
    addAndMakeVisible(content_);

    setResizable(true, true);
    setResizeLimits(juce::roundToInt(theme::kWidth * theme::kScaleMin),
                    juce::roundToInt(theme::kHeightCollapsed * theme::kScaleMin),
                    juce::roundToInt(theme::kWidth * theme::kScaleMax),
                    juce::roundToInt(theme::kHeightExpanded * theme::kScaleMax));

    processor_.getAPVTS().addParameterListener(wg::plugin::param::bypass, this);
    if (auto* bp = processor_.getAPVTS().getParameter(wg::plugin::param::bypass))
        bypassOn_ = bp->getValue() >= 0.5f;

    amountWatch_ = std::make_unique<juce::ParameterAttachment>(
        ranged(processor_.getAPVTS(), wg::plugin::param::amount),
        [this](float v) {
            const auto& p = ranged(processor_.getAPVTS(), wg::plugin::param::amount);
            content_.vinyl.setAmount01(p.convertTo0to1(v));
        },
        nullptr);
    amountWatch_->sendInitialUpdate();

    updateBypassDim();
    applyWindowSize();
    startTimerHz(15);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    processor_.getAPVTS().removeParameterListener(wg::plugin::param::bypass, this);
    setLookAndFeel(nullptr);
}

void PluginEditor::timerCallback()
{
    content_.advanced.setPlaying(processor_.getTransportState().isPlaying.load(std::memory_order_relaxed));
    // Refresh out-gain text painted in content
    content_.repaint(68, 362, 120, 16);
}

void PluginEditor::parameterChanged(const juce::String& id, float value)
{
    if (id == wg::plugin::param::bypass)
    {
        bypassOn_ = value >= 0.5f;
        updateBypassDim();
    }
}

void PluginEditor::updateBypassDim()
{
    content_.presetBar.setDimmed(bypassOn_);
    content_.vinyl.setDimmed(bypassOn_);
    content_.macro.setDimmed(bypassOn_);
    content_.outGain.setDimmed(bypassOn_);
    content_.autoGain.setDimmed(bypassOn_);
    content_.advanced.setDimmed(bypassOn_);
    content_.repaint();
}

void PluginEditor::setUiScale(float scale)
{
    uiScale_ = juce::jlimit(theme::kScaleMin, theme::kScaleMax, scale);
    applyWindowSize();
}

void PluginEditor::setAdvancedOpen(bool open)
{
    if (advancedOpen_ == open)
        return;
    advancedOpen_ = open;
    content_.advanced.setVisible(open);
    applyWindowSize();
}

void PluginEditor::applyWindowSize()
{
    const float logicalH = advancedOpen_ ? (float) theme::kHeightExpanded : (float) theme::kHeightCollapsed;
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio((double) theme::kWidth / (double) logicalH);
    setSize(juce::roundToInt(theme::kWidth * uiScale_), juce::roundToInt(logicalH * uiScale_));
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(theme::c(theme::bg));
}

void PluginEditor::resized()
{
    if (getWidth() > 0)
        uiScale_ = juce::jlimit(theme::kScaleMin, theme::kScaleMax, (float) getWidth() / (float) theme::kWidth);

    const int logicalH = advancedOpen_ ? theme::kHeightExpanded : theme::kHeightCollapsed;
    content_.setTransform(juce::AffineTransform::scale(uiScale_));
    content_.setBounds(0, 0, theme::kWidth, logicalH);
}

} // namespace wg::ui
