#pragma once
#include <JuceHeader.h>

class GRMeter : public juce::Component
{
public:
    void setGainReduction (float grDb)
    {
        float clamped = juce::jlimit (-30.0f, 0.0f, grDb);
        float normalised = -clamped / 30.0f;
        currentGr = std::max (normalised, currentGr * 0.92f);
        if (normalised > peakGr) peakGr = normalised;
        else peakGr *= 0.998f;
        repaint();
    }

    void setBarColour (juce::Colour c) { barColour = c; }
    void setLabel (const juce::String& text) { label = text; }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1);

        // Background
        g.setColour (juce::Colour (0xff111122));
        g.fillRoundedRectangle (bounds, 2.0f);

        // GR bar (fills from right to left)
        float w = bounds.getWidth() * currentGr;
        auto filled = bounds.withLeft (bounds.getRight() - w);
        g.setColour (barColour);
        g.fillRoundedRectangle (filled, 2.0f);

        // Peak hold line
        if (peakGr > 0.01f)
        {
            float peakX = bounds.getRight() - bounds.getWidth() * peakGr;
            g.setColour (juce::Colours::white.withAlpha (0.8f));
            g.fillRect (peakX, bounds.getY(), 1.5f, bounds.getHeight());
        }

        // Label
        g.setColour (juce::Colour (0xffe0e0e0).withAlpha (0.7f));
        g.setFont (11.0f);
        g.drawText (label, bounds.reduced (4, 0), juce::Justification::centredLeft);

        // dB readout
        float dbVal = -currentGr * 30.0f;
        if (currentGr > 0.001f)
            g.drawText (juce::String (dbVal, 1) + " dB", bounds.reduced (4, 0),
                        juce::Justification::centredRight);
    }

private:
    float currentGr = 0.0f;
    float peakGr = 0.0f;
    juce::Colour barColour { 0xffff8833 };
    juce::String label;
};
