#pragma once
#include <JuceHeader.h>

class TransformerSaturation
{
public:
    enum Type { Off = 0, Nickel = 1, Iron = 2, Steel = 3 };

    TransformerSaturation();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    // Process buffer in-place with 4x oversampling
    void processBlock (juce::AudioBuffer<float>& buffer, Type type);

private:
    double sr = 44100.0;

    // 4x oversampler (stereo)
    juce::dsp::Oversampling<float> oversampler { 2, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };

    // Subtle tonestack (post-saturation)
    juce::dsp::IIR::Filter<float> ironBoostL, ironBoostR;
    juce::dsp::IIR::Filter<float> steelBoostL, steelBoostR;

    // Simple pre-emphasis for frequency-dependent saturation
    // Boost lows before waveshaper, compensate after
    juce::dsp::IIR::Filter<float> preEmphL, preEmphR;
    juce::dsp::IIR::Filter<float> deEmphL, deEmphR;

    void setupPreEmphasis (double sampleRate, float boostDb);
};
