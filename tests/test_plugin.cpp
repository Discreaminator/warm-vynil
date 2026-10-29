#include "dsp/EngineParams.h"
#include "plugin/ParamBridge.h"
#include "plugin/ParameterIds.h"
#include "plugin/PluginProcessor.h"
#include "util/SignalGen.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

double lin2dB(double x) noexcept
{
    return 20.0 * std::log10(std::max(x, 1.0e-300));
}


void setFloat(wg::plugin::PluginProcessor& p, const char* id, float value)
{
    auto& apvts = p.getAPVTS();
    if (auto* param = apvts.getParameter(id))
        param->setValueNotifyingHost(param->convertTo0to1(value));
}

void setChoice(wg::plugin::PluginProcessor& p, const char* id, int index)
{
    auto& apvts = p.getAPVTS();
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(id)))
        param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(index)));
}

void setBool(wg::plugin::PluginProcessor& p, const char* id, bool on)
{
    auto& apvts = p.getAPVTS();
    if (auto* param = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(id)))
        param->setValueNotifyingHost(on ? 1.0f : 0.0f);
}

void processBuffer(wg::plugin::PluginProcessor& p, juce::AudioBuffer<float>& buf)
{
    juce::MidiBuffer midi;
    p.processBlock(buf, midi);
}

std::vector<float> renderMono(wg::plugin::PluginProcessor& p,
                              const std::vector<float>& input,
                              const std::vector<int>& blocks)
{
    std::vector<float> out = input;
    const int n = static_cast<int>(out.size());
    int off = 0;
    std::size_t bi = 0;
    while (off < n)
    {
        int bs = blocks.empty() ? 512 : blocks[bi % blocks.size()];
        bs = std::min(bs, n - off);
        juce::AudioBuffer<float> buf(1, bs);
        buf.copyFrom(0, 0, out.data() + off, bs);
        processBuffer(p, buf);
        for (int i = 0; i < bs; ++i)
            out[static_cast<std::size_t>(off + i)] = buf.getSample(0, i);
        off += bs;
        ++bi;
    }
    return out;
}

} // namespace

TEST_CASE("autoGain Off/Static/Dynamic indices via ParameterAttachment + ParamBridge",
          "[plugin][autoGain][mapping]")
{
    wg::plugin::PluginProcessor proc;
    auto* choice = dynamic_cast<juce::AudioParameterChoice*>(
        proc.getAPVTS().getParameter(wg::plugin::param::autoGain));
    REQUIRE(choice != nullptr);
    REQUIRE(choice->choices.size() == 3);

    wg::plugin::ParamBridge bridge(proc.getAPVTS());

    // Mimic SegmentedControl: ParameterAttachment wants denormalised choice indices.
    juce::ParameterAttachment attach(*choice, [](float) {}, nullptr);
    const wg::dsp::AutoGainMode expected[] = {
        wg::dsp::AutoGainMode::Off,
        wg::dsp::AutoGainMode::Static,
        wg::dsp::AutoGainMode::Dynamic,
    };

    for (int i = 0; i < 3; ++i)
    {
        attach.setValueAsCompleteGesture(static_cast<float>(i));
        REQUIRE(choice->getIndex() == i);
        REQUIRE(static_cast<int>(std::lround(proc.getAPVTS().getRawParameterValue(wg::plugin::param::autoGain)->load()))
                == i);
        REQUIRE(bridge.snapshot().autoGain == expected[i]);
    }

    // Guard against the old double-normalise bug: normalised Dynamic (1.0) must not stick as Static.
    attach.setValueAsCompleteGesture(choice->convertTo0to1(2.0f)); // wrong API usage
    REQUIRE(choice->getIndex() != 2);
}

