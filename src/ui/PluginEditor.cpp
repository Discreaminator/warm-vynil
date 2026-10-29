#include "PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "plugin/ProductInfo.h"

namespace wg::ui {

PluginEditor::PluginEditor(wg::plugin::PluginProcessor& processor)
    : AudioProcessorEditor(processor)
{
    setSize(560, 420);
}

PluginEditor::~PluginEditor() = default;

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff16171a));
    g.setColour(juce::Colour(0xffffb000));
    g.setFont(24.0f);
    g.drawFittedText(wg::plugin::product::kName, getLocalBounds().reduced(24, 80),
                     juce::Justification::centredTop, 1);

    g.setColour(juce::Colour(0xff8a8d94));
    g.setFont(14.0f);
    g.drawFittedText("M0 stub editor — passthrough", getLocalBounds().reduced(24, 140),
                     juce::Justification::centredTop, 1);
}

void PluginEditor::resized() {}

} // namespace wg::ui
