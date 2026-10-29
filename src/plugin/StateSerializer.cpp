#include "StateSerializer.h"
#include "ParameterIds.h"

namespace wg::plugin {

void StateSerializer::getState(juce::AudioProcessorValueTreeState& apvts,
                               juce::MemoryBlock& dest,
                               const juce::String& lastPresetName)
{
    auto state = apvts.copyState();
    state.setProperty("stateVersion", kCurrentVersion, nullptr);
    if (lastPresetName.isNotEmpty())
        state.setProperty("lastPresetName", lastPresetName, nullptr);

    if (auto xml = state.createXml())
        juce::AudioProcessor::copyXmlToBinary(*xml, dest);
}

void StateSerializer::setState(juce::AudioProcessorValueTreeState& apvts,
                               const void* data,
                               int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    auto xml = juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr || !xml->hasTagName(apvts.state.getType()))
    {
        // Accept legacy / factory roots or bare PARAMETERS trees.
        if (xml == nullptr)
            return;
    }

    auto tree = juce::ValueTree::fromXml(*xml);
    if (!tree.isValid())
        return;

    const int version = static_cast<int>(tree.getProperty("stateVersion", 1));
    // M2: only v1. Future migrations branch on `version`.
    juce::ignoreUnused(version);

    if (tree.hasType(apvts.state.getType()))
        apvts.replaceState(tree);
    else if (tree.getNumChildren() > 0)
    {
        // Wrap child PARAM nodes if needed.
        juce::ValueTree wrapped{ apvts.state.getType() };
        for (int i = 0; i < tree.getNumChildren(); ++i)
            wrapped.appendChild(tree.getChild(i).createCopy(), nullptr);
        for (int i = 0; i < tree.getNumProperties(); ++i)
        {
            const auto name = tree.getPropertyName(i);
            wrapped.setProperty(name, tree.getProperty(name), nullptr);
        }
        wrapped.setProperty("stateVersion", kCurrentVersion, nullptr);
        apvts.replaceState(wrapped);
    }
}

} // namespace wg::plugin
