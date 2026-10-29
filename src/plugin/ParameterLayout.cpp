#include "ParameterLayout.h"
#include "ParameterIds.h"

#include <cmath>
#include <memory>
#include <vector>

namespace wg::plugin {
namespace {

/** JUCE String(const char*) uses CharPointer_ASCII — UTF-8 must be explicit. */
juce::String u8(const char* utf8)
{
    return juce::String(juce::CharPointer_UTF8(utf8));
}

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
        juce::ParameterID{ param::amount, ver }, u8("\xd0\x93\xd0\xbb\xd1\x83\xd0\xb1\xd0\xb8\xd0\xbd\xd0\xb0"),
        percentRange(), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::outGain, ver }, u8("\xd0\x92\xd1\x8b\xd1\x85\xd0\xbe\xd0\xb4"),
        juce::NormalisableRange<float>{ -24.0f, 12.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ param::autoGain, ver },
        u8("\xd0\x90\xd0\xb2\xd1\x82\xd0\xbe\x2d\xd1\x83\xd1\x80\xd0\xbe\xd0\xb2\xd0\xb5\xd0\xbd\xd1\x8c"),
        juce::StringArray{ u8("\xd0\x92\xd1\x8b\xd0\xba\xd0\xbb"),
                           u8("\xd0\xa1\xd1\x82\xd0\xb0\xd1\x82\xd0\xb8\xd1\x87\x2e"),
                           u8("\xd0\x94\xd0\xb8\xd0\xbd\xd0\xb0\xd0\xbc\x2e") },
        1));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ param::bypass, ver }, u8("\xd0\x91\xd0\xb0\xd0\xb9\xd0\xbf\xd0\xb0\xd1\x81"), false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::inGain, ver }, u8("\xd0\x92\xd1\x85\xd0\xbe\xd0\xb4"),
        juce::NormalisableRange<float>{ -24.0f, 24.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::mix, ver },
        u8("\xd0\xa1\xd1\x83\xd1\x85\xd0\xbe\xd0\xb9\x2f\xd0\xbe\xd0\xb1\xd1\x80\xd0\xb0\xd0\xb1\x2e"),
        percentRange(), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::monoFreq, ver },
        u8("\xd0\x9c\xd0\xbe\xd0\xbd\xd0\xbe\x20\xd0\xbd\xd0\xb8\xd0\xb6\xd0\xb5"),
        logFreqRange(40.0f, 300.0f), 150.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::monoDepth, ver },
        u8("\xd0\x93\xd0\xbb\xd1\x83\xd0\xb1\xd0\xb8\xd0\xbd\xd0\xb0\x20\xd0\xbc\xd0\xbe\xd0\xbd\xd0\xbe"),
        percentRange(), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::tracingTrim, ver },
        u8("\xd0\x98\xd1\x81\xd0\xba\xd0\xb0\xd0\xb6\xd0\xb5\xd0\xbd\xd0\xb8\xd1\x8f\x20\xd0\xb8\xd0\xb3\xd0\xbb\xd1\x8b"),
        juce::NormalisableRange<float>{ 0.0f, 200.0f, 0.1f }, 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::riaaTilt, ver },
        u8("\x52\x49\x41\x41\x20\xd1\x80\xd0\xb0\xd1\x81\xd1\x81\xd0\xbe\xd0\xb3\xd0\xbb\xd0\xb0\xd1\x81\xd0\xbe\xd0\xb2\xd0\xb0\xd0\xbd\xd0\xb8\xd0\xb5"),
        juce::NormalisableRange<float>{ -1.0f, 1.0f, 0.01f }, 0.5f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ param::riaaNeumann, ver },
        u8("\xd0\x9f\xd0\xbe\xd0\xbb\xd1\x8e\xd1\x81\x20\x4e\x65\x75\x6d\x61\x6e\x6e"), true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::cartFreq, ver },
        u8("\xd0\xa0\xd0\xb5\xd0\xb7\xd0\xbe\xd0\xbd\xd0\xb0\xd0\xbd\xd1\x81\x20\xd0\xb3\xd0\xbe\xd0\xbb\xd0\xbe\xd0\xb2\xd0\xba\xd0\xb8"),
        logFreqRange(10000.0f, 18000.0f), 14000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::cartQ, ver },
        u8("\xd0\x94\xd0\xbe\xd0\xb1\xd1\x80\xd0\xbe\xd1\x82\xd0\xbd\xd0\xbe\xd1\x81\xd1\x82\xd1\x8c\x20\xd0\xb3\xd0\xbe\xd0\xbb\xd0\xbe\xd0\xb2\xd0\xba\xd0\xb8"),
        juce::NormalisableRange<float>{ 0.7f, 3.0f, 0.01f }, 1.2f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::crosstalk, ver },
        u8("\xd0\x9f\xd0\xb5\xd1\x80\xd0\xb5\xd0\xba\xd1\x80\xd1\x91\xd1\x81\xd1\x82\xd0\xbd\xd1\x8b\xd0\xb5\x20\xd0\xbf\xd0\xbe\xd0\xbc\xd0\xb5\xd1\x85\xd0\xb8"),
        juce::NormalisableRange<float>{ -40.0f, -20.0f, 0.1f }, -30.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::tubeTrim, ver },
        u8("\xd0\x94\xd1\x80\xd0\xb0\xd0\xb9\xd0\xb2\x20\xd0\xbb\xd0\xb0\xd0\xbc\xd0\xbf\xd1\x8b"),
        juce::NormalisableRange<float>{ 0.0f, 200.0f, 0.1f }, 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::tubeBias, ver },
        u8("\xd0\xa1\xd0\xbc\xd0\xb5\xd1\x89\xd0\xb5\xd0\xbd\xd0\xb8\xd0\xb5\x20\xd0\xbb\xd0\xb0\xd0\xbc\xd0\xbf\xd1\x8b"),
        juce::NormalisableRange<float>{ -1.0f, 1.0f, 0.01f }, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ param::tubeModel, ver },
        u8("\xd0\x9c\xd0\xbe\xd0\xb4\xd0\xb5\xd0\xbb\xd1\x8c\x20\xd0\xbb\xd0\xb0\xd0\xbc\xd0\xbf\xd1\x8b"),
        juce::StringArray{ "Koren", "Fast" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::xfmrSat, ver },
        u8("\xd0\x9d\xd0\xb0\xd1\x81\xd1\x8b\xd1\x89\xd0\xb5\xd0\xbd\xd0\xb8\xd0\xb5\x20\xd1\x82\xd1\x80\xd0\xb0\xd0\xbd\xd1\x81\xd1\x84\x2e"),
        percentRange(), 40.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ param::xfmrHF, ver },
        u8("\xd0\x92\xd0\xa7\x2d\xd1\x81\xd0\xbf\xd0\xb0\xd0\xb4\x20\xd1\x82\xd1\x80\xd0\xb0\xd0\xbd\xd1\x81\xd1\x84\x2e"),
        juce::NormalisableRange<float>{ 16000.0f, 24000.0f, 1.0f }, 20000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    // Not host-automatable (SPEC §7): still in APVTS for state / Generic editor.
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ param::osFactor, ver },
        u8("\xd0\x9f\xd0\xb5\xd1\x80\xd0\xb5\xd0\xb4\xd0\xb8\xd1\x81\xd0\xba\xd1\x80\xd0\xb5\xd1\x82\xd0\xb8\xd0\xb7\xd0\xb0\xd1\x86\xd0\xb8\xd1\x8f"),
        juce::StringArray{ "2x", "4x", "8x" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ param::adaa, ver }, "ADAA", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ param::offlineHQ, ver },
        u8("\x48\x51\x20\xd0\xbf\xd1\x80\xd0\xb8\x20\xd1\x80\xd0\xb5\xd0\xbd\xd0\xb4\xd0\xb5\xd1\x80\xd0\xb5"), false));

    return { params.begin(), params.end() };
}

} // namespace wg::plugin
