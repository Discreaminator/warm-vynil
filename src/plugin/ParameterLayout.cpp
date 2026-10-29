#include "ParameterLayout.h"
#include "ParameterIds.h"

#include <cmath>
#include <memory>
#include <vector>

namespace wg::plugin {
namespace {

juce::NormalisableRange<float> percentRange()
{
    return { 0.0f, 100.0f, 0.1f };
}

juce::NormalisableRange<float> logFreqRange(float minHz, float maxHz)
{
    juce::NormalisableRange<float> r{ minHz, maxHz, 0.1f };
    r.setSkewForCentre(std::sqrt(minHz * maxHz));
    return r;
}

} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    const auto ver = 1;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::amount, ver }, "Глубина", percentRange(), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::outGain, ver }, "Выход",
        juce::NormalisableRange<float>{ -24.0f, 12.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ param::autoGain, ver }, "Авто-уровень",
        juce::StringArray{ "Выкл", "Статич.", "Динам." }, 1));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ param::bypass, ver }, "Байпас", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::inGain, ver }, "Вход",
        juce::NormalisableRange<float>{ -24.0f, 24.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::mix, ver }, "Сухой/обраб.", percentRange(), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::monoFreq, ver }, "Моно ниже", logFreqRange(40.0f, 300.0f), 150.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::monoDepth, ver }, "Глубина моно", percentRange(), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::tracingTrim, ver }, "Искажения иглы",
        juce::NormalisableRange<float>{ 0.0f, 200.0f, 0.1f }, 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::riaaTilt, ver }, "RIAA рассогласование",
        juce::NormalisableRange<float>{ -1.0f, 1.0f, 0.01f }, 0.5f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ param::riaaNeumann, ver }, "Полюс Neumann", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::cartFreq, ver }, "Резонанс головки",
        logFreqRange(10000.0f, 18000.0f), 14000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::cartQ, ver }, "Добротность головки",
        juce::NormalisableRange<float>{ 0.7f, 3.0f, 0.01f }, 1.2f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::crosstalk, ver }, "Перекрёстные помехи",
        juce::NormalisableRange<float>{ -40.0f, -20.0f, 0.1f }, -30.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::tubeTrim, ver }, "Драйв лампы",
        juce::NormalisableRange<float>{ 0.0f, 200.0f, 0.1f }, 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::tubeBias, ver }, "Смещение лампы",
        juce::NormalisableRange<float>{ -1.0f, 1.0f, 0.01f }, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ param::tubeModel, ver }, "Модель лампы",
        juce::StringArray{ "Koren", "Fast" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::xfmrSat, ver }, "Насыщение трансф.", percentRange(), 40.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::xfmrHF, ver }, "ВЧ-спад трансф.",
        juce::NormalisableRange<float>{ 16000.0f, 24000.0f, 1.0f }, 20000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    // Not host-automatable (SPEC §7): still in APVTS for state / Generic editor.
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ param::osFactor, ver }, "Передискретизация",
        juce::StringArray{ "2x", "4x", "8x" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ param::adaa, ver }, "ADAA", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ param::offlineHQ, ver }, "HQ при рендере", false));

    return { params.begin(), params.end() };
}

} // namespace wg::plugin
