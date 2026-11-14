#include "Parameters.h"

namespace
{
    juce::String formatDb (float v, int maxLen)
    {
        juce::ignoreUnused (maxLen);
        return juce::String (v, 1) + " dB";
    }

    juce::AudioParameterFloatAttributes dbAttributes()
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel ("dB")
            .withStringFromValueFunction (formatDb)
            .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); });
    }

    juce::NormalisableRange<float> hpfRange()
    {
        juce::NormalisableRange<float> r (20.0f, 500.0f);
        r.setSkewForCentre (100.0f);
        return r;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pid = [] (const char* id) { return ParameterID { id, 1 }; };

    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamIDs::optoThreshold), "Optical Threshold",
                                                       NormalisableRange<float> (-40.0f, 0.0f), -20.0f, dbAttributes()));
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamIDs::optoGain), "Optical Gain",
                                                       NormalisableRange<float> (0.0f, 20.0f), 0.0f, dbAttributes()));
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamIDs::optoScHpf), "Optical SC HPF",
                                                       hpfRange(), 90.0f,
                                                       AudioParameterFloatAttributes()
                                                           .withLabel ("Hz")
                                                           .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " Hz"; })
                                                           .withValueFromStringFunction ([] (const String& s) { return s.getFloatValue(); })));
    layout.add (std::make_unique<AudioParameterBool> (pid (ParamIDs::optoBypass), "Optical Bypass", false));

    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamIDs::vcaThreshold), "Discrete Threshold",
                                                       NormalisableRange<float> (-40.0f, 0.0f), -20.0f, dbAttributes()));
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamIDs::vcaGain), "Discrete Gain",
                                                       NormalisableRange<float> (0.0f, 20.0f), 0.0f, dbAttributes()));
    layout.add (std::make_unique<AudioParameterChoice> (pid (ParamIDs::vcaRatio), "Discrete Ratio",
                                                        ParamChoices::ratioNames, 1));
    layout.add (std::make_unique<AudioParameterChoice> (pid (ParamIDs::vcaAttack), "Discrete Attack",
                                                        ParamChoices::attackNames, 2));
    layout.add (std::make_unique<AudioParameterChoice> (pid (ParamIDs::vcaRelease), "Discrete Release",
                                                        ParamChoices::releaseNames, 2));
    layout.add (std::make_unique<AudioParameterBool> (pid (ParamIDs::vcaBypass), "Discrete Bypass", false));

    layout.add (std::make_unique<AudioParameterChoice> (pid (ParamIDs::transformerType), "Transformer",
                                                        ParamChoices::transformerNames, 0));
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamIDs::outputGain), "Output Gain",
                                                       NormalisableRange<float> (-12.0f, 12.0f), 0.0f, dbAttributes()));
    layout.add (std::make_unique<AudioParameterBool> (pid (ParamIDs::stereoLink), "Stereo Link", true));
    layout.add (std::make_unique<AudioParameterBool> (pid (ParamIDs::globalBypass), "Bypass", false));

    return layout;
}

float getParameterDefault (const juce::String& id)
{
    return getFactoryPresets().front().valueFor (id);
}

float FactoryPreset::valueFor (const juce::String& id) const
{
    if (id == ParamIDs::optoThreshold)   return optoThreshold;
    if (id == ParamIDs::optoGain)        return optoGain;
    if (id == ParamIDs::optoScHpf)       return optoScHpf;
    if (id == ParamIDs::optoBypass)      return optoBypass ? 1.0f : 0.0f;
    if (id == ParamIDs::vcaThreshold)    return vcaThreshold;
    if (id == ParamIDs::vcaGain)         return vcaGain;
    if (id == ParamIDs::vcaRatio)        return (float) vcaRatio;
    if (id == ParamIDs::vcaAttack)       return (float) vcaAttack;
    if (id == ParamIDs::vcaRelease)      return (float) vcaRelease;
    if (id == ParamIDs::vcaBypass)       return vcaBypass ? 1.0f : 0.0f;
    if (id == ParamIDs::transformerType) return (float) transformerType;
    if (id == ParamIDs::outputGain)      return outputGain;
    if (id == ParamIDs::stereoLink)      return stereoLink ? 1.0f : 0.0f;
    return 0.0f; // globalBypass: never part of a preset
}

const std::vector<FactoryPreset>& getFactoryPresets()
{
    // Ratio:   0=1.2  1=2  2=3  3=4  4=6  5=10  6=Flood
    // Attack:  0=0.1  1=0.5  2=1  3=5  4=10  5=30 ms
    // Release: 0=100  1=250  2=500  3=800 ms  4=1.2 s  5=Dual
    // Transformer: 0=Off 1=Nickel 2=Iron 3=Steel
    static const std::vector<FactoryPreset> presets {
        //  name                                  oThr    oGain  oHpf    oByp   vThr    vGain  rat att rel vByp   xfm  out    link
        { "Default",                              -20.0f, 0.0f,  90.0f,  false, -20.0f, 0.0f,  1,  2,  2,  false, 0,   0.0f, true },
        { "Mix Bus \xe2\x80\x94 Gentle Glue",      -20.0f, 3.0f,  90.0f,  false, -14.0f, 1.5f,  1,  2,  5,  false, 1,   0.5f, true },
        { "Master \xe2\x80\x94 Transparent",       -16.0f, 1.0f,  120.0f, false, -10.0f, 1.0f,  0,  5,  5,  false, 1,   0.0f, true },
        { "Master \xe2\x80\x94 Dense & Loud",      -24.0f, 3.0f,  90.0f,  false, -12.0f, 2.0f,  2,  3,  1,  false, 2,   0.0f, true },
        { "Drum Bus \xe2\x80\x94 Punch",           -22.0f, 2.0f,  60.0f,  false, -18.0f, 3.0f,  3,  5,  0,  false, 3,   0.0f, true },
        { "Vocal \xe2\x80\x94 Smooth Leveler",     -28.0f, 5.0f,  150.0f, false, -12.0f, 1.0f,  1,  3,  2,  false, 2,   0.0f, true },
        { "Bass \xe2\x80\x94 Tight & Even",        -22.0f, 3.0f,  30.0f,  false, -16.0f, 2.0f,  3,  4,  1,  false, 2,   0.0f, true },
        { "Optical Only \xe2\x80\x94 Warm",        -24.0f, 3.5f,  90.0f,  false, -20.0f, 0.0f,  1,  2,  2,  true,  2,   0.0f, true },
        { "Peak Catcher \xe2\x80\x94 Flood",       -20.0f, 0.0f,  90.0f,  true,  -6.0f,  0.0f,  6,  0,  0,  false, 1,   0.0f, true },
    };
    return presets;
}
