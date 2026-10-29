#pragma once

#include <juce_core/juce_core.h>

#include <cmath>

/** All UI strings (ru). Branding: WARM VYNIL / Discreaminator. */
namespace wg::ui::str {

inline const juce::String& brand()
{
    static const juce::String s{ "WARM VYNIL" };
    return s;
}

inline const juce::String& depth()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x93\xd0\x9b\xd0\xa3\xd0\x91\xd0\x98\xd0\x9d\xd0\x90") };
    return s;
}

inline const juce::String& out()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x92\xd0\xab\xd0\xa5\xd0\x9e\xd0\x94") };
    return s;
}

inline const juce::String& autoLevel()
{
    static const juce::String s{
        juce::CharPointer_UTF8("\xd0\x90\xd0\x92\xd0\xa2\xd0\x9e-\xd0\xa3\xd0\xa0\xd0\x9e\xd0\x92\xd0\x95\xd0\x9d\xd0\xac")
    };
    return s;
}

inline const juce::String& bypass()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x91\xd0\xb0\xd0\xb9\xd0\xbf\xd0\xb0\xd1\x81") };
    return s;
}

inline const juce::String& advanced()
{
    static const juce::String s{
        juce::CharPointer_UTF8("\xd0\x94\xd0\xbe\xd0\xbf\xd0\xbe\xd0\xbb\xd0\xbd\xd0\xb8\xd1\x82\xd0\xb5\xd0\xbb\xd1\x8c\xd0\xbd\xd0\xbe")
    };
    return s;
}

inline const juce::String& autoOff()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x92\xd1\x8b\xd0\xba\xd0\xbb") };
    return s;
}

inline const juce::String& autoStatic()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\xa1\xd1\x82\xd0\xb0\xd1\x82\xd0\xb8\xd1\x87.") };
    return s;
}

inline const juce::String& autoDyn()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x94\xd0\xb8\xd0\xbd\xd0\xb0\xd0\xbc.") };
    return s;
}

inline const juce::String& saveAs()
{
    static const juce::String s{
        juce::CharPointer_UTF8("\xd0\xa1\xd0\xbe\xd1\x85\xd1\x80\xd0\xb0\xd0\xbd\xd0\xb8\xd1\x82\xd1\x8c \xd0\xba\xd0\xb0\xd0\xba\xe2\x80\xa6")
    };
    return s;
}

inline const juce::String& removePreset()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\xa3\xd0\xb4\xd0\xb0\xd0\xbb\xd0\xb8\xd1\x82\xd1\x8c") };
    return s;
}

inline const juce::String& factoryGroup()
{
    static const juce::String s{
        juce::CharPointer_UTF8("\xd0\x97\xd0\xb0\xd0\xb2\xd0\xbe\xd0\xb4\xd1\x81\xd0\xba\xd0\xb8\xd0\xb5")
    };
    return s;
}

inline const juce::String& userGroup()
{
    static const juce::String s{
        juce::CharPointer_UTF8("\xd0\x9f\xd0\xbe\xd0\xbb\xd1\x8c\xd0\xb7\xd0\xbe\xd0\xb2\xd0\xb0\xd1\x82\xd0\xb5\xd0\xbb\xd1\x8c\xd1\x81\xd0\xba\xd0\xb8\xd0\xb5")
    };
    return s;
}

inline const juce::String& dbUnit()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\xb4\xd0\x91") };
    return s;
}

inline const juce::String& cardInMix()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x92\xd0\xa5\xd0\x9e\xd0\x94 \xc2\xb7 \xd0\x9c\xd0\x98\xd0\x9a\xd0\xa1") };
    return s;
}

inline const juce::String& cardMono()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x9c\xd0\x9e\xd0\x9d\xd0\x9e") };
    return s;
}

inline const juce::String& cardRiaa()
{
    static const juce::String s{ "RIAA" };
    return s;
}

inline const juce::String& cardCart()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x93\xd0\x9e\xd0\x9b\xd0\x9e\xd0\x92\xd0\x9a\xd0\x90") };
    return s;
}

inline const juce::String& cardTube()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x9b\xd0\x90\xd0\x9c\xd0\x9f\xd0\x90") };
    return s;
}

inline const juce::String& cardXfmr()
{
    static const juce::String s{
        juce::CharPointer_UTF8("\xd0\xa2\xd0\xa0\xd0\x90\xd0\x9d\xd0\xa1\xd0\xa4\xd0\x9e\xd0\xa0\xd0\x9c\xd0\x90\xd0\xa2\xd0\x9e\xd0\xa0")
    };
    return s;
}

