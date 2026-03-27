#pragma once
#include <JuceHeader.h>

class GRMeter : public juce::Component
{
public:
    void setGainReduction (float grDb)
    {
        // 0dB GR = 0.0 (empty), -30dB GR = 1.0 (full bar)
        // Bar fills from RIGHT to LEFT showing how much GR
        float clamped = juce::jlimit (-30.0f, 0.0f, grDb);
        float normalised = -clamped / 30.0f;  // 0dB→0.0, -30dB→1.0
        currentLevel = std::max (normalised, currentLevel * 0.92f);
        if (normalised > peakLevel) peakLevel = normalised;
        else peakLevel *= 0.998f;
        leftToRight = false;  // right-to-left
        repaint();
    }

    // Input level meter (left-to-right, linear level 0..1)
    void setInputLevel (float linearLevel)
    {
        float db = (linearLevel > 0.0001f) ? 20.0f * std::log10 (linearLevel) : -60.0f;
        // Map -30..0 dB to 0..1
        float normalised = juce::jlimit (0.0f, 1.0f, (db + 30.0f) / 30.0f);
        currentLevel = std::max (normalised, currentLevel * 0.92f);
        if (normalised > peakLevel) peakLevel = normalised;
        else peakLevel *= 0.998f;
        leftToRight = true;
        repaint();
    }

    void setBarColour (juce::Colour c) { barColour = c; }
    void setLabel (const juce::String& text) { label = text; }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1);

        g.setColour (juce::Colour (0xff111122));
        g.fillRoundedRectangle (bounds, 2.0f);

        float w = bounds.getWidth() * currentLevel;

        juce::Rectangle<float> filled;
        if (leftToRight)
            filled = bounds.withWidth (w);  // left to right for input level
        else
            filled = bounds.withLeft (bounds.getRight() - w);  // right to left for GR

        g.setColour (barColour);
        g.fillRoundedRectangle (filled, 2.0f);

        // Peak hold line
        if (peakLevel > 0.01f)
        {
            float peakX = leftToRight
                ? bounds.getX() + bounds.getWidth() * peakLevel
                : bounds.getRight() - bounds.getWidth() * peakLevel;
            g.setColour (juce::Colours::white.withAlpha (0.8f));
            g.fillRect (peakX - 0.75f, bounds.getY(), 1.5f, bounds.getHeight());
        }

        // Label
        g.setColour (juce::Colour (0xffe0e0e0).withAlpha (0.7f));
        g.setFont (11.0f);
        g.drawText (label, bounds.reduced (4, 0), juce::Justification::centredLeft);

        // dB readout
        if (currentLevel > 0.001f)
        {
            float dbVal = leftToRight
                ? (currentLevel * 30.0f - 30.0f)    // input: -30..0 dB
                : (-currentLevel * 30.0f);           // GR: 0..-30 dB
            g.drawText (juce::String (dbVal, 1) + " dB", bounds.reduced (4, 0),
                        juce::Justification::centredRight);
        }
    }

private:
    float currentLevel = 0.0f;
    float peakLevel = 0.0f;
    bool leftToRight = false;
    juce::Colour barColour { 0xffff8833 };
    juce::String label;
};
