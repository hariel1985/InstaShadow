#include "TransformerSaturation.h"

TransformerSaturation::TransformerSaturation() {}

void TransformerSaturation::prepare (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    oversampler.initProcessing ((size_t) samplesPerBlock);

    // Iron tonestack: very subtle low shelf +0.2dB at 110Hz
    auto ironCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        sampleRate, 110.0f, 0.707f, juce::Decibels::decibelsToGain (0.2f));
    ironBoostL.coefficients = ironCoeffs;
    ironBoostR.coefficients = ironCoeffs;

    // Steel tonestack: subtle low shelf +0.4dB at 40Hz
    auto steelCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        sampleRate, 40.0f, 0.707f, juce::Decibels::decibelsToGain (0.4f));
    steelBoostL.coefficients = steelCoeffs;
    steelBoostR.coefficients = steelCoeffs;
}

void TransformerSaturation::reset()
{
    oversampler.reset();
    ironBoostL.reset(); ironBoostR.reset();
    steelBoostL.reset(); steelBoostR.reset();
}

void TransformerSaturation::processBlock (juce::AudioBuffer<float>& buffer, Type type)
{
    if (type == Off) return;

    const int numChannels = std::min (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    // Select parameters per type:
    //   drive:  how hard the tanh clips (1.0 = no effect)
    //   even:   2nd harmonic amount (asymmetric warmth)
    //   odd:    3rd harmonic amount (edge/presence)
    //   mix:    wet/dry blend (keeps effect very subtle)
    float drive, even, odd, mix;

    switch (type)
    {
        case Nickel:
            drive = 1.05f;  even = 0.002f;  odd = 0.0005f; mix = 0.3f;
            break;
        case Iron:
            drive = 1.15f;  even = 0.008f;  odd = 0.002f;  mix = 0.5f;
            break;
        case Steel:
            drive = 1.3f;   even = 0.012f;  odd = 0.008f;  mix = 0.6f;
            break;
        default:
            return;
    }

    // 4x upsample
    juce::dsp::AudioBlock<float> block (buffer);
    auto oversampledBlock = oversampler.processSamplesUp (block);

    const int osNumSamples = (int) oversampledBlock.getNumSamples();
    const int osNumChannels = std::min ((int) oversampledBlock.getNumChannels(), 2);

    // Apply waveshaping at oversampled rate
    for (int ch = 0; ch < osNumChannels; ++ch)
    {
        float* data = oversampledBlock.getChannelPointer ((size_t) ch);

        for (int i = 0; i < osNumSamples; ++i)
        {
            float x = data[i];
            float dry = x;

            // Soft-clip: tanh(drive*x)/drive — unity gain at DC
            float shaped = std::tanh (drive * x) / drive;

            // Add subtle harmonic content
            shaped += even * x * std::abs (x);    // 2nd harmonic (even-order)
            shaped -= odd * x * x * x;            // 3rd harmonic (odd-order)

            // Blend dry/wet to keep the effect subtle
            data[i] = dry + (shaped - dry) * mix;
        }
    }

    // 4x downsample
    oversampler.processSamplesDown (block);

    // Post-saturation tonestack (very subtle EQ coloration)
    if (type == Iron)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            if (numChannels > 0) buffer.setSample (0, i, ironBoostL.processSample (buffer.getSample (0, i)));
            if (numChannels > 1) buffer.setSample (1, i, ironBoostR.processSample (buffer.getSample (1, i)));
        }
    }
    else if (type == Steel)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            if (numChannels > 0) buffer.setSample (0, i, steelBoostL.processSample (buffer.getSample (0, i)));
            if (numChannels > 1) buffer.setSample (1, i, steelBoostR.processSample (buffer.getSample (1, i)));
        }
    }
}
