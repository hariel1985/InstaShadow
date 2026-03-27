#pragma once
#include <JuceHeader.h>

// ============================================================
// Analog-style needle VU meter (semicircular, like Shadow Hills)
// ============================================================
class NeedleVuMeter : public juce::Component
{
public:
    void setLevel (float linearLevel)
    {
        // Convert to dB, map to needle position
        float db = (linearLevel > 0.0001f)
                 ? 20.0f * std::log10 (linearLevel)
                 : -60.0f;

        // VU range: -20 to +3 dB → 0.0 to 1.0
        float target = juce::jlimit (0.0f, 1.0f, (db + 20.0f) / 23.0f);

        // Smooth needle movement (ballistic)
        if (target > needlePos)
            needlePos += (target - needlePos) * 0.07f;   // slow attack (inertia)
        else
            needlePos += (target - needlePos) * 0.05f;   // moderate release

        repaint();
    }

    void setLabel (const juce::String& text) { label = text; }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2);
        float w = bounds.getWidth();
        float h = bounds.getHeight();

        // Meter face background (warm cream)
        float arcH = h * 0.85f;
        auto faceRect = bounds.withHeight (arcH);

        g.setColour (juce::Colour (0xff1a1a22));
        g.fillRoundedRectangle (bounds, 4.0f);

        // Cream arc area
        auto arcArea = faceRect.reduced (6, 4);
        {
            juce::ColourGradient grad (juce::Colour (0xfff0e8d0), arcArea.getCentreX(), arcArea.getY(),
                                        juce::Colour (0xffd8d0b8), arcArea.getCentreX(), arcArea.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (arcArea, 3.0f);
        }

        // Arc center point (bottom center of arc area)
        float cx = arcArea.getCentreX();
        float cy = arcArea.getBottom() - 4.0f;
        float radius = std::min (arcArea.getWidth() * 0.45f, arcArea.getHeight() * 0.8f);

        // Scale markings
        float startAngle = juce::MathConstants<float>::pi * 1.25f;  // -225 deg
        float endAngle   = juce::MathConstants<float>::pi * 1.75f;  // -315 deg (sweep right)

        // Draw scale ticks and labels
        g.setFont (std::max (6.0f, h * 0.045f));
        const float dbValues[] = { -20, -10, -7, -5, -3, -1, 0, 1, 2, 3 };
        const int numTicks = 10;

        for (int i = 0; i < numTicks; ++i)
        {
            float norm = (dbValues[i] + 20.0f) / 23.0f;
            float angle = startAngle + norm * (endAngle - startAngle);

            float cosA = std::cos (angle);
            float sinA = std::sin (angle);

            float innerR = radius * 0.82f;
            float outerR = radius * 0.95f;
            bool isMajor = (dbValues[i] == -20 || dbValues[i] == -10 || dbValues[i] == -5
                          || dbValues[i] == 0 || dbValues[i] == 3);

            // Tick line
            g.setColour (dbValues[i] >= 0 ? juce::Colour (0xffcc3333) : juce::Colour (0xff333333));
            float tickInner = isMajor ? innerR * 0.9f : innerR;
            g.drawLine (cx + cosA * tickInner, cy + sinA * tickInner,
                       cx + cosA * outerR, cy + sinA * outerR,
                       isMajor ? 1.5f : 0.8f);

            // Label for major ticks
            if (isMajor)
            {
                float labelR = radius * 0.7f;
                float lx = cx + cosA * labelR;
                float ly = cy + sinA * labelR;
                juce::String txt = (dbValues[i] > 0 ? "+" : "") + juce::String ((int) dbValues[i]);
                g.setColour (dbValues[i] >= 0 ? juce::Colour (0xffcc3333) : juce::Colour (0xff444444));
                g.drawText (txt, (int) (lx - 12), (int) (ly - 6), 24, 12, juce::Justification::centred);
            }
        }

        // Red zone arc (0 to +3 dB)
        {
            float redStart = startAngle + (20.0f / 23.0f) * (endAngle - startAngle);
            juce::Path redArc;
            redArc.addCentredArc (cx, cy, radius * 0.92f, radius * 0.92f, 0,
                                   redStart, endAngle, true);
            g.setColour (juce::Colour (0x33ff3333));
            g.strokePath (redArc, juce::PathStrokeType (radius * 0.08f));
        }

        // Needle
        {
            float angle = startAngle + needlePos * (endAngle - startAngle);
            float cosA = std::cos (angle);
            float sinA = std::sin (angle);

            // Needle shadow
            g.setColour (juce::Colours::black.withAlpha (0.3f));
            g.drawLine (cx + 1, cy + 1,
                       cx + cosA * radius * 0.88f + 1, cy + sinA * radius * 0.88f + 1,
                       2.0f);

            // Needle
            g.setColour (juce::Colour (0xff222222));
            g.drawLine (cx, cy,
                       cx + cosA * radius * 0.88f, cy + sinA * radius * 0.88f,
                       1.5f);

            // Needle pivot dot
            g.setColour (juce::Colour (0xff333333));
            g.fillEllipse (cx - 3, cy - 3, 6, 6);
        }

        // Label below
        g.setColour (juce::Colour (0xffaaaaaa));
        g.setFont (std::max (7.0f, h * 0.05f));
        g.drawText (label, bounds.getX(), bounds.getBottom() - h * 0.18f,
                    bounds.getWidth(), h * 0.15f, juce::Justification::centred);

        // Border
        g.setColour (juce::Colour (0xff333344));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
    }

private:
    float needlePos = 0.0f;  // 0..1 mapped to -20..+3 dB
    juce::String label;
};
