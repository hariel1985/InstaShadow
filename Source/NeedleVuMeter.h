#pragma once
#include <JuceHeader.h>

// ============================================================
// Analog-style needle meter (semicircular)
// Two modes: VU (level) and GR (gain reduction)
// ============================================================
class NeedleVuMeter : public juce::Component
{
public:
    enum Mode { VU, GR };

    void setMode (Mode m) { mode = m; repaint(); }

    void setLevel (float linearLevel)
    {
        float db = (linearLevel > 0.0001f)
                 ? 20.0f * std::log10 (linearLevel)
                 : -60.0f;
        float target = juce::jlimit (0.0f, 1.0f, (db + 20.0f) / 23.0f);
        applyNeedlePhysics (target);
    }

    // For GR mode: pass negative dB value (e.g. -6.0 = 6dB reduction)
    // Standard VU scale, needle rests at 0dB mark, moves LEFT with compression
    void setGainReduction (float grDb)
    {
        float target = juce::jlimit (0.0f, 1.0f, (grDb + 20.0f) / 23.0f);
        applyNeedlePhysics (target);
    }

    void setLabel (const juce::String& text) { label = text; }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2);
        float h = bounds.getHeight();

        float arcH = h * 0.85f;
        auto faceRect = bounds.withHeight (arcH);

        // Dark background
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

        float cx = arcArea.getCentreX();
        float cy = arcArea.getBottom() - 4.0f;
        float radius = std::min (arcArea.getWidth() * 0.45f, arcArea.getHeight() * 0.8f);

        float startAngle = juce::MathConstants<float>::pi * 1.25f;
        float endAngle   = juce::MathConstants<float>::pi * 1.75f;

        g.setFont (std::max (6.0f, h * 0.045f));

        // Always use VU scale — in GR mode the needle just starts at 0 and goes left
        drawVuScale (g, cx, cy, radius, startAngle, endAngle);

        // Needle with leaf-shaped arrowhead
        {
            float angle = startAngle + needlePos * (endAngle - startAngle);
            float cosA = std::cos (angle);
            float sinA = std::sin (angle);

            float tipX = cx + cosA * radius * 0.88f;
            float tipY = cy + sinA * radius * 0.88f;

            // Needle shadow
            g.setColour (juce::Colours::black.withAlpha (0.25f));
            g.drawLine (cx + 1, cy + 1, tipX + 1, tipY + 1, 1.5f);

            // Needle shaft (thin line from pivot to base of arrowhead)
            float shaftEnd = radius * 0.65f;
            float shaftX = cx + cosA * shaftEnd;
            float shaftY = cy + sinA * shaftEnd;
            g.setColour (juce::Colour (0xff222222));
            g.drawLine (cx, cy, shaftX, shaftY, 1.2f);

            // Leaf-shaped arrowhead (elongated diamond from shaft end to tip)
            float leafW = radius * 0.035f;  // half-width of leaf
            float perpX = -sinA;  // perpendicular to needle direction
            float perpY = cosA;

            juce::Path leaf;
            leaf.startNewSubPath (shaftX, shaftY);                                    // base (narrow)
            leaf.lineTo (cx + cosA * radius * 0.76f + perpX * leafW,
                        cy + sinA * radius * 0.76f + perpY * leafW);                  // left bulge
            leaf.lineTo (tipX, tipY);                                                  // tip (narrow)
            leaf.lineTo (cx + cosA * radius * 0.76f - perpX * leafW,
                        cy + sinA * radius * 0.76f - perpY * leafW);                  // right bulge
            leaf.closeSubPath();

            // Shadow
            g.setColour (juce::Colours::black.withAlpha (0.2f));
            g.fillPath (leaf, juce::AffineTransform::translation (0.5f, 0.5f));

            // Fill
            g.setColour (juce::Colour (0xff111111));
            g.fillPath (leaf);

            // Pivot dot
            g.setColour (juce::Colour (0xff333333));
            g.fillEllipse (cx - 3, cy - 3, 6, 6);
        }

        // Label
        g.setColour (juce::Colour (0xffaaaaaa));
        g.setFont (std::max (7.0f, h * 0.05f));
        g.drawText (label, bounds.getX(), bounds.getBottom() - h * 0.18f,
                    bounds.getWidth(), h * 0.15f, juce::Justification::centred);

        g.setColour (juce::Colour (0xff333344));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
    }

