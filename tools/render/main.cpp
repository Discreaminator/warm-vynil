// wg_render — offline harness (SPEC §10.3)
#define DR_WAV_IMPLEMENTATION
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4456)
#endif
#include "dr_wav.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "dsp/Constants.h"
#include "dsp/Engine.h"
#include "dsp/EngineParams.h"
#include "dsp/util/MathUtil.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct Args
{
    std::string signal = "sine";
    double freq = 1000.0;
    double levelDb = -6.0;
    double sr = 48000.0;
    double seconds = 1.0;
    int os = 4;
    float amount = 50.0f; // percent
    int block = 512;
    std::string outPath = "out.wav";
    std::string csvPath;
    bool calibrate = false;
    bool mono = false;
};

void printUsage()
{
    std::fprintf(stderr,
                 "wg_render\n"
                 "  --signal sine|sweep|impulse|pinknoise|multitone\n"
                 "  --freq 1000 --level -6 --sr 48000 --seconds 1\n"
                 "  --os 4 --amount 50 --block 512\n"
                 "  --out out.wav [--csv spectrum.csv] [--calibrate] [--mono]\n");
}

bool parseArgs(int argc, char** argv, Args& a)
{
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        auto need = [&](const char* name) -> const char* {
            if (i + 1 >= argc)
            {
                std::fprintf(stderr, "Missing value for %s\n", name);
                return nullptr;
            }
            return argv[++i];
        };
        if (std::strcmp(arg, "--signal") == 0)
        {
            const char* v = need("--signal");
            if (!v)
                return false;
            a.signal = v;
        }
        else if (std::strcmp(arg, "--freq") == 0)
        {
            const char* v = need("--freq");
            if (!v)
                return false;
            a.freq = std::atof(v);
        }
        else if (std::strcmp(arg, "--level") == 0)
        {
            const char* v = need("--level");
            if (!v)
                return false;
            a.levelDb = std::atof(v);
        }
        else if (std::strcmp(arg, "--sr") == 0)
        {
            const char* v = need("--sr");
            if (!v)
                return false;
            a.sr = std::atof(v);
        }
        else if (std::strcmp(arg, "--seconds") == 0)
        {
            const char* v = need("--seconds");
            if (!v)
                return false;
            a.seconds = std::atof(v);
        }
        else if (std::strcmp(arg, "--os") == 0)
        {
            const char* v = need("--os");
            if (!v)
                return false;
            a.os = std::atoi(v);
        }
        else if (std::strcmp(arg, "--amount") == 0)
        {
            const char* v = need("--amount");
            if (!v)
                return false;
            a.amount = static_cast<float>(std::atof(v));
        }
        else if (std::strcmp(arg, "--block") == 0)
        {
            const char* v = need("--block");
            if (!v)
                return false;
            a.block = std::atoi(v);
        }
        else if (std::strcmp(arg, "--out") == 0)
        {
            const char* v = need("--out");
            if (!v)
                return false;
            a.outPath = v;
        }
        else if (std::strcmp(arg, "--csv") == 0)
        {
            const char* v = need("--csv");
            if (!v)
                return false;
            a.csvPath = v;
        }
        else if (std::strcmp(arg, "--calibrate") == 0)
            a.calibrate = true;
        else if (std::strcmp(arg, "--mono") == 0)
            a.mono = true;
        else if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0)
        {
            printUsage();
            std::exit(0);
        }
        else
        {
            std::fprintf(stderr, "Unknown arg: %s\n", arg);
            printUsage();
            return false;
        }
    }
    return true;
}

std::uint32_t xorshift(std::uint32_t& s)
{
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}

std::vector<float> genPink(int n, std::uint32_t seed, float amp)
{
    std::vector<float> out(static_cast<std::size_t>(n));
    double b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
    for (int i = 0; i < n; ++i)
    {
        const double white = (static_cast<int>(xorshift(seed) & 0xffffu) / 32768.0) - 1.0;
        b0 = 0.99886 * b0 + white * 0.0555179;
        b1 = 0.99332 * b1 + white * 0.0750759;
        b2 = 0.96900 * b2 + white * 0.1538520;
        b3 = 0.86650 * b3 + white * 0.3104856;
        b4 = 0.55000 * b4 + white * 0.5329522;
        b5 = -0.7616 * b5 - white * 0.0168980;
        const double pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362;
        b6 = white * 0.115926;
        out[static_cast<std::size_t>(i)] = amp * static_cast<float>(pink * 0.11);
    }
    return out;
}

std::vector<float> genSignal(const Args& a, int n)
{
    const float amp = static_cast<float>(wg::dsp::dB2lin(a.levelDb));
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    if (a.signal == "sine")
    {
        for (int i = 0; i < n; ++i)
            out[static_cast<std::size_t>(i)] =
                amp * static_cast<float>(std::sin(2.0 * wg::dsp::kPi * a.freq * i / a.sr));
    }
    else if (a.signal == "impulse")
    {
        if (n > 0)
            out[0] = amp;
    }
    else if (a.signal == "sweep")
    {
        const double f0 = 20.0, f1 = 20000.0;
        const double T = a.seconds;
        for (int i = 0; i < n; ++i)
        {
            const double t = i / a.sr;
            const double f = f0 * std::pow(f1 / f0, t / T);
            out[static_cast<std::size_t>(i)] =
                amp * static_cast<float>(std::sin(2.0 * wg::dsp::kPi * f * t));
        }
    }
    else if (a.signal == "pinknoise")
    {
        out = genPink(n, 0xC0FFEEu, amp);
    }
    else if (a.signal == "multitone")
    {
        for (int i = 0; i < n; ++i)
        {
            double s = 0.0;
            for (double f : { 100.0, 440.0, 1000.0, 2500.0, 5000.0 })
                s += std::sin(2.0 * wg::dsp::kPi * f * i / a.sr);
            out[static_cast<std::size_t>(i)] = amp * static_cast<float>(s / 5.0);
        }
    }
    else
    {
        std::fprintf(stderr, "Unsupported signal %s\n", a.signal.c_str());
    }
    return out;
}

