#include "PresetManager.h"
#include "ParameterIds.h"
#include "ProductInfo.h"
#include "StateSerializer.h"

#include <array>

namespace wg::plugin {
namespace {

struct FactoryPreset
{
    const char* name;
    float amount;
    float outGain;
    int autoGain;
    float inGain;
    float mix;
    float monoFreq;
    float monoDepth;
    float tracingTrim;
    float riaaTilt;
    bool riaaNeumann;
    float cartFreq;
    float cartQ;
    float crosstalk;
    float tubeTrim;
    float tubeBias;
    int tubeModel;
    float xfmrSat;
    float xfmrHF;
};

// SPEC §9.3 — excludes osFactor / offlineHQ / uiScale.
constexpr FactoryPreset kFactory[] = {
    // По умолчанию
    { "По умолчанию", 50.0f, 0.0f, 1, 0.0f, 100.0f, 150.0f, 100.0f, 100.0f, 0.5f, true, 14000.0f, 1.2f, -30.0f,
      100.0f, 0.0f, 0, 40.0f, 20000.0f },
    // Лёгкий тёплый
    { "Лёгкий тёплый", 30.0f, 0.0f, 1, 0.0f, 100.0f, 150.0f, 100.0f, 100.0f, 0.3f, true, 14000.0f, 1.2f, -35.0f,
      80.0f, 0.0f, 0, 40.0f, 20000.0f },
    // Классический винил
    { "Классический винил", 55.0f, 0.0f, 1, 0.0f, 100.0f, 150.0f, 100.0f, 100.0f, 0.5f, true, 14000.0f, 1.4f, -28.0f,
      100.0f, 0.0f, 0, 40.0f, 20000.0f },
    // Глубокий ламповый
    { "Глубокий ламповый", 80.0f, 0.0f, 1, 0.0f, 100.0f, 150.0f, 100.0f, 90.0f, 0.5f, true, 14000.0f, 1.2f, -30.0f,
      150.0f, 0.3f, 0, 70.0f, 18500.0f },
};

constexpr int kNumFactory = static_cast<int>(std::size(kFactory));

} // namespace

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts)
{
}

int PresetManager::getNumPrograms() const noexcept
{
    return kNumFactory;
}

juce::String PresetManager::getProgramName(int index) const
{
    if (juce::isPositiveAndBelow(index, kNumFactory))
        return kFactory[static_cast<std::size_t>(index)].name;
    return {};
}

void PresetManager::setParam(const char* id, float value)
{
    if (auto* p = apvts_.getParameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(value));
}

void PresetManager::setChoice(const char* id, int index)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(apvts_.getParameter(id)))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(index)));
}

void PresetManager::setBool(const char* id, bool value)
{
    if (auto* p = dynamic_cast<juce::AudioParameterBool*>(apvts_.getParameter(id)))
        p->setValueNotifyingHost(value ? 1.0f : 0.0f);
}

void PresetManager::applyFactoryPreset(int index)
{
    if (!juce::isPositiveAndBelow(index, kNumFactory))
        return;

    const auto& f = kFactory[static_cast<std::size_t>(index)];
    setParam(param::amount, f.amount);
    setParam(param::outGain, f.outGain);
    setChoice(param::autoGain, f.autoGain);
    setBool(param::bypass, false);
    setParam(param::inGain, f.inGain);
    setParam(param::mix, f.mix);
    setParam(param::monoFreq, f.monoFreq);
    setParam(param::monoDepth, f.monoDepth);
    setParam(param::tracingTrim, f.tracingTrim);
    setParam(param::riaaTilt, f.riaaTilt);
    setBool(param::riaaNeumann, f.riaaNeumann);
    setParam(param::cartFreq, f.cartFreq);
    setParam(param::cartQ, f.cartQ);
    setParam(param::crosstalk, f.crosstalk);
    setParam(param::tubeTrim, f.tubeTrim);
    setParam(param::tubeBias, f.tubeBias);
    setChoice(param::tubeModel, f.tubeModel);
    setParam(param::xfmrSat, f.xfmrSat);
    setParam(param::xfmrHF, f.xfmrHF);
    // Preserve osFactor / adaa / offlineHQ (SPEC §9.3).
}

void PresetManager::setCurrentProgram(int index)
{
    if (!juce::isPositiveAndBelow(index, kNumFactory))
        return;
    currentProgram_ = index;
    applyFactoryPreset(index);
    lastPresetName_ = kFactory[static_cast<std::size_t>(index)].name;
}

juce::File PresetManager::getUserPresetsDirectory()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                   .getChildFile(product::kName)
                   .getChildFile("Presets");
    dir.createDirectory();
    return dir;
}

bool PresetManager::saveUserPreset(const juce::String& name)
{
    if (name.isEmpty())
        return false;

    auto xml = apvts_.copyState().createXml();
    if (xml == nullptr)
        return false;

    xml->setAttribute("preset", name);
    xml->setAttribute("stateVersion", StateSerializer::kCurrentVersion);

    const auto file = getUserPresetsDirectory().getChildFile(name + ".wgpreset");
    const bool ok = xml->writeTo(file);
    if (ok)
        lastPresetName_ = name;
    return ok;
}

bool PresetManager::loadUserPreset(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    auto xml = juce::parseXML(file);
    if (xml == nullptr)
        return false;

    const float osKeep = apvts_.getRawParameterValue(param::osFactor)->load();
    const float hqKeep = apvts_.getRawParameterValue(param::offlineHQ)->load();
    const float adaaKeep = apvts_.getRawParameterValue(param::adaa)->load();

    auto tree = juce::ValueTree::fromXml(*xml);
    if (!tree.isValid())
        return false;

    if (!tree.hasType(apvts_.state.getType()))
    {
        juce::ValueTree wrapped{ apvts_.state.getType() };
        for (int i = 0; i < tree.getNumChildren(); ++i)
            wrapped.appendChild(tree.getChild(i).createCopy(), nullptr);
        tree = wrapped;
    }

    apvts_.replaceState(tree);

    if (auto* p = apvts_.getParameter(param::osFactor))
        p->setValueNotifyingHost(p->convertTo0to1(osKeep));
    if (auto* p = apvts_.getParameter(param::offlineHQ))
        p->setValueNotifyingHost(hqKeep >= 0.5f ? 1.0f : 0.0f);
    if (auto* p = apvts_.getParameter(param::adaa))
        p->setValueNotifyingHost(adaaKeep >= 0.5f ? 1.0f : 0.0f);

    lastPresetName_ = xml->getStringAttribute("preset", file.getFileNameWithoutExtension());
    return true;
}

bool PresetManager::deleteUserPreset(const juce::File& file)
{
    return file.existsAsFile() && file.deleteFile();
}

juce::Array<juce::File> PresetManager::listUserPresets() const
{
    return getUserPresetsDirectory().findChildFiles(juce::File::findFiles, false, "*.wgpreset");
}

} // namespace wg::plugin
