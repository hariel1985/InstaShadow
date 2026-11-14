#pragma once
#include <JuceHeader.h>
#include "Theme.h"

// ============================================================
// Rotary knob, painted per theme. The value text is either inside
// the knob (Console) or in a separate ValueLabel (Graphite, Ivory).
// ============================================================
class Knob : public juce::Slider
{
public:
    enum class Accent { primary, secondary, grey, white };

    Knob();

    void setTheme (ThemeId t)         { theme = t; repaint(); }
    void setAccent (Accent a)         { accent = a; repaint(); }
    void setBipolar (bool b)          { bipolar = b; repaint(); }
    void setUnitText (juce::String u) { unit = std::move (u); repaint(); }
    void setLighterBody (bool b)      { lighterBody = b; repaint(); }

    // Text shown for a value (also used by ValueLabel).
    std::function<juce::String (double)> formatter;
    juce::String getDisplayText() const { return formatter ? formatter (getValue()) : juce::String (getValue(), 1); }

    void paint (juce::Graphics&) override;

private:
    ThemeId theme = ThemeId::graphite;
    Accent accent = Accent::primary;
    bool bipolar = false;
    bool lighterBody = false;
    juce::String unit;

    float angleForProportion (float p) const;
    juce::Colour accentColour() const;
    void paintGraphite (juce::Graphics&, juce::Rectangle<float>, float angle, float startAngle);
    void paintConsole  (juce::Graphics&, juce::Rectangle<float>, float angle, float startAngle);
    void paintIvory    (juce::Graphics&, juce::Rectangle<float>, float angle);
};

// Value text under a knob; double-click to type a value.
class ValueLabel : public juce::Label, private juce::Slider::Listener
{
public:
    explicit ValueLabel (Knob&);
    ~ValueLabel() override;

    void setStyle (const juce::Font&, juce::Colour);

private:
    Knob& knob;
    void sliderValueChanged (juce::Slider*) override;
    void textWasEdited() override;
    juce::TextEditor* createEditorComponent() override;
    void update();
};

// ============================================================
// Toggle buttons bound to a bool parameter.
// stageIn shows the stage as "in" when its bypass parameter is off.
// ============================================================
class ThemedToggle : public juce::Button
{
public:
    enum class Role { stageIn, link, bypass };

    ThemedToggle (Role, juce::RangedAudioParameter&, bool inverted);

    void setTheme (ThemeId t) { theme = t; repaint(); }
    bool isOn() const { return getToggleState(); }

    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    using juce::Button::clicked;
    void clicked() override;

private:
    Role role;
    ThemeId theme = ThemeId::graphite;
    bool inverted;
    juce::ParameterAttachment attachment;
};

// ============================================================
// Choice parameter selector: segmented buttons (Graphite, Ivory) or a
// dropdown row (Console).
// ============================================================
class ChoiceSelector : public juce::Component
{
public:
    ChoiceSelector (juce::RangedAudioParameter&, juce::StringArray shortNames,
                    juce::StringArray longNames, juce::String title);

    void setTheme (ThemeId t) { theme = t; repaint(); }
    int getIndex() const { return index; }

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    juce::RangedAudioParameter& param;
    juce::StringArray shortNames, longNames;
    juce::String title;
    ThemeId theme = ThemeId::graphite;
    int index = 0;
    int hover = -1;
    juce::ParameterAttachment attachment;

    juce::Rectangle<float> segmentBounds (int i) const;
    int segmentAt (juce::Point<float>) const;
    void choose (int i);
};

// ============================================================
// Transformer type: three buttons (Graphite), three cards (Console)
// or a rotary switch (Ivory). Clicking the selected type turns it off.
// ============================================================
class TransformerSelector : public juce::Component
{
public:
    explicit TransformerSelector (juce::RangedAudioParameter&);

    void setTheme (ThemeId t) { theme = t; repaint(); }
    int getIndex() const { return index; }

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    static juce::String description (int type);       // long: "Transparent, clean — minimal harmonics"
    static juce::String shortDescription (int type);  // "Transparent, clean"

private:
    juce::RangedAudioParameter& param;
    ThemeId theme = ThemeId::graphite;
    int index = 0;
    int dragStartIndex = 0;
    juce::ParameterAttachment attachment;

    juce::Rectangle<float> itemBounds (int type) const;   // type 1..3 (and 0 = Off for Ivory)
    juce::Point<float> ivoryKnobCentre() const;
    float ivoryAngle (int type) const;
    void choose (int type);
};

// ============================================================
// Header buttons (preset bar, A/B compare).
// ============================================================
class HeaderButton : public juce::Button
{
public:
    enum class Kind { previous, next, presetName, save, compareA, compareB };

    explicit HeaderButton (Kind);

    void setTheme (ThemeId t) { theme = t; repaint(); }

    // For presetName: the text, whether it is modified, and its 1-based number.
    void setPreset (const juce::String& name, bool modified, int number);
    // For compareA/B: whether this slot is active.
    void setActive (bool a) { if (a != active) { active = a; repaint(); } }

    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    Kind kind;
    ThemeId theme = ThemeId::graphite;
    juce::String presetName;
    bool modified = false;
    bool active = false;
    int number = 0;
};

// One of the three theme swatches in the header.
class ThemeSwatch : public juce::Button
{
public:
    explicit ThemeSwatch (ThemeId represents);

    void setTheme (ThemeId current) { if (current != theme) { theme = current; repaint(); } }
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    ThemeId represents;
    ThemeId theme = ThemeId::graphite;
};

// Chevron path (left/right/down) in a box.
juce::Path chevronPath (juce::Rectangle<float> box, int direction); // -1 left, 1 right, 0 down