TEST_CASE("T15 state get/set/get and factory presets", "[T15][plugin][state]")
{
    wg::plugin::PluginProcessor proc;
    setFloat(proc, wg::plugin::param::amount, 77.0f);
    setFloat(proc, wg::plugin::param::outGain, -3.0f);
    setChoice(proc, wg::plugin::param::autoGain, 2);

    juce::MemoryBlock blob;
    proc.getStateInformation(blob);
    REQUIRE(blob.getSize() > 0);

    wg::plugin::PluginProcessor proc2;
    proc2.setStateInformation(blob.getData(), static_cast<int>(blob.getSize()));

    auto near = [](float a, float b, float eps = 1.0e-3f) {
        return std::abs(a - b) <= eps;
    };

    REQUIRE(near(proc2.getAPVTS().getRawParameterValue(wg::plugin::param::amount)->load(), 77.0f));
    REQUIRE(near(proc2.getAPVTS().getRawParameterValue(wg::plugin::param::outGain)->load(), -3.0f));
    REQUIRE(static_cast<int>(std::lround(proc2.getAPVTS().getRawParameterValue(wg::plugin::param::autoGain)->load()))
            == 2);

    // Round-trip again
    juce::MemoryBlock blob2;
    proc2.getStateInformation(blob2);
    wg::plugin::PluginProcessor proc3;
    proc3.setStateInformation(blob2.getData(), static_cast<int>(blob2.getSize()));
    REQUIRE(near(proc3.getAPVTS().getRawParameterValue(wg::plugin::param::amount)->load(), 77.0f));

    REQUIRE(proc.getNumPrograms() == 4);
    proc.setCurrentProgram(2); // Классический винил
    REQUIRE(proc.getProgramName(2)
            == juce::String(juce::CharPointer_UTF8(
                "\xd0\x9a\xd0\xbb\xd0\xb0\xd1\x81\xd1\x81\xd0\xb8\xd1\x87\xd0\xb5\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9\x20\xd0\xb2\xd0\xb8\xd0\xbd\xd0\xb8\xd0\xbb")));
    REQUIRE(near(proc.getAPVTS().getRawParameterValue(wg::plugin::param::amount)->load(), 55.0f));
    REQUIRE(near(proc.getAPVTS().getRawParameterValue(wg::plugin::param::cartQ)->load(), 1.4f));

    proc.setCurrentProgram(0);
    REQUIRE(near(proc.getAPVTS().getRawParameterValue(wg::plugin::param::amount)->load(), 50.0f));
}

TEST_CASE("T2 plugin null at Amount 0", "[T2][plugin][null]")
{
    constexpr double fs = 48000.0;
    constexpr int n = 48000;

    wg::plugin::PluginProcessor proc;
    // Snap APVTS before prepare so Engine smoothers reset at Amount 0 (SPEC T2).
    setFloat(proc, wg::plugin::param::amount, 0.0f);
    setChoice(proc, wg::plugin::param::autoGain, 0); // Off
    setFloat(proc, wg::plugin::param::mix, 100.0f);
    setChoice(proc, wg::plugin::param::osFactor, 1); // 4x

    proc.setPlayConfigDetails(2, 2, fs, 512);
    proc.prepareToPlay(fs, 512);

    std::vector<float> L(n), R(n), inL(n), inR(n);
    wg::test::fillSine(L, fs, 1000.0, -12.0);
    R = L;
    inL = L;
    inR = R;

    for (int off = 0; off < n;)
    {
        const int bs = std::min(512, n - off);
        juce::AudioBuffer<float> buf(2, bs);
        for (int i = 0; i < bs; ++i)
        {
            buf.setSample(0, i, L[static_cast<std::size_t>(off + i)]);
            buf.setSample(1, i, R[static_cast<std::size_t>(off + i)]);
        }
        processBuffer(proc, buf);
        for (int i = 0; i < bs; ++i)
        {
            L[static_cast<std::size_t>(off + i)] = buf.getSample(0, i);
            R[static_cast<std::size_t>(off + i)] = buf.getSample(1, i);
        }
        off += bs;
    }

    const int lat = proc.getLatencySamples();
    const int settle = static_cast<int>(0.05 * fs) + lat + 16;
    double peak = 0.0;
    for (int i = settle; i < n; ++i)
    {
        const int j = i - lat;
        if (j < 0)
            continue;
        peak = std::max(peak, static_cast<double>(std::abs(L[static_cast<std::size_t>(i)] - inL[static_cast<std::size_t>(j)])));
        peak = std::max(peak, static_cast<double>(std::abs(R[static_cast<std::size_t>(i)] - inR[static_cast<std::size_t>(j)])));
    }
    const double db = lin2dB(peak);
    INFO("latency=" << lat << " peakDiffDb=" << db);
    REQUIRE(db < -120.0);
    proc.releaseResources();
}

TEST_CASE("T11 plugin latency impulse Amount 0", "[T11][plugin][latency]")
{
    constexpr double fs = 48000.0;
    constexpr int n = 8192;

    wg::plugin::PluginProcessor proc;
    setFloat(proc, wg::plugin::param::amount, 0.0f);
    setChoice(proc, wg::plugin::param::autoGain, 0);
    setChoice(proc, wg::plugin::param::osFactor, 1);

    proc.setPlayConfigDetails(1, 1, fs, 512);
    proc.prepareToPlay(fs, 512);

    const int lat = proc.getLatencySamples();
    REQUIRE(lat >= 0);

    juce::AudioBuffer<float> buf(1, n);
    buf.clear();
    buf.setSample(0, 0, 1.0f);
    processBuffer(proc, buf);

    int peakAt = 0;
    float peak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float v = std::abs(buf.getSample(0, i));
        if (v > peak)
        {
            peak = v;
            peakAt = i;
        }
    }
    INFO("lat=" << lat << " peakAt=" << peakAt << " peak=" << peak);
    REQUIRE(peakAt == lat);
    proc.releaseResources();
}

