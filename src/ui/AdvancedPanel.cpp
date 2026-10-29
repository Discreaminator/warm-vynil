#include "AdvancedPanel.h"
#include "Fonts.h"
#include "Strings.h"
#include "Theme.h"
#include "plugin/ParameterIds.h"
#include "plugin/PluginProcessor.h"

namespace wg::ui {
namespace {

juce::RangedAudioParameter& ranged(juce::AudioProcessorValueTreeState& apvts, const char* id)
{
    auto* p = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(id));
    jassert(p != nullptr);
    return *p;
}

juce::AudioParameterBool& boolean(juce::AudioProcessorValueTreeState& apvts, const char* id)
{
    auto* p = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(id));
    jassert(p != nullptr);
    return *p;
}

juce::AudioParameterChoice& choice(juce::AudioProcessorValueTreeState& apvts, const char* id)
{
    auto* p = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(id));
    jassert(p != nullptr);
    return *p;
}

} // namespace

AdvancedPanel::AdvancedPanel(wg::plugin::PluginProcessor& processor)
    : processor_(processor)
{
    auto& apvts = processor_.getAPVTS();

    auto addKnob = [this, &apvts](const char* id, const juce::String& caption, auto formatter) {
        auto knob = std::make_unique<RotaryKnob>(ranged(apvts, id), RotaryKnob::Style::Small, formatter);
        knob->setCaption(caption);
        addAndMakeVisible(*knob);
        knobs_.push_back(std::move(knob));
    };

    namespace S = str;
    addKnob(wg::plugin::param::inGain, S::lblIn(), [](float v) { return S::formatDb(v); });
    addKnob(wg::plugin::param::mix, S::lblMix(), [](float v) { return S::formatPct(v, 0); });
    addKnob(wg::plugin::param::monoFreq, S::lblMonoFreq(), [](float v) { return S::formatHz(v); });
    addKnob(wg::plugin::param::monoDepth, S::lblMonoDepth(), [](float v) { return S::formatPct(v, 0); });
    addKnob(wg::plugin::param::tracingTrim, S::lblTracing(), [](float v) { return S::formatPct(v, 0); });
    addKnob(wg::plugin::param::riaaTilt, S::lblTilt(), [](float v) { return S::formatDb(v); });
    addKnob(wg::plugin::param::cartFreq, S::lblCartFreq(), [](float v) { return S::formatHz(v); });
    addKnob(wg::plugin::param::cartQ, S::lblCartQ(), [](float v) { return juce::String(v, 2); });
    addKnob(wg::plugin::param::crosstalk, S::lblXtalk(), [](float v) { return S::formatDb(v); });
    addKnob(wg::plugin::param::tubeTrim, S::lblTubeDrive(), [](float v) { return S::formatPct(v, 0); });
    addKnob(wg::plugin::param::tubeBias, S::lblTubeBias(), [](float v) { return juce::String(v, 2); });
    addKnob(wg::plugin::param::xfmrSat, S::lblXfmrSat(), [](float v) { return S::formatPct(v, 0); });
    addKnob(wg::plugin::param::xfmrHF, S::lblXfmrHF(), [](float v) { return S::formatHz(v); });

    neumann_ = std::make_unique<ToggleSwitch>(boolean(apvts, wg::plugin::param::riaaNeumann));
    addAndMakeVisible(*neumann_);

    adaa_ = std::make_unique<ToggleSwitch>(boolean(apvts, wg::plugin::param::adaa));
    addAndMakeVisible(*adaa_);

    offlineHQ_ = std::make_unique<ToggleSwitch>(boolean(apvts, wg::plugin::param::offlineHQ));
    addAndMakeVisible(*offlineHQ_);

    osFactor_ = std::make_unique<SegmentedControl>(choice(apvts, wg::plugin::param::osFactor));
    addAndMakeVisible(*osFactor_);

    auto& tube = choice(apvts, wg::plugin::param::tubeModel);
    tubeAttach_ = std::make_unique<juce::ParameterAttachment>(
        tube,
        [this](float) {
            tubeModelIndex_ = dynamic_cast<juce::AudioParameterChoice*>(
                                  processor_.getAPVTS().getParameter(wg::plugin::param::tubeModel))
                                  ->getIndex();
            repaint();
        },
        nullptr);
    tubeAttach_->sendInitialUpdate();

    cards_[0] = { { 14.0f, 10.0f, 147.4f, 82.0f }, S::cardInMix() };
    cards_[1] = { { 169.4f, 10.0f, 147.4f, 82.0f }, S::cardMono() };
    cards_[2] = { { 324.9f, 10.0f, 221.1f, 82.0f }, S::cardRiaa() };
    cards_[3] = { { 14.0f, 100.0f, 193.5f, 82.0f }, S::cardCart() };
    cards_[4] = { { 215.5f, 100.0f, 193.5f, 82.0f }, S::cardTube() };
    cards_[5] = { { 417.0f, 100.0f, 129.0f, 82.0f }, S::cardXfmr() };
    cards_[6] = { { 14.0f, 190.0f, 532.0f, 32.0f }, S::cardQuality() };
}