int runCalibrate(const Args& args)
{
    // Pink −18 LUFS-ish; measure mean-square ratio in→out for a=0..1, print constexpr table.
    constexpr int n = 48000 * 5;
    auto pink = genPink(n, 0xA11CEu, static_cast<float>(wg::dsp::dB2lin(-18.0)));
    std::printf("// AutoGain static table (dB) from wg_render --calibrate\n");
    std::printf("constexpr double kStaticGainDb[11] = {\n");
    for (int k = 0; k <= 10; ++k)
    {
        const float amount = static_cast<float>(k) * 0.1f;
        wg::dsp::Engine e;
        wg::dsp::EngineParams p{};
        p.amount = amount;
        p.autoGain = wg::dsp::AutoGainMode::Off;
        p.osFactor = args.os;
        e.setTargets(p);
        e.prepare({ args.sr, args.block, 1 });
        std::vector<float> x = pink;
        int off = 0;
        while (off < n)
        {
            const int b = std::min(args.block, n - off);
            float* ch[1] = { x.data() + off };
            e.process(ch, 1, b);
            off += b;
        }
        double pin = 0.0, pout = 0.0;
        const int skip = n / 5;
        for (int i = skip; i < n; ++i)
        {
            pin += static_cast<double>(pink[static_cast<std::size_t>(i)])
                   * static_cast<double>(pink[static_cast<std::size_t>(i)]);
            pout += static_cast<double>(x[static_cast<std::size_t>(i)])
                    * static_cast<double>(x[static_cast<std::size_t>(i)]);
        }
        const double gDb = wg::dsp::lin2dB(std::sqrt(pin / std::max(pout, 1e-30)));
        std::printf("    %.6f%s\n", gDb, k < 10 ? "," : "");
    }
    std::printf("};\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    Args args;
    if (!parseArgs(argc, argv, args))
        return 1;

    if (args.calibrate)
        return runCalibrate(args);

    if (args.sr <= 0.0 || args.seconds <= 0.0)
        return 1;

    const auto numFrames = static_cast<int>(args.sr * args.seconds + 0.5);
    auto mono = genSignal(args, numFrames);
    const int chans = args.mono ? 1 : 2;

    wg::dsp::Engine engine;
    wg::dsp::EngineParams p{};
    p.amount = args.amount * 0.01f;
    p.osFactor = args.os;
    p.autoGain = wg::dsp::AutoGainMode::Off;
    p.adaa = false;
    engine.setTargets(p);
    engine.prepare({ args.sr, args.block, chans });

    std::vector<float> L = mono;
    std::vector<float> R = mono;
    int off = 0;
    while (off < numFrames)
    {
        const int b = std::min(args.block, numFrames - off);
        if (chans == 1)
        {
            float* ch[1] = { L.data() + off };
            engine.process(ch, 1, b);
        }
        else
        {
            float* ch[2] = { L.data() + off, R.data() + off };
            engine.process(ch, 2, b);
        }
        off += b;
    }

    std::vector<float> interleaved(static_cast<std::size_t>(numFrames) * static_cast<std::size_t>(chans));
    for (int i = 0; i < numFrames; ++i)
    {
        interleaved[static_cast<std::size_t>(i) * chans] = L[static_cast<std::size_t>(i)];
        if (chans == 2)
            interleaved[static_cast<std::size_t>(i) * 2u + 1u] = R[static_cast<std::size_t>(i)];
    }

    drwav_data_format format{};
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = static_cast<drwav_uint32>(chans);
    format.sampleRate = static_cast<drwav_uint32>(args.sr);
    format.bitsPerSample = 32;

    drwav wav;
    if (!drwav_init_file_write(&wav, args.outPath.c_str(), &format, nullptr))
    {
        std::fprintf(stderr, "Failed to open %s\n", args.outPath.c_str());
        return 1;
    }
    drwav_write_pcm_frames(&wav, static_cast<drwav_uint64>(numFrames), interleaved.data());
    drwav_uninit(&wav);
    std::printf("Wrote %s (%d frames, sr=%.0f, amount=%.1f%%, os=%d)\n", args.outPath.c_str(), numFrames,
                args.sr, args.amount, args.os);

    if (!args.csvPath.empty())
    {
        FILE* f = nullptr;
#if defined(_MSC_VER)
        if (fopen_s(&f, args.csvPath.c_str(), "w") != 0)
            f = nullptr;
#else
        f = std::fopen(args.csvPath.c_str(), "w");
#endif
        if (f)
        {
            std::fprintf(f, "index,sample\n");
            for (int i = 0; i < numFrames; ++i)
                std::fprintf(f, "%d,%.9g\n", i, L[static_cast<std::size_t>(i)]);
            std::fclose(f);
        }
    }
    return 0;
}
