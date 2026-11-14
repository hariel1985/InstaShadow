#pragma once
#include <JuceHeader.h>
#include "Theme.h"

// ============================================================
// Meters. Each meter is stepped by the editor's timer (30 Hz while
// something moves) and repaints itself only when what it shows has
// changed. update() returns true while the meter is still moving,
// so the editor can tell when every meter is at rest.
// ============================================================

enum class MeterMode { gainReduction, inputLevel };

// VU dial law, as drawn in the mockups: dB -> needle angle in degrees
// from vertical (-50 = left end at -20 dB, +50 = right end at +3 dB).
namespace VuLaw
{
    float angleForDb (float db);
    float dbForAngle (float degrees);
}

// Counts repaint requests (used by the tests to check idle behaviour).
struct RepaintCounter
{
    std::atomic<int> count { 0 };
};

// ------------------------------------------------------------
// Classic VU needle meter: scale at the top, needle pivot hidden
// behind a cover plate at the bottom, 4:3 face (Graphite, Ivory).
// ------------------------------------------------------------
class NeedleMeter : public juce::Component, public juce::SettableTooltipClient
{
public:
    // stageCaps: "OPTICAL", stageTitle: "Optical"
    NeedleMeter (juce::String stageCaps, juce::String stageTitle);

    void setTheme (ThemeId);
    void setMode (MeterMode, const juce::String& inputChannelName);

    // valueDb: gain reduction (<= 0) or input level in dBFS. Returns true while moving.
    bool update (float valueDb);

    // Resets the needle to rest (no animation).
    void reset();

    std::function<void()> onClick;
    RepaintCounter* repaintCounter = nullptr;

    float getNeedlePosition() const { return pos; }

    void paint (juce::Graphics&) override;
    void resized() override { invalidateCache(); }
    void mouseUp (const juce::MouseEvent&) override;

    // Size of the component for a theme (face plus caption row).
    static juce::Rectangle<int> faceBoundsFor (ThemeId, juce::Rectangle<int> component);

private:
    juce::String stageCaps, stageTitle, inputName;
    ThemeId theme = ThemeId::graphite;
    MeterMode mode = MeterMode::gainReduction;

    float pos = 0.0f, vel = 0.0f, target = 0.0f;
    float paintedPos = -1.0f;
    juce::String readout, paintedReadout;

    juce::Image under, over;
    float cacheScale = 0.0f;

    void invalidateCache() { under = {}; over = {}; cacheScale = 0.0f; }
    void buildCache (float scale);
    void drawUnder (juce::Graphics&);
    void drawOver (juce::Graphics&);
    juce::Rectangle<float> faceRect() const;
    juce::String captionText() const;
    juce::String makeReadout() const;
    float positionForDb (float db) const;
    void requestRepaint();
};

// ------------------------------------------------------------
// Segmented gain-reduction meter with a number (Console).
// ------------------------------------------------------------
class SegmentMeter : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit SegmentMeter (juce::Colour accent);

    void setMode (MeterMode, const juce::String& inputChannelName);
    bool update (float valueDb);
    void reset();

    std::function<void()> onClick;
    RepaintCounter* repaintCounter = nullptr;

    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Colour accent;
    MeterMode mode = MeterMode::gainReduction;
    juce::String inputName;

    float amount = 0.0f;      // GR mode: dB of reduction (>= 0); input mode: linear level
    int litSegments = 0;
    juce::String readout;

    int computeLit() const;
    juce::String makeReadout() const;
};

// ------------------------------------------------------------
// Output level: two bars (Graphite, Console) or two LED rows (Ivory).
// ------------------------------------------------------------
class OutputMeter : public juce::Component
{
public:
    OutputMeter();

    void setTheme (ThemeId t) { theme = t; repaint(); }
    bool update (float left, float right);
    void reset();

    RepaintCounter* repaintCounter = nullptr;

    void paint (juce::Graphics&) override;

private:
    ThemeId theme = ThemeId::graphite;
    float level[2] {};
    int shown[2] {};      // bar height in quarter pixels, or LEDs lit

    int quantise (float level) const;
    void paintBar (juce::Graphics&, juce::Rectangle<float>, float display);
};
