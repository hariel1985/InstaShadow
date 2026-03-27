#pragma once
#include <JuceHeader.h>
#include "OpticalCell.h"
#include "VCACompressor.h"
#include "TransformerSaturation.h"

class CompressorEngine
{
public:
    CompressorEngine();

    void prepare (double sampleRate, int samplesPerBlock);
    void processBlock (juce::AudioBuffer<float>& buffer);

    // --- Optical section (GUI → audio) ---
    std::atomic<float> optoThresholdDb  { -20.0f };
    std::atomic<float> optoGainDb       { 0.0f };
    std::atomic<float> optoScHpfHz      { 90.0f };
    std::atomic<bool>  optoBypass       { false };

    // --- Discrete VCA section ---
    std::atomic<float> vcaThresholdDb   { -20.0f };
    std::atomic<float> vcaGainDb        { 0.0f };
    std::atomic<int>   vcaRatioIndex    { 1 };
    std::atomic<int>   vcaAttackIndex   { 2 };
    std::atomic<int>   vcaReleaseIndex  { 2 };
    std::atomic<bool>  vcaBypass        { false };

    // --- Transformer ---
    std::atomic<int>   transformerType  { 0 };   // 0=Off, 1=Nickel, 2=Iron, 3=Steel

    // --- Output / Global ---
    std::atomic<float> outputGainDb     { 0.0f };
    std::atomic<bool>  stereoLink       { true };
    std::atomic<bool>  globalBypass     { false };

    // --- Metering (audio → GUI) ---
    std::atomic<float> optoGrDb         { 0.0f };
    std::atomic<float> vcaGrDb          { 0.0f };
    std::atomic<float> outputLevelL     { 0.0f };
    std::atomic<float> outputLevelR     { 0.0f };

    // Lookup tables
    static constexpr float ratios[]   = { 1.2f, 2.0f, 3.0f, 4.0f, 6.0f, 10.0f, 20.0f };
    static constexpr float attacks[]  = { 0.0001f, 0.0005f, 0.001f, 0.005f, 0.01f, 0.03f };
    static constexpr float releases[] = { 0.1f, 0.25f, 0.5f, 0.8f, 1.2f, -1.0f }; // -1 = Dual
    static constexpr int numRatios   = 7;
    static constexpr int numAttacks  = 6;
    static constexpr int numReleases = 6;

private:
    double currentSampleRate = 44100.0;

    // Per-channel DSP
    std::array<OpticalCell, 2> optoCells;
    std::array<VCACompressor, 2> vcaComps;
    TransformerSaturation transformer;

    // Sidechain HPF (2nd order, per channel)
    std::array<juce::dsp::IIR::Filter<float>, 2> scHpfFilters;
    float lastScHpfFreq = 0.0f;

    void updateScHpf (float freqHz);
};
