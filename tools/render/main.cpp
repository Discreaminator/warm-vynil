// wg_render — M0: generate a sine WAV (32-bit float). Full DSP path arrives in M1.
#define DR_WAV_IMPLEMENTATION
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4456) // shadowing inside third-party dr_wav.h
#endif
#include "dr_wav.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif


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
    std::string outPath = "out.wav";
};

void printUsage()
{
    std::fprintf(stderr,
                 "wg_render (M0 stub)\n"
                 "  --signal sine\n"
                 "  --freq 1000\n"
                 "  --level -6\n"
                 "  --sr 48000\n"
                 "  --seconds 1\n"
                 "  --out out.wav\n");
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
        else if (std::strcmp(arg, "--out") == 0)
        {
            const char* v = need("--out");
            if (!v)
                return false;
            a.outPath = v;
        }
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

} // namespace

int main(int argc, char** argv)
{
    Args args;
    if (!parseArgs(argc, argv, args))
        return 1;

    if (args.signal != "sine")
    {
        std::fprintf(stderr, "M0 only supports --signal sine\n");
        return 1;
    }

    if (args.sr <= 0.0 || args.seconds <= 0.0)
    {
        std::fprintf(stderr, "Invalid --sr or --seconds\n");
        return 1;
    }

    const auto numFrames = static_cast<drwav_uint64>(args.sr * args.seconds + 0.5);
    const float amp = static_cast<float>(std::pow(10.0, args.levelDb / 20.0));
    const double twopi = 6.28318530717958647692;

    std::vector<float> interleaved(static_cast<size_t>(numFrames) * 2u);
    for (drwav_uint64 i = 0; i < numFrames; ++i)
    {
        const float s = amp * static_cast<float>(std::sin(twopi * args.freq * static_cast<double>(i) / args.sr));
        interleaved[static_cast<size_t>(i) * 2u + 0u] = s;
        interleaved[static_cast<size_t>(i) * 2u + 1u] = s;
    }

    drwav_data_format format{};
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = 2;
    format.sampleRate = static_cast<drwav_uint32>(args.sr);
    format.bitsPerSample = 32;

    drwav wav;
    if (!drwav_init_file_write(&wav, args.outPath.c_str(), &format, nullptr))
    {
        std::fprintf(stderr, "Failed to open %s for writing\n", args.outPath.c_str());
        return 1;
    }

    const drwav_uint64 written = drwav_write_pcm_frames(&wav, numFrames, interleaved.data());
    drwav_uninit(&wav);

    if (written != numFrames)
    {
        std::fprintf(stderr, "Short write (%llu / %llu)\n",
                     static_cast<unsigned long long>(written),
                     static_cast<unsigned long long>(numFrames));
        return 1;
    }

    std::printf("Wrote %s (%llu frames, %.1f Hz, %.1f dBFS sine @ %.1f Hz)\n",
                args.outPath.c_str(),
                static_cast<unsigned long long>(numFrames),
                args.sr,
                args.levelDb,
                args.freq);
    return 0;
}