TEST_CASE("T7 plugin block-size independence", "[T7][plugin][blocks]")
{
    constexpr double fs = 48000.0;
    constexpr int n = 24000;
    std::vector<float> src(static_cast<std::size_t>(n));
    {
        std::vector<float> t1(src.size()), t2(src.size()), t3(src.size());
        wg::test::fillSine(t1, fs, 220.0, -18.0);
        wg::test::fillSine(t2, fs, 1000.0, -20.0, 0.3);
        wg::test::fillSine(t3, fs, 5000.0, -24.0, 1.1);
        for (std::size_t i = 0; i < src.size(); ++i)
            src[i] = t1[i] + t2[i] + t3[i];
    }

    auto render = [&](const std::vector<int>& blocks) {
        wg::plugin::PluginProcessor proc;
        setFloat(proc, wg::plugin::param::amount, 50.0f);
        setChoice(proc, wg::plugin::param::autoGain, 0);
        setChoice(proc, wg::plugin::param::osFactor, 1);
        proc.setPlayConfigDetails(1, 1, fs, 4096);
        proc.prepareToPlay(fs, 4096);
        auto out = renderMono(proc, src, blocks);
        proc.releaseResources();
        return out;
    };

    const auto ref = render({ 512 });
    const std::vector<std::vector<int>> cases = {
        { 1 }, { 7 }, { 32 }, { 64 }, { 4096 }, { 1, 7, 32, 64, 13, 97 }
    };
    for (const auto& blocks : cases)
        REQUIRE(render(blocks) == ref);
}

TEST_CASE("T6 plugin automation smoothness", "[T6][plugin][automation]")
{
    constexpr double fs = 48000.0;
    constexpr int block = 64;
    constexpr int total = static_cast<int>(fs); // 1 s

    wg::plugin::PluginProcessor proc;
    proc.setPlayConfigDetails(1, 1, fs, block);
    proc.prepareToPlay(fs, block);
    setChoice(proc, wg::plugin::param::autoGain, 0);
    setFloat(proc, wg::plugin::param::amount, 0.0f);

    std::vector<float> y;
    y.reserve(static_cast<std::size_t>(total));

    auto step = [&](float amountPercent) {
        setFloat(proc, wg::plugin::param::amount, amountPercent);
        juce::AudioBuffer<float> buf(1, block);
        for (int i = 0; i < block; ++i)
        {
            const double t = static_cast<double>(y.size() + i) / fs;
            buf.setSample(0, i, static_cast<float>(0.25 * std::sin(2.0 * juce::MathConstants<double>::pi * 1000.0 * t)));
        }
        processBuffer(proc, buf);
        for (int i = 0; i < block; ++i)
            y.push_back(buf.getSample(0, i));
    };

    // Steady a=0
    for (int i = 0; i < 20; ++i)
        step(0.0f);
    // Jump 0 → 100
    for (int i = 0; i < 40; ++i)
        step(100.0f);
    // Jump 100 → 0
    for (int i = 0; i < 40; ++i)
        step(0.0f);
    // outGain sweep
    for (int i = 0; i < 20; ++i)
    {
        setFloat(proc, wg::plugin::param::outGain, -24.0f);
        step(50.0f);
    }
    for (int i = 0; i < 20; ++i)
    {
        setFloat(proc, wg::plugin::param::outGain, 12.0f);
        step(50.0f);
    }

    // Random jumps (fixed seed 'WARM')
    juce::Random rnd{ 0x5741524Du };
    for (int i = 0; i < 200; ++i)
    {
        setFloat(proc, wg::plugin::param::amount, rnd.nextFloat() * 100.0f);
        setFloat(proc, wg::plugin::param::monoFreq, 40.0f + rnd.nextFloat() * 260.0f);
        if (rnd.nextBool())
            setBool(proc, wg::plugin::param::bypass, rnd.nextBool());
        step(proc.getAPVTS().getRawParameterValue(wg::plugin::param::amount)->load());
    }

    REQUIRE(y.size() > 100);
    double maxAbs = 0.0;
    double maxDelta = 0.0;
    for (std::size_t i = 0; i < y.size(); ++i)
    {
        REQUIRE(std::isfinite(y[i]));
        maxAbs = std::max(maxAbs, static_cast<double>(std::abs(y[i])));
        if (i > 0)
            maxDelta = std::max(maxDelta, static_cast<double>(std::abs(y[i] - y[i - 1])));
    }
    REQUIRE(maxAbs < 4.0);
    // Soft gate: smoothed params should keep sample deltas well below hard jumps (~0.5+)
    REQUIRE(maxDelta < 0.75);
    proc.releaseResources();
}