void AdvancedPanel::setDimmed(bool dim)
{
    dimmed_ = dim;
    for (auto& k : knobs_)
        k->setDimmed(dim);
    if (neumann_)
        neumann_->setDimmed(dim);
    if (adaa_)
        adaa_->setDimmed(dim);
    if (offlineHQ_)
        offlineHQ_->setDimmed(dim);
    if (osFactor_)
        osFactor_->setDimmed(dim);
    repaint();
}

void AdvancedPanel::setPlaying(bool playing)
{
    // SPEC: osFactor disabled while playing if change forbidden — soft-disable UI.
    if (osFactor_)
        osFactor_->setEnabled(! playing);
}

void AdvancedPanel::mouseUp(const juce::MouseEvent& e)
{
    const float baseX = 376.8f - 24.0f;
    const float y0 = 520.0f + 22.0f - 420.0f;
    const float y1 = 520.0f + 40.0f - 420.0f;
    auto chip0 = juce::Rectangle<float>(baseX, y0, 48.0f, 15.0f);
    auto chip1 = juce::Rectangle<float>(baseX, y1, 48.0f, 15.0f);
    int idx = -1;
    if (chip0.contains(e.position))
        idx = 0;
    else if (chip1.contains(e.position))
        idx = 1;
    if (idx < 0 || tubeAttach_ == nullptr)
        return;
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(processor_.getAPVTS().getParameter(wg::plugin::param::tubeModel)))
        tubeAttach_->setValueAsCompleteGesture(p->convertTo0to1((float) idx));
}

void AdvancedPanel::resized()
{
    // Card-local knob centres from SPEC (absolute) → panel-relative (panel y starts at 420)
    auto place = [](RotaryKnob& k, float absCx, float absCy) {
        k.setBounds(juce::roundToInt(absCx - 28.0f), juce::roundToInt(absCy - 420.0f - 14.0f), 56, 72);
    };

    // indices: in, mix, monoF, monoD, tracing, tilt, cartF, cartQ, xtalk, tubeD, tubeB, xfmrS, xfmrH
    if (knobs_.size() >= 13)
    {
        place(*knobs_[0], 50.9f, 430.0f + 38.0f);
        place(*knobs_[1], 124.6f, 430.0f + 38.0f);
        place(*knobs_[2], 206.3f, 430.0f + 38.0f);
        place(*knobs_[3], 280.0f, 430.0f + 38.0f);
        place(*knobs_[4], 361.7f, 430.0f + 38.0f);
        place(*knobs_[5], 435.4f, 430.0f + 38.0f);
        place(*knobs_[6], 46.2f, 520.0f + 38.0f);
        place(*knobs_[7], 110.8f, 520.0f + 38.0f);
        place(*knobs_[8], 175.2f, 520.0f + 38.0f);
        place(*knobs_[9], 247.8f, 520.0f + 38.0f);
        place(*knobs_[10], 312.2f, 520.0f + 38.0f);
        place(*knobs_[11], 449.2f, 520.0f + 38.0f);
        place(*knobs_[12], 513.8f, 520.0f + 38.0f);
    }

    if (neumann_)
        neumann_->setBounds(juce::roundToInt(509.1f - 14.0f), juce::roundToInt(430.0f + 38.0f - 7.5f - 420.0f), 28, 15);
    if (osFactor_)
        osFactor_->setBounds(170, 617 - 420, 102, 18);
    if (adaa_)
        adaa_->setBounds(344 - 14, 626 - 7 - 420, 28, 15);
    if (offlineHQ_)
        offlineHQ_->setBounds(482 - 14, 626 - 7 - 420, 28, 15);
}

