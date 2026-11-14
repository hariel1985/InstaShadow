#pragma once
#include <JuceHeader.h>

// ============================================================
// Parameter IDs, ranges and factory presets.
//
// The IDs are the attribute names the v1.0/v1.1 state used, so a
// parameter means the same thing in old and new projects.
// ============================================================
namespace ParamIDs
{
    inline constexpr const char* optoThreshold   = "optoThreshold";
    inline constexpr const char* optoGain        = "optoGain";
    inline constexpr const char* optoScHpf       = "optoScHpf";
    inline constexpr const char* optoBypass      = "optoBypass";
    inline constexpr const char* vcaThreshold    = "vcaThreshold";
    inline constexpr const char* vcaGain         = "vcaGain";
    inline constexpr const char* vcaRatio        = "vcaRatio";
    inline constexpr const char* vcaAttack       = "vcaAttack";
    inline constexpr const char* vcaRelease      = "vcaRelease";
    inline constexpr const char* vcaBypass       = "vcaBypass";
    inline constexpr const char* transformerType = "transformerType";
    inline constexpr const char* outputGain      = "outputGain";
    inline constexpr const char* stereoLink      = "stereoLink";
    inline constexpr const char* globalBypass    = "globalBypass";

    // Every parameter, in a fixed order (used for state, presets and tests).
    inline constexpr const char* all[] = {
        optoThreshold, optoGain, optoScHpf, optoBypass,
        vcaThreshold, vcaGain, vcaRatio, vcaAttack, vcaRelease, vcaBypass,
        transformerType, outputGain, stereoLink, globalBypass
    };
    inline constexpr int numParams = (int) (sizeof (all) / sizeof (all[0]));

    // Presets store everything except the global bypass, which is a mixing
    // decision rather than part of a sound.
    inline bool isPresetParam (const juce::String& id) { return id != globalBypass; }
}

namespace ParamChoices
{
    // Long names (host display, dropdowns) and short names (segmented buttons).
    inline const juce::StringArray ratioNames    { "1.2:1", "2:1", "3:1", "4:1", "6:1", "10:1", "Flood" };
    inline const juce::StringArray ratioShort    { "1.2", "2", "3", "4", "6", "10", "FL" };
    inline const juce::StringArray attackNames   { "0.1 ms", "0.5 ms", "1 ms", "5 ms", "10 ms", "30 ms" };
    inline const juce::StringArray attackShort   { ".1", ".5", "1", "5", "10", "30" };
    inline const juce::StringArray releaseNames  { "100 ms", "250 ms", "500 ms", "800 ms", "1.2 s", "Dual" };
    inline const juce::StringArray releaseShort  { ".1", ".25", ".5", ".8", "1.2", "DL" };
    inline const juce::StringArray transformerNames { "Off", "Nickel", "Iron", "Steel" };
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// Default value (in the parameter's own units) of a parameter, by ID.
float getParameterDefault (const juce::String& paramID);

// ============================================================
// Factory presets
// ============================================================
struct FactoryPreset
{
    const char* name;
    float optoThreshold, optoGain, optoScHpf;
    bool  optoBypass;
    float vcaThreshold, vcaGain;
    int   vcaRatio, vcaAttack, vcaRelease;
    bool  vcaBypass;
    int   transformerType;
    float outputGain;
    bool  stereoLink;

    // Value of one preset parameter, in the parameter's own units.
    float valueFor (const juce::String& paramID) const;
};

const std::vector<FactoryPreset>& getFactoryPresets();