private:
    Mode mode = VU;
    float needlePos = 0.0f;
    float needleVelocity = 0.0f;
    juce::String label;

    void applyNeedlePhysics (float target)
    {
        // VU mode: heavier needle, more damping (lazy, smooth movement)
        // GR mode: lighter needle, less damping (responsive to compression changes)
        float spring  = (mode == VU) ? 0.12f : 0.35f;
        float damping = (mode == VU) ? 0.70f : 0.55f;

        float force = spring * (target - needlePos);
        needleVelocity = needleVelocity * (1.0f - damping) + force;
        needlePos += needleVelocity;
        needlePos = juce::jlimit (0.0f, 1.05f, needlePos);

        repaint();
    }

    void drawVuScale (juce::Graphics& g, float cx, float cy, float radius,
                      float startAngle, float endAngle)
    {
        const float dbValues[] = { -20, -10, -7, -5, -3, -1, 0, 1, 2, 3 };

        for (int i = 0; i < 10; ++i)
        {
            float norm = (dbValues[i] + 20.0f) / 23.0f;
            float angle = startAngle + norm * (endAngle - startAngle);
            float cosA = std::cos (angle), sinA = std::sin (angle);

            float innerR = radius * 0.82f, outerR = radius * 0.95f;
            bool isMajor = (dbValues[i] == -20 || dbValues[i] == -10 || dbValues[i] == -5
                          || dbValues[i] == 0 || dbValues[i] == 3);

            g.setColour (dbValues[i] >= 0 ? juce::Colour (0xffcc3333) : juce::Colour (0xff333333));
            g.drawLine (cx + cosA * (isMajor ? innerR * 0.9f : innerR), cy + sinA * (isMajor ? innerR * 0.9f : innerR),
                       cx + cosA * outerR, cy + sinA * outerR, isMajor ? 1.5f : 0.8f);

            if (isMajor)
            {
                float lx = cx + cosA * radius * 0.7f, ly = cy + sinA * radius * 0.7f;
                juce::String txt = (dbValues[i] > 0 ? "+" : "") + juce::String ((int) dbValues[i]);
                g.setColour (dbValues[i] >= 0 ? juce::Colour (0xffcc3333) : juce::Colour (0xff444444));
                g.drawText (txt, (int) (lx - 12), (int) (ly - 6), 24, 12, juce::Justification::centred);
            }
        }

        // Red zone arc
        float redStart = startAngle + (20.0f / 23.0f) * (endAngle - startAngle);
        juce::Path redArc;
        redArc.addCentredArc (cx, cy, radius * 0.92f, radius * 0.92f, 0, redStart, endAngle, true);
        g.setColour (juce::Colour (0x33ff3333));
        g.strokePath (redArc, juce::PathStrokeType (radius * 0.08f));
    }

    void drawGrScale (juce::Graphics& g, float cx, float cy, float radius,
                      float startAngle, float endAngle)
    {
        // GR scale: 0 (left, rest) to -20 (right, max compression)
        const float grValues[] = { 0, -2, -4, -6, -8, -10, -14, -20 };

        for (int i = 0; i < 8; ++i)
        {
            float norm = -grValues[i] / 20.0f;  // 0→0.0, -20→1.0
            float angle = startAngle + norm * (endAngle - startAngle);
            float cosA = std::cos (angle), sinA = std::sin (angle);

            float innerR = radius * 0.82f, outerR = radius * 0.95f;
            bool isMajor = (grValues[i] == 0 || grValues[i] == -6 || grValues[i] == -10 || grValues[i] == -20);

            g.setColour (grValues[i] <= -10 ? juce::Colour (0xffcc3333) : juce::Colour (0xff333333));
            g.drawLine (cx + cosA * (isMajor ? innerR * 0.9f : innerR), cy + sinA * (isMajor ? innerR * 0.9f : innerR),
                       cx + cosA * outerR, cy + sinA * outerR, isMajor ? 1.5f : 0.8f);

            if (isMajor)
            {
                float lx = cx + cosA * radius * 0.7f, ly = cy + sinA * radius * 0.7f;
                juce::String txt = juce::String ((int) grValues[i]);
                g.setColour (grValues[i] <= -10 ? juce::Colour (0xffcc3333) : juce::Colour (0xff444444));
                g.drawText (txt, (int) (lx - 12), (int) (ly - 6), 24, 12, juce::Justification::centred);
            }
        }

        // Warning zone arc (-10 to -20 dB GR)
        float warnStart = startAngle + (10.0f / 20.0f) * (endAngle - startAngle);
        juce::Path warnArc;
        warnArc.addCentredArc (cx, cy, radius * 0.92f, radius * 0.92f, 0, warnStart, endAngle, true);
        g.setColour (juce::Colour (0x33ff3333));
        g.strokePath (warnArc, juce::PathStrokeType (radius * 0.08f));
    }
};