void AdvancedPanel::paint(juce::Graphics& g)
{
    if (dimmed_)
        g.beginTransparencyLayer(0.6f);

    g.setColour(theme::c(theme::panelBg));
    g.fillAll();
    g.setColour(theme::c(theme::line));
    g.drawLine(0.0f, 0.5f, 560.0f, 0.5f, 1.0f);

    auto& fonts = Fonts::get();

    for (int i = 0; i < 7; ++i)
    {
        const auto& c = cards_[i];
        g.setColour(theme::c(theme::surface));
        g.fillRoundedRectangle(c.bounds, 8.0f);
        g.setColour(theme::c(theme::track));
        g.drawRoundedRectangle(c.bounds.reduced(0.5f), 8.0f, 1.0f);

        g.setFont(fonts.interTight(i == 6 ? 10.0f : 8.5f, 600));
        g.setColour(theme::c(theme::textCardTitle));
        const float titleX = i == 6 ? c.bounds.getX() + 10.0f : c.bounds.getX() + 10.0f;
        const float titleY = i == 6 ? c.bounds.getY() + 10.0f : c.bounds.getY() + 4.0f;
        g.drawText(c.title, juce::Rectangle<float>(titleX, titleY, c.bounds.getWidth() - 20.0f, 14.0f),
                   juce::Justification::centredLeft, false);
    }

    // Tube model chips
    {
        const float baseX = 376.8f - 24.0f;
        const float y0 = 520.0f + 22.0f - 420.0f;
        const float y1 = 520.0f + 40.0f - 420.0f;
        const char* labels[2] = { "Koren", "Fast" };
        for (int i = 0; i < 2; ++i)
        {
            auto r = juce::Rectangle<float>(baseX, i == 0 ? y0 : y1, 48.0f, 15.0f);
            const bool sel = tubeModelIndex_ == i;
            g.setColour(sel ? theme::c(theme::accent) : theme::c(theme::bg));
            g.fillRoundedRectangle(r, 7.5f);
            if (! sel)
            {
                g.setColour(theme::c(theme::line));
                g.drawRoundedRectangle(r.reduced(0.5f), 7.5f, 1.0f);
            }
            g.setFont(fonts.inter(9.5f, sel ? 600 : 500));
            g.setColour(sel ? theme::c(theme::bg) : theme::c(theme::textControl));
            g.drawText(labels[i], r, juce::Justification::centred, false);
        }
    }

    // Quality row labels
    g.setFont(fonts.inter(10.0f, 500));
    g.setColour(theme::c(theme::textLabel));
    g.drawText(str::lblOs(), juce::Rectangle<float>(110.0f, 617.0f - 420.0f, 55.0f, 18.0f), juce::Justification::centredLeft, false);
    g.drawText(str::lblAdaa(), juce::Rectangle<float>(296.0f, 617.0f - 420.0f, 40.0f, 18.0f), juce::Justification::centredLeft, false);
    g.drawText(str::lblOfflineHQ(), juce::Rectangle<float>(384.0f, 617.0f - 420.0f, 90.0f, 18.0f), juce::Justification::centredLeft, false);

    // Neumann caption
    g.setFont(fonts.inter(10.0f, 500));
    g.setColour(theme::c(theme::textLabel));
    g.drawText(str::lblNeumann(), juce::Rectangle<float>(509.1f - 24.0f, 430.0f + 55.0f - 420.0f, 48.0f, 12.0f),
               juce::Justification::centred, false);

    if (dimmed_)
        g.endTransparencyLayer();
}

} // namespace wg::ui