inline const juce::String& cardQuality()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x9a\xd0\x90\xd0\xa7\xd0\x95\xd0\xa1\xd0\xa2\xd0\x92\xd0\x9e") };
    return s;
}

inline const juce::String& lblIn()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x92\xd1\x85\xd0\xbe\xd0\xb4") };
    return s;
}

inline const juce::String& lblMix()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\xa1\xd1\x83\xd1\x85./\xd0\xbe\xd0\xb1\xd1\x80\xd0\xb0\xd0\xb1.") };
    return s;
}

inline const juce::String& lblMonoFreq()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\xa7\xd0\xb0\xd1\x81\xd1\x82\xd0\xbe\xd1\x82\xd0\xb0") };
    return s;
}

inline const juce::String& lblMonoDepth()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x93\xd0\xbb\xd1\x83\xd0\xb1\xd0\xb8\xd0\xbd\xd0\xb0") };
    return s;
}

inline const juce::String& lblTracing()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x98\xd1\x81\xd0\xba. \xd0\xb8\xd0\xb3\xd0\xbb\xd1\x8b") };
    return s;
}

inline const juce::String& lblTilt()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\xa0\xd0\xb0\xd1\x81\xd1\x81\xd0\xbe\xd0\xb3\xd0\xbb.") };
    return s;
}

inline const juce::String& lblNeumann()
{
    static const juce::String s{ "Neumann" };
    return s;
}

inline const juce::String& lblCartFreq()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\xa0\xd0\xb5\xd0\xb7\xd0\xbe\xd0\xbd\xd0\xb0\xd0\xbd\xd1\x81") };
    return s;
}

inline const juce::String& lblCartQ()
{
    static const juce::String s{ "Q" };
    return s;
}

inline const juce::String& lblXtalk()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x9f\xd0\xbe\xd0\xbc\xd0\xb5\xd1\x85\xd0\xb8") };
    return s;
}

inline const juce::String& lblTubeDrive()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x94\xd1\x80\xd0\xb0\xd0\xb9\xd0\xb2") };
    return s;
}

inline const juce::String& lblTubeBias()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\xa1\xd0\xbc\xd0\xb5\xd1\x89\xd0\xb5\xd0\xbd\xd0\xb8\xd0\xb5") };
    return s;
}

inline const juce::String& lblTubeModel()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x9c\xd0\xbe\xd0\xb4\xd0\xb5\xd0\xbb\xd1\x8c") };
    return s;
}

inline const juce::String& lblXfmrSat()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x9d\xd0\xb0\xd1\x81\xd1\x8b\xd1\x89\xd0\xb5\xd0\xbd\xd0\xb8\xd0\xb5") };
    return s;
}

inline const juce::String& lblXfmrHF()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x92\xd0\xa7-\xd1\x81\xd0\xbf\xd0\xb0\xd0\xb4") };
    return s;
}

inline const juce::String& lblOs()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x9f\xd0\xb5\xd1\x80\xd0\xb5\xd0\xb4\xd0\xb8\xd1\x81\xd0\xba\xd1\x80.") };
    return s;
}

inline const juce::String& lblAdaa()
{
    static const juce::String s{ "ADAA" };
    return s;
}

inline const juce::String& lblOfflineHQ()
{
    static const juce::String s{ juce::CharPointer_UTF8("HQ \xd0\xbf\xd1\x80\xd0\xb8 \xd1\x80\xd0\xb5\xd0\xbd\xd0\xb4\xd0\xb5\xd1\x80\xd0\xb5") };
    return s;
}

inline const juce::String& scaleMenu()
{
    static const juce::String s{ juce::CharPointer_UTF8("\xd0\x9c\xd0\xb0\xd1\x81\xd1\x88\xd1\x82\xd0\xb0\xd0\xb1") };
    return s;
}

inline juce::String formatDb(float db)
{
    const juce::String minus{ juce::CharPointer_UTF8("\xe2\x88\x92") }; // U+2212
    juce::String num = juce::String(db, 1);
    if (db > 0.0f)
        num = "+" + num;
    else if (db < 0.0f)
        num = minus + juce::String(std::abs(db), 1);
    return num + " " + dbUnit();
}

inline juce::String formatHz(float hz)
{
    if (hz >= 1000.0f)
        return juce::String(hz / 1000.0f, hz >= 10000.0f ? 1 : 2) + " kHz";
    return juce::String(hz, hz >= 100.0f ? 0 : 1) + " Hz";
}

inline juce::String formatPct(float pct, int decimals = 0)
{
    return juce::String(pct, decimals) + " %";
}

} // namespace wg::ui::str
