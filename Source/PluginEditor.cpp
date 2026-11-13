#include "PluginEditor.h"

namespace
{
    juce::Rectangle<int> R (int x, int y, int w, int h) { return { x, y, w, h }; }
    juce::Rectangle<float> RF (float x, float y, float w, float h) { return { x, y, w, h }; }

    juce::RangedAudioParameter& param (juce::AudioProcessorValueTreeState& s, const char* id)
    {
        auto* p = s.getParameter (id);
        jassert (p != nullptr);
        return *p;
    }

    void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float radius, juce::Colour fill, juce::Colour border)
    {
        g.setColour (fill);
        g.fillRoundedRectangle (r, radius);
        g.setColour (border);
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    }

    // Text centred on x (tracking adds space after the last glyph; compensate for it).
    void centredText (juce::Graphics& g, const juce::String& text, const juce::Font& f, juce::Colour c,
                      float centreX, float y, float h, float trackingEm = 0.0f)
    {
        auto shift = trackingEm * f.getHeightInPoints() * 0.5f;
        drawText (g, text, f, c, { centreX - 150.0f + shift, y, 300.0f, h }, juce::Justification::centred);
    }

    juce::String versionShort()
    {
        juce::StringArray parts;
        parts.addTokens (JucePlugin_VersionString, ".", {});
        return parts.size() >= 2 ? parts[0] + "." + parts[1] : juce::String (JucePlugin_VersionString);
    }

    float gainToDb (float g) { return g > 1.0e-6f ? 20.0f * std::log10 (g) : -120.0f; }
}

// ============================================================
// ThemeLookAndFeel
// ============================================================

ThemeLookAndFeel::ThemeLookAndFeel() { setTheme (ThemeId::graphite); }

void ThemeLookAndFeel::setTheme (ThemeId t)
{
    theme = t;
    juce::Colour bg, text, dim, highlight, highlightText, outline;

    switch (t)
    {
        case ThemeId::graphite:
            bg = Graphite::header; text = Graphite::text; dim = Graphite::dim;
            highlight = Graphite::selBg; highlightText = Graphite::cream; outline = Graphite::btnBorder; break;
        case ThemeId::console:
            bg = Console::btnBg; text = Console::text; dim = Console::dim;
            highlight = Console::cyanBg; highlightText = Console::cyanSoft; outline = Console::btnBorder; break;
        case ThemeId::ivory:
            bg = Ivory::segBg; text = Ivory::ink; dim = Ivory::dim;
            highlight = Ivory::ink; highlightText = Ivory::face; outline = Ivory::btnBorder; break;
    }

    setColour (juce::PopupMenu::backgroundColourId, bg);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::headerTextColourId, dim);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, highlight);
    setColour (juce::PopupMenu::highlightedTextColourId, highlightText);
    setColour (juce::TooltipWindow::backgroundColourId, bg);
    setColour (juce::TooltipWindow::textColourId, text);
    setColour (juce::TooltipWindow::outlineColourId, outline);
    setColour (juce::TextEditor::backgroundColourId, bg.darker (0.1f));
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::TextEditor::outlineColourId, outline);
    setColour (juce::TextEditor::focusedOutlineColourId, dim);
    setColour (juce::TextEditor::highlightColourId, highlight.withAlpha (0.6f));
    setColour (juce::CaretComponent::caretColourId, text);
    setColour (juce::TextButton::buttonColourId, bg);
    setColour (juce::TextButton::buttonOnColourId, highlight);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, highlightText);
    setColour (juce::ComboBox::outlineColourId, outline);
    setColour (juce::Label::textColourId, text);
}

juce::Font ThemeLookAndFeel::getPopupMenuFont()
{
    switch (theme)
    {
        case ThemeId::graphite: return fonts().get (FontId::interRegular, 13.0f);
        case ThemeId::console:  return fonts().get (FontId::interRegular, 13.0f);
        case ThemeId::ivory:    return fonts().get (FontId::plexCondRegular, 14.0f);
    }
    return fonts().get (FontId::interRegular, 13.0f);
}

juce::Font ThemeLookAndFeel::getLabelFont (juce::Label& l)
{
    return l.getFont();
}

juce::Font ThemeLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return theme == ThemeId::ivory ? fonts().get (FontId::plexCondMedium, 12.0f) : fonts().get (FontId::interMedium, 12.0f);
}

juce::Rectangle<int> ThemeLookAndFeel::getTooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    auto font = getPopupMenuFont().withPointHeight (12.0f);
    auto w = juce::roundToInt (juce::GlyphArrangement::getStringWidth (font, text)) + 16;
    auto h = 24;
    return juce::Rectangle<int> (pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 24,
                                 pos.y > parent.getCentreY() ? pos.y - (h + 6) : pos.y + 6, w, h)
               .constrainedWithin (parent);
}

void ThemeLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    auto b = juce::Rectangle<int> (width, height).toFloat();
    g.setColour (findColour (juce::TooltipWindow::backgroundColourId));
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (findColour (juce::TooltipWindow::outlineColourId));
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);
    drawText (g, text, getPopupMenuFont().withPointHeight (12.0f), findColour (juce::TooltipWindow::textColourId),
              b, juce::Justification::centred);
}

// ============================================================
// InlineDialog
// ============================================================

InlineDialog::InlineDialog (juce::String t, int w, int h) : title (std::move (t)), boxWidth (w), boxHeight (h)
{
    setWantsKeyboardFocus (true);
}

void InlineDialog::open (ThemeId t)
{
    theme = t;
    setVisible (true);
    toFront (true);
    resized();
    repaint();
}

juce::Rectangle<int> InlineDialog::box() const
{
    return getLocalBounds().withSizeKeepingCentre (boxWidth, boxHeight);
}

juce::Rectangle<int> InlineDialog::contentArea() const
{
    auto b = box().reduced (18);
    b.removeFromTop (26);
    return b;
}

juce::Colour InlineDialog::textColour() const
{
    switch (theme)
    {
        case ThemeId::graphite: return Graphite::text;
        case ThemeId::console:  return Console::text;
        case ThemeId::ivory:    return Ivory::ink;
    }
    return Graphite::text;
}

juce::Font InlineDialog::bodyFont() const
{
    return theme == ThemeId::ivory ? fonts().get (FontId::plexCondRegular, 14.0f)
                                   : fonts().get (FontId::interRegular, 13.0f);
}

bool InlineDialog::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey) { cancel(); return true; }
    return false;
}

void InlineDialog::mouseDown (const juce::MouseEvent& e)
{
    if (! box().contains (e.getPosition()))
        cancel();
}

void InlineDialog::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.45f));

    juce::Colour bg, border, text;
    juce::Font titleFont = fonts().get (FontId::interSemiBold, 13.0f);
    switch (theme)
    {
        case ThemeId::graphite: bg = Graphite::panel; border = Graphite::gold.withAlpha (0.6f); text = Graphite::text;
                                titleFont = fonts().get (FontId::barlowSemiBold, 15.0f, 0.2f); break;
        case ThemeId::console:  bg = Console::panel; border = Console::btnBorder; text = Console::text; break;
        case ThemeId::ivory:    bg = Ivory::face; border = Ivory::ink; text = Ivory::ink;
                                titleFont = fonts().get (FontId::plexCondSemiBold, 12.0f, 0.3f); break;
    }

    auto b = box().toFloat();
    g.setColour (bg);
    g.fillRoundedRectangle (b, 10.0f);
    g.setColour (border);
    g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.0f);
    drawText (g, theme == ThemeId::console ? title : title.toUpperCase(), titleFont,
              theme == ThemeId::graphite ? Graphite::gold : text, b.reduced (18.0f, 14.0f).withHeight (18.0f));
}

// ============================================================
// SavePresetPanel
// ============================================================

SavePresetPanel::SavePresetPanel() : InlineDialog ("Save preset", 340, 140)
{
    addAndMakeVisible (nameEditor);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (cancelButton);

    nameEditor.setIndents (8, 6);
    nameEditor.onReturnKey = [this] { commit(); };
    nameEditor.onEscapeKey = [this] { cancel(); };
    saveButton.onClick = [this] { commit(); };
    cancelButton.onClick = [this] { cancel(); };
}

void SavePresetPanel::show (ThemeId t, const juce::String& suggestedName)
{
    nameEditor.setFont (t == ThemeId::ivory ? fonts().get (FontId::plexMonoMedium, 13.0f)
                                            : fonts().get (FontId::interRegular, 13.5f));
    nameEditor.setText (suggestedName, false);
    nameEditor.applyFontToAllText (nameEditor.getFont());
    open (t);
    nameEditor.grabKeyboardFocus();
    nameEditor.selectAll();
}

void SavePresetPanel::commit()
{
    auto name = nameEditor.getText().trim();
    if (name.isEmpty())
        return;
    setVisible (false);
    if (onSave) onSave (name);
}

void SavePresetPanel::resized()
{
    auto b = contentArea();
    nameEditor.setBounds (b.removeFromTop (32));
    b.removeFromTop (14);
    auto row = b.removeFromTop (30);
    saveButton.setBounds (row.removeFromRight (84));
    row.removeFromRight (8);
    cancelButton.setBounds (row.removeFromRight (84));
}

// ============================================================
// DeletePresetPanel
// ============================================================

DeletePresetPanel::DeletePresetPanel() : InlineDialog ("Delete preset", 360, 150)
{
    addAndMakeVisible (deleteButton);
    addAndMakeVisible (cancelButton);
    deleteButton.onClick = [this] { confirm(); };
    cancelButton.onClick = [this] { cancel(); };
}

void DeletePresetPanel::show (ThemeId t, const juce::String& name, std::function<void()> callback)
{
    presetName = name;
    onDelete = std::move (callback);
    open (t);
    grabKeyboardFocus();
}

void DeletePresetPanel::confirm()
{
    if (! isVisible())
        return;
    setVisible (false);
    if (auto f = std::exchange (onDelete, nullptr))
        f();
}

bool DeletePresetPanel::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::returnKey) { confirm(); return true; }
    return InlineDialog::keyPressed (k);
}

void DeletePresetPanel::resized()
{
    auto b = contentArea();
    auto row = b.removeFromBottom (30);
    deleteButton.setBounds (row.removeFromRight (84));
    row.removeFromRight (8);
    cancelButton.setBounds (row.removeFromRight (84));
}

void DeletePresetPanel::paint (juce::Graphics& g)
{
    InlineDialog::paint (g);

    auto area = contentArea().withTrimmedBottom (40).toFloat();
    g.setFont (bodyFont());
    g.setColour (textColour());
    auto quote = [] (juce::juce_wchar c) { return juce::String::charToString (c); };
    g.drawFittedText ("Delete " + quote (0x201c) + presetName + quote (0x201d) + "?\n"
                        + "The preset file is moved to the Trash.",
                      area.toNearestInt(), juce::Justification::topLeft, 3, 1.0f);
}

// ============================================================
// Editor
// ============================================================

InstaShadowEditor::InstaShadowEditor (InstaShadowProcessor& p)
    : AudioProcessorEditor (&p), processor (p), state (p.getState()), presets (p.getPresetManager()),
      linkToggle (ThemedToggle::Role::link, param (state, ParamIDs::stereoLink), false),
      bypassToggle (ThemedToggle::Role::bypass, param (state, ParamIDs::globalBypass), false),
      optoIn (ThemedToggle::Role::stageIn, param (state, ParamIDs::optoBypass), true),
      vcaIn (ThemedToggle::Role::stageIn, param (state, ParamIDs::vcaBypass), true),
      ratioSelector (param (state, ParamIDs::vcaRatio), ParamChoices::ratioShort, ParamChoices::ratioNames, "Ratio"),
      attackSelector (param (state, ParamIDs::vcaAttack), ParamChoices::attackShort, ParamChoices::attackNames, "Attack"),
      releaseSelector (param (state, ParamIDs::vcaRelease), ParamChoices::releaseShort, ParamChoices::releaseNames, "Release"),
      transformer (param (state, ParamIDs::transformerType))
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (content);

    for (auto* c : std::initializer_list<juce::Component*> {
             &prevButton, &nameButton, &nextButton, &saveButton, &compareA, &compareB,
             &swatches[0], &swatches[1], &swatches[2], &linkToggle, &bypassToggle,
             &optoIn, &optoThreshold, &optoGain, &optoHpf, &optoThresholdValue, &optoGainValue, &optoHpfValue,
             &optoNeedle, &optoSegments,
             &vcaIn, &vcaThreshold, &vcaGain, &vcaThresholdValue, &vcaGainValue,
             &ratioSelector, &attackSelector, &releaseSelector, &vcaNeedle, &vcaSegments,
             &transformer, &outGain, &outGainValue, &outMeter })
        content.addAndMakeVisible (c);

    content.addChildComponent (savePanel);
    content.addChildComponent (deletePanel);

    setupKnob (optoThreshold, ParamIDs::optoThreshold, nullptr);
    setupKnob (optoGain, ParamIDs::optoGain, nullptr);
    setupKnob (optoHpf, ParamIDs::optoScHpf, nullptr);
    setupKnob (vcaThreshold, ParamIDs::vcaThreshold, nullptr);
    setupKnob (vcaGain, ParamIDs::vcaGain, nullptr);
    setupKnob (outGain, ParamIDs::outputGain, nullptr);
    outGain.setBipolar (true);
    optoHpf.setAccent (Knob::Accent::grey);
    optoHpf.setLighterBody (true);
    vcaThreshold.setAccent (Knob::Accent::secondary);
    vcaGain.setAccent (Knob::Accent::secondary);
    outGain.setAccent (Knob::Accent::white);
    optoThreshold.setUnitText ("dB");
    vcaThreshold.setUnitText ("dB");

    // Header actions
    prevButton.onClick = [this] { presets.loadPrevious(); };
    nextButton.onClick = [this] { presets.loadNext(); };
    nameButton.onClick = [this] { showPresetMenu(); };
    saveButton.onClick = [this] { showSavePanel(); };
    compareA.onClick = [this] { presets.switchCompareSlot (0); };
    compareB.onClick = [this] { presets.switchCompareSlot (1); };
    for (int i = 0; i < numThemes; ++i)
        swatches[i].onClick = [this, i] { setTheme ((ThemeId) i); };

    savePanel.onSave = [this] (const juce::String& name) { presets.saveUserPreset (name); };

    optoNeedle.onClick = optoSegments.onClick = vcaNeedle.onClick = vcaSegments.onClick = [this] { toggleMeterMode(); };
    for (auto* m : { &optoNeedle, &vcaNeedle }) m->repaintCounter = &meterRepaints;
    for (auto* m : { &optoSegments, &vcaSegments }) m->repaintCounter = &meterRepaints;
    outMeter.repaintCounter = &meterRepaints;

    presets.addChangeListener (this);

    // Read the saved size first: setting the limits resizes the (still empty) editor.
    auto saved = processor.getEditorSize();

    setResizable (true, true);
    setResizeLimits (700, 378, 2000, 1080);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) designWidth / (double) designHeight);

    setTheme ((ThemeId) processor.getThemeIndex());
    updatePresetDisplay();

    if (saved.x >= 700 && saved.x <= 2000)
        setSize (saved.x, juce::roundToInt ((float) saved.x * designHeight / designWidth));
    else
        setSize (designWidth, designHeight);

    constructed = true;

    lastMeterSequence = processor.getEngine().meterSequence.load();
    setTimerRate (activeRate);
}

InstaShadowEditor::~InstaShadowEditor()
{
    stopTimer();
    presets.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void InstaShadowEditor::setupKnob (Knob& k, const char* id, std::function<juce::String (double)>)
{
    auto& p = param (state, id);
    knobAttachments.push_back (std::make_unique<juce::SliderParameterAttachment> (p, k));
    k.setDoubleClickReturnValue (true, (double) getParameterDefault (id));
    k.setTooltip (p.getName (64));
}

void InstaShadowEditor::updateKnobFormatters()
{
    bool console = theme == ThemeId::console;

    auto db = [console] (bool plus)
    {
        return [console, plus] (double v) { return formatNumber ((float) v, 1, plus) + (console ? "" : " dB"); };
    };

    optoThreshold.formatter = db (false);
    vcaThreshold.formatter  = db (false);
    optoGain.formatter      = console ? db (false) : db (true);
    vcaGain.formatter       = console ? db (false) : db (true);
    outGain.formatter       = db (true);
    optoHpf.formatter       = [console] (double v) { return juce::String (juce::roundToInt (v)) + (console ? "" : " Hz"); };

    for (auto* k : { &optoThreshold, &optoGain, &optoHpf, &vcaThreshold, &vcaGain, &outGain })
        k->repaint();

    optoThresholdValue.setText (optoThreshold.getDisplayText(), juce::dontSendNotification);
    optoGainValue.setText (optoGain.getDisplayText(), juce::dontSendNotification);
    optoHpfValue.setText (optoHpf.getDisplayText(), juce::dontSendNotification);
    vcaThresholdValue.setText (vcaThreshold.getDisplayText(), juce::dontSendNotification);
    vcaGainValue.setText (vcaGain.getDisplayText(), juce::dontSendNotification);
    outGainValue.setText (outGain.getDisplayText(), juce::dontSendNotification);
}

void InstaShadowEditor::setTheme (ThemeId t)
{
    theme = t;
    processor.setThemeIndex ((int) t);
    lookAndFeel.setTheme (t);

    for (auto& s : swatches) s.setTheme (t);
    for (auto* b : { &prevButton, &nameButton, &nextButton, &saveButton, &compareA, &compareB }) b->setTheme (t);
    for (auto* b : { &linkToggle, &bypassToggle, &optoIn, &vcaIn }) b->setTheme (t);
    for (auto* k : { &optoThreshold, &optoGain, &optoHpf, &vcaThreshold, &vcaGain, &outGain }) k->setTheme (t);
    for (auto* s : { &ratioSelector, &attackSelector, &releaseSelector }) s->setTheme (t);
    transformer.setTheme (t);
    optoNeedle.setTheme (t);
    vcaNeedle.setTheme (t);
    outMeter.setTheme (t);

    // Ivory shows no number on the output knob: show it while dragging.
    outGain.setPopupDisplayEnabled (t == ThemeId::ivory, false, &content);

    bool console = t == ThemeId::console;
    optoNeedle.setVisible (! console);
    vcaNeedle.setVisible (! console);
    optoSegments.setVisible (console);
    vcaSegments.setVisible (console);
    compareA.setVisible (console);
    compareB.setVisible (console);
    for (auto* l : { &optoThresholdValue, &optoGainValue, &optoHpfValue, &vcaThresholdValue, &vcaGainValue })
        l->setVisible (! console);
    outGainValue.setVisible (t == ThemeId::graphite);

    updateKnobFormatters();

    switch (t)
    {
        case ThemeId::graphite: layoutGraphite(); break;
        case ThemeId::console:  layoutConsole();  break;
        case ThemeId::ivory:    layoutIvory();    break;
    }

    resetMeters();
    content.invalidateBackground();
}

void InstaShadowEditor::resized()
{
    auto scale = std::min ((float) getWidth() / (float) designWidth, (float) getHeight() / (float) designHeight);
    content.setBounds (0, 0, designWidth, designHeight);
    content.setTransform (juce::AffineTransform::scale (scale));
    savePanel.setBounds (0, 0, designWidth, designHeight);
    deletePanel.setBounds (0, 0, designWidth, designHeight);
    if (constructed)
        processor.setEditorSize ({ getWidth(), getHeight() });
}

void InstaShadowEditor::Content::paint (juce::Graphics& g)
{
    ++owner.contentPaints;

    auto scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (background.isNull() || ! juce::approximatelyEqual (scale, backgroundScale))
    {
        background = juce::Image (juce::Image::RGB, juce::roundToInt ((float) getWidth() * scale),
                                  juce::roundToInt ((float) getHeight() * scale), false);
        juce::Graphics bg (background);
        bg.addTransform (juce::AffineTransform::scale (scale));
        owner.paintContent (bg);
        backgroundScale = scale;
        ++owner.backgroundRenders;
    }

    g.drawImageTransformed (background, juce::AffineTransform::scale (1.0f / scale));
}

void InstaShadowEditor::paint (juce::Graphics& g)
{
    // Only visible if the host gives a size off the aspect ratio.
    g.fillAll (theme == ThemeId::ivory ? Ivory::outer : theme == ThemeId::console ? Console::bg : Graphite::bg);
}

// ============================================================
// Presets
// ============================================================

void InstaShadowEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // A restored state may carry another theme.
    if (processor.getThemeIndex() != (int) theme)
        setTheme ((ThemeId) processor.getThemeIndex());

    updatePresetDisplay();
}

void InstaShadowEditor::updatePresetDisplay()
{
    auto index = presets.getCurrentIndex();
    nameButton.setPreset (presets.getCurrentName(), presets.isModified(), index >= 0 ? index + 1 : 0);
    compareA.setActive (presets.getCompareSlot() == 0);
    compareB.setActive (presets.getCompareSlot() == 1);
}

void InstaShadowEditor::showPresetMenu()
{
    presets.refresh();

    juce::PopupMenu menu, deleteMenu;
    const auto& entries = presets.getEntries();
    auto current = presets.getCurrentIndex();

    constexpr int saveId = 10000, folderId = 10001, deleteCurrentId = 10002, deleteBase = 20000;

    menu.addSectionHeader ("Factory presets");
    bool userHeader = false;
    for (int i = 0; i < (int) entries.size(); ++i)
    {
        if (! entries[(size_t) i].isFactory)
        {
            if (! userHeader)
                menu.addSectionHeader ("User presets");
            userHeader = true;
            deleteMenu.addItem (deleteBase + i, entries[(size_t) i].name, true, i == current);
        }
        menu.addItem (i + 1, entries[(size_t) i].name, true, i == current);
    }

    menu.addSeparator();
    menu.addItem (saveId, "Save as...");
    if (presets.canDelete (current))
        menu.addItem (deleteCurrentId, "Delete " + juce::String::charToString (0x201c) + entries[(size_t) current].name
                                         + juce::String::charToString (0x201d) + "...");
    menu.addSubMenu ("Delete user preset", deleteMenu, userHeader);
    menu.addItem (folderId, "Show preset folder");

    // Shown inside the editor (scaled with it): a long list scrolls there instead
    // of filling the screen, and hosts don't have to deal with another window.
    juce::Component::SafePointer<InstaShadowEditor> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&nameButton)
                                                  .withParentComponent (&content)
                                                  .withMinimumWidth (240)
                                                  .withMaximumNumColumns (1),
                        [safe, current] (int result)
                        {
                            if (safe == nullptr || result == 0)
                                return;
                            if (result == saveId)
                                safe->showSavePanel();
                            else if (result == deleteCurrentId)
                                safe->askToDeletePreset (current);
                            else if (result >= deleteBase)
                                safe->askToDeletePreset (result - deleteBase);
                            else if (result == folderId)
                            {
                                auto folder = safe->presets.getUserPresetFolder();
                                folder.createDirectory();
                                folder.revealToUser();
                            }
                            else
                                safe->presets.load (result - 1);
                        });
}

void InstaShadowEditor::askToDeletePreset (int index)
{
    if (! presets.canDelete (index))
        return;

    // Remember the file, not the index: the list may change before the answer.
    auto file = presets.getEntries()[(size_t) index].file;
    juce::Component::SafePointer<InstaShadowEditor> safe (this);
    deletePanel.show (theme, presets.getEntries()[(size_t) index].name, [safe, file]
    {
        if (safe == nullptr)
            return;
        safe->presets.refresh();
        safe->presets.deleteUserPreset (safe->presets.findUserPreset (file));
    });
}

void InstaShadowEditor::showSavePanel()
{
    auto name = presets.getCurrentName();
    const auto& entries = presets.getEntries();
    auto index = presets.getCurrentIndex();
    if (index >= 0 && entries[(size_t) index].isFactory)
        name = name + " (copy)";
    savePanel.show (theme, name);
}

// ============================================================
// Meters
// ============================================================

void InstaShadowEditor::setTimerRate (int hz)
{
    if (hz != timerRate)
    {
        timerRate = hz;
        startTimerHz (hz);
    }
}

void InstaShadowEditor::resetMeters()
{
    optoNeedle.reset();
    vcaNeedle.reset();
    optoSegments.reset();
    vcaSegments.reset();
    outMeter.reset();
    idleTicks = 0;
    setTimerRate (activeRate);
}

void InstaShadowEditor::setMeterMode (MeterMode m)
{
    meterMode = m;
    bool gr = m == MeterMode::gainReduction;
    optoNeedle.setMode (m, gr ? "" : "Input L");
    vcaNeedle.setMode (m, gr ? "" : "Input R");
    optoSegments.setMode (m, gr ? "" : "Input L");
    vcaSegments.setMode (m, gr ? "" : "Input R");
    resetMeters();
}

void InstaShadowEditor::toggleMeterMode()
{
    setMeterMode (meterMode == MeterMode::gainReduction ? MeterMode::inputLevel : MeterMode::gainReduction);
}

void InstaShadowEditor::timerCallback()
{
    auto& e = processor.getEngine();

    // If the host stops processing, the last values stay in the engine:
    // after a moment, let the meters fall back to rest instead.
    auto seq = e.meterSequence.load();
    if (seq != lastMeterSequence) { lastMeterSequence = seq; staleTicks = 0; }
    else if (staleTicks < 1000) ++staleTicks;
    bool live = staleTicks * 1000 < 300 * timerRate;   // fresh within ~300 ms

    float optoGr = live ? e.optoGrDb.load() : 0.0f;
    float vcaGr  = live ? e.vcaGrDb.load() : 0.0f;
    float inL    = live ? e.inputLevelL.load() : 0.0f;
    float inR    = live ? e.inputLevelR.load() : 0.0f;
    float outL   = live ? e.outputLevelL.load() : 0.0f;
    float outR   = live ? e.outputLevelR.load() : 0.0f;

    bool gr = meterMode == MeterMode::gainReduction;
    float optoValue = gr ? optoGr : gainToDb (inL);
    float vcaValue  = gr ? vcaGr  : gainToDb (inR);

    bool moving = false;
    if (theme == ThemeId::console)
    {
        moving |= optoSegments.update (optoValue);
        moving |= vcaSegments.update (vcaValue);
    }
    else
    {
        moving |= optoNeedle.update (optoValue);
        moving |= vcaNeedle.update (vcaValue);
    }
    moving |= outMeter.update (outL, outR);

    // At rest: keep polling slowly (no repaints happen while nothing changes).
    if (moving)
    {
        idleTicks = 0;
        setTimerRate (activeRate);
    }
    else if (++idleTicks > activeRate / 2)
    {
        setTimerRate (idleRate);
    }
}

// ============================================================
// Layouts (design coordinates from the mockups)
// ============================================================

void InstaShadowEditor::layoutGraphite()
{
    prevButton.setBounds (R (288, 14, 32, 32));
    nameButton.setBounds (R (326, 14, 210, 32));
    nextButton.setBounds (R (542, 14, 32, 32));
    saveButton.setBounds (R (584, 14, 58, 32));
    swatches[0].setBounds (R (715, 20, 20, 20));
    swatches[1].setBounds (R (741, 20, 20, 20));
    swatches[2].setBounds (R (767, 20, 20, 20));
    linkToggle.setBounds (R (801, 14, 73, 32));
    bypassToggle.setBounds (R (884, 14, 94, 32));

    optoIn.setBounds (R (196, 93, 43, 22));
    optoThreshold.setBounds (R (86, 185, 104, 104));
    optoGain.setBounds (R (57, 403, 62, 62));
    optoHpf.setBounds (R (158, 403, 62, 62));
    optoThresholdValue.setBounds (R (78, 296, 120, 18));
    optoGainValue.setBounds (R (48, 471, 80, 16));
    optoHpfValue.setBounds (R (149, 471, 80, 16));

    optoNeedle.setBounds (R (287, 91, 206, 176));
    vcaNeedle.setBounds (R (507, 91, 206, 176));

    transformer.setBounds (R (289, 389, 210, 118));
    outGain.setBounds (R (583, 424, 62, 62));
    outGainValue.setBounds (R (574, 491, 80, 16));
    outMeter.setBounds (R (693, 313, 18, 195));

    vcaIn.setBounds (R (920, 93, 43, 22));
    vcaThreshold.setBounds (R (775, 172, 84, 84));
    vcaGain.setBounds (R (887, 194, 62, 62));
    vcaThresholdValue.setBounds (R (767, 262, 100, 17));
    vcaGainValue.setBounds (R (878, 262, 80, 16));
    ratioSelector.setBounds (R (761, 369, 202, 26));
    attackSelector.setBounds (R (761, 424, 202, 26));
    releaseSelector.setBounds (R (761, 479, 202, 26));

    optoThresholdValue.setStyle (fonts().get (FontId::plexMonoMedium, 13.0f), Graphite::text);
    vcaThresholdValue.setStyle (fonts().get (FontId::plexMonoMedium, 12.0f), Graphite::text);
    for (auto* l : { &optoGainValue, &optoHpfValue, &vcaGainValue, &outGainValue })
        l->setStyle (fonts().get (FontId::plexMonoMedium, 11.5f), Graphite::text);
}

void InstaShadowEditor::layoutConsole()
{
    nameButton.setBounds (R (242, 12, 280, 34));
    prevButton.setBounds (R (530, 12, 34, 34));
    nextButton.setBounds (R (572, 12, 34, 34));
    saveButton.setBounds (R (614, 12, 60, 34));
    compareA.setBounds (R (688, 11, 36, 36));
    compareB.setBounds (R (723, 11, 36, 36));
    swatches[0].setBounds (R (781, 19, 20, 20));
    swatches[1].setBounds (R (807, 19, 20, 20));
    swatches[2].setBounds (R (833, 19, 20, 20));
    linkToggle.setBounds (R (867, 12, 71, 34));
    bypassToggle.setBounds (R (946, 12, 34, 34));

    optoIn.setBounds (R (207, 93, 38, 24));
    optoSegments.setBounds (R (37, 148, 208, 60));
    optoThreshold.setBounds (R (85, 246, 112, 112));
    optoGain.setBounds (R (55, 419, 64, 64));
    optoHpf.setBounds (R (164, 419, 64, 64));

    vcaIn.setBounds (R (521, 93, 38, 24));
    vcaSegments.setBounds (R (301, 148, 258, 60));
    vcaThreshold.setBounds (R (315, 224, 96, 96));
    vcaGain.setBounds (R (461, 248, 72, 72));
    ratioSelector.setBounds (R (301, 391, 258, 32));
    attackSelector.setBounds (R (301, 431, 258, 32));
    releaseSelector.setBounds (R (301, 471, 258, 32));

    transformer.setBounds (R (615, 140, 150, 190));
    outGain.setBounds (R (842, 413, 64, 64));
    outMeter.setBounds (R (941, 140, 22, 345));
}

void InstaShadowEditor::layoutIvory()
{
    prevButton.setBounds (R (303, 24, 30, 30));
    nameButton.setBounds (R (339, 23, 220, 32));
    nextButton.setBounds (R (565, 24, 30, 30));
    saveButton.setBounds (R (605, 24, 61, 30));
    swatches[0].setBounds (R (751, 29, 20, 20));
    swatches[1].setBounds (R (777, 29, 20, 20));
    swatches[2].setBounds (R (803, 29, 20, 20));
    linkToggle.setBounds (R (839, 20, 44, 24));
    bypassToggle.setBounds (R (897, 20, 44, 24));

    optoIn.setBounds (R (231, 89, 28, 16));
    optoThreshold.setBounds (R (103, 187, 112, 112));
    optoGain.setBounds (R (59, 411, 76, 76));
    optoHpf.setBounds (R (183, 411, 76, 76));
    optoThresholdValue.setBounds (R (109, 320, 100, 16));
    optoGainValue.setBounds (R (57, 507, 80, 16));
    optoHpfValue.setBounds (R (181, 507, 80, 16));

    optoNeedle.setBounds (R (281, 89, 196, 170));
    vcaNeedle.setBounds (R (491, 89, 196, 170));

    transformer.setBounds (R (281, 436, 150, 104));
    outMeter.setBounds (R (493, 483, 114, 19));
    outGain.setBounds (R (621, 440, 66, 66));

    vcaIn.setBounds (R (913, 89, 28, 16));
    vcaThreshold.setBounds (R (709, 207, 96, 96));
    vcaGain.setBounds (R (865, 227, 76, 76));
    vcaThresholdValue.setBounds (R (707, 323, 100, 16));
    vcaGainValue.setBounds (R (863, 323, 80, 16));
    ratioSelector.setBounds (R (769, 434, 172, 24));
    attackSelector.setBounds (R (769, 466, 172, 24));
    releaseSelector.setBounds (R (769, 498, 172, 24));

    for (auto* l : { &optoThresholdValue, &vcaThresholdValue })
        l->setStyle (fonts().get (FontId::plexMonoMedium, 11.0f), Ivory::dim);
    for (auto* l : { &optoGainValue, &optoHpfValue, &vcaGainValue })
        l->setStyle (fonts().get (FontId::plexMonoMedium, 10.5f), Ivory::dim);
}

// ============================================================
// Static painting (everything that does not change with audio)
// ============================================================

void InstaShadowEditor::paintContent (juce::Graphics& g)
{
    switch (theme)
    {
        case ThemeId::graphite: paintGraphite (g); break;
        case ThemeId::console:  paintConsole (g);  break;
        case ThemeId::ivory:    paintIvory (g);    break;
    }
}

void InstaShadowEditor::paintGraphite (juce::Graphics& g)
{
    using namespace Graphite;
    g.fillAll (bg);

    // Header
    g.setColour (header);
    g.fillRect (0, 0, designWidth, 60);
    g.setColour (headerLine);
    g.fillRect (0, 59, designWidth, 1);

    auto logo = fonts().get (FontId::barlowSemiBold, 24.0f, 0.16f);
    juce::GlyphArrangement ga;
    ga.addLineOfText (logo, "INSTA", 22.0f, 33.0f);
    auto instaWidth = ga.getBoundingBox (0, -1, true).getWidth() + 0.16f * 24.0f;
    g.setFont (logo);
    g.setColour (text);
    g.drawSingleLineText ("INSTA", 22, 33);
    g.setColour (gold);
    g.drawSingleLineText ("SHADOW", 22 + juce::roundToInt (instaWidth), 33);
    drawText (g, "DUAL-STAGE MASTERING COMPRESSOR", fonts().get (FontId::interRegular, 9.0f, 0.18f), dim, RF (22, 38, 260, 12));

    // Panels
    drawPanel (g, RF (20, 76, 236, 446), 10.0f, Graphite::panel, panelBorder);
    drawPanel (g, RF (272, 76, 456, 206), 10.0f, meterBox, meterBorder);
    drawPanel (g, RF (272, 298, 244, 225), 10.0f, Graphite::panel, panelBorder);
    drawPanel (g, RF (532, 298, 196, 225), 10.0f, Graphite::panel, panelBorder);
    drawPanel (g, RF (744, 76, 236, 446), 10.0f, Graphite::panel, panelBorder);

    auto title = fonts().get (FontId::barlowSemiBold, 15.0f, 0.24f);
    drawText (g, "OPTICAL", title, gold, RF (37, 94, 150, 20));
    drawText (g, "TRANSFORMER", title, gold, RF (289, 312, 200, 20));
    drawText (g, "OUTPUT", title, gold, RF (549, 312, 130, 20));
    drawText (g, "DISCRETE", title, gold, RF (761, 94, 150, 20));

    auto caption = fonts().get (FontId::interRegular, 10.0f, 0.2f);
    auto small = fonts().get (FontId::interRegular, 9.5f, 0.2f);
    centredText (g, "THRESHOLD", caption, dim, 138.0f, 321.0f, 13.0f, 0.2f);
    centredText (g, "GAIN", small, dim, 88.0f, 493.0f, 12.0f, 0.2f);
    centredText (g, "SC HPF", small, dim, 189.0f, 493.0f, 12.0f, 0.2f);
    centredText (g, "THRESHOLD", small, dim, 817.0f, 284.0f, 12.0f, 0.2f);
    centredText (g, "GAIN", small, dim, 918.0f, 284.0f, 12.0f, 0.2f);

    drawText (g, "RATIO", small, dim, RF (761, 351, 120, 12));
    juce::GlyphArrangement attack;
    attack.addLineOfText (small, "ATTACK", 0.0f, 0.0f);
    auto attackWidth = attack.getBoundingBox (0, -1, true).getWidth() + 0.2f * 9.5f * 2.0f;
    drawText (g, "ATTACK", small, dim, RF (761, 406, 120, 12));
    drawText (g, "ms", fonts().get (FontId::interRegular, 9.5f), dim, RF (761.0f + attackWidth, 406, 40, 12));
    drawText (g, "RELEASE", small, dim, RF (761, 461, 120, 12));
}

void InstaShadowEditor::paintConsole (juce::Graphics& g)
{
    using namespace Console;
    g.fillAll (bg);
    g.setColour (line);
    g.fillRect (0, 57, designWidth, 1);

    // Logo mark: a half-filled circle
    auto mark = RF (21.25f, 19.25f, 19.5f, 19.5f);
    g.setColour (cyan);
    g.drawEllipse (mark, 1.5f);
    juce::Path half;
    half.addPieSegment (mark, 0.0f, juce::MathConstants<float>::pi, 0.0f);
    g.fillPath (half);

    drawText (g, "InstaShadow", fonts().get (FontId::interSemiBold, 16.0f, -0.01f), text, RF (52, 19, 110, 20));
    auto badge = RF (160.5f, 19.5f, 29.0f, 18.0f);
    g.setColour (btnBorder);
    g.drawRoundedRectangle (badge, 4.0f, 1.0f);
    drawText (g, versionShort(), fonts().get (FontId::jetMonoRegular, 10.0f), dim, badge, juce::Justification::centred);

    // Modules
    drawPanel (g, RF (20, 76, 242, 444), 12.0f, Console::panel, line);
    drawPanel (g, RF (284, 76, 292, 444), 12.0f, Console::panel, line);
    drawPanel (g, RF (598, 76, 184, 444), 12.0f, Console::panel, line);
    drawPanel (g, RF (804, 76, 176, 444), 12.0f, Console::panel, line);

    g.setColour (chevron);
    for (auto x : { 273.0f, 587.0f, 793.0f })
        g.strokePath (chevronPath (RF (x - 5.0f, 293.0f, 10.0f, 10.0f), 1),
                      juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto stage = fonts().get (FontId::interRegular, 10.0f, 0.12f);
    auto name = fonts().get (FontId::interSemiBold, 15.0f);
    drawText (g, "STAGE 1", stage, dim, RF (37, 98, 120, 13));
    drawText (g, "Optical", name, text, RF (37, 112, 150, 20));
    drawText (g, "STAGE 2", stage, dim, RF (301, 98, 120, 13));
    drawText (g, "Discrete VCA", name, text, RF (301, 112, 180, 20));
    drawText (g, "STAGE 3", stage, dim, RF (615, 92, 120, 13));
    drawText (g, "Transformer", name, text, RF (615, 106, 150, 20));
    drawText (g, "OUT", stage, dim, RF (821, 92, 120, 13));
    drawText (g, "Output", name, text, RF (821, 106, 150, 20));

    auto caption = fonts().get (FontId::interRegular, 11.5f);
    auto small = fonts().get (FontId::interRegular, 11.0f);

    auto captionWithUnit = [&] (const juce::String& main, const juce::String& unit, const juce::Font& f, float cx, float y)
    {
        juce::GlyphArrangement a, b;
        a.addLineOfText (f, main + " ", 0.0f, 0.0f);
        b.addLineOfText (f, unit, 0.0f, 0.0f);
        auto wa = a.getBoundingBox (0, -1, true).getWidth() + f.getHeightInPoints() * 0.28f;
        auto wb = b.getBoundingBox (0, -1, true).getWidth();
        auto x = cx - (wa + wb) * 0.5f;
        drawText (g, main, f, soft, RF (x, y, wa + 4.0f, 15.0f));
        drawText (g, unit, f, dimmer, RF (x + wa, y, wb + 4.0f, 15.0f));
    };

    centredText (g, "Threshold", caption, soft, 141.0f, 365.0f, 15.0f);
    centredText (g, "Gain", small, soft, 87.0f, 488.0f, 15.0f);
    captionWithUnit ("SC HPF", "Hz", small, 196.0f, 488.0f);
    centredText (g, "Threshold", caption, soft, 363.0f, 327.0f, 15.0f);
    captionWithUnit ("Gain", "dB", caption, 497.0f, 327.0f);
    captionWithUnit ("Gain", "dB", small, 874.0f, 484.0f);
    drawText (g, "L R", fonts().get (FontId::jetMonoRegular, 9.0f), dimmer, RF (941, 490, 24, 12), juce::Justification::centred);
}

void InstaShadowEditor::paintIvory (juce::Graphics& g)
{
    using namespace Ivory;
    g.fillAll (outer);

    // Rack ears
    for (auto x : { 0.0f, 963.0f })
    {
        g.setColour (ear);
        g.fillRect (RF (x, 0, 37, 540));
        g.setColour (earBorder);
        g.fillRect (RF (x < 1.0f ? 36.0f : 963.0f, 0, 1, 540));
        for (auto y : { 30.0f, 490.0f })
        {
            auto slotRect = RF (x + 12.0f, y, 12.0f, 20.0f);
            g.setColour (slot);
            g.fillRoundedRectangle (slotRect, 6.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.5f), 0.0f, y,
                                                     juce::Colours::transparentBlack, 0.0f, y + 4.0f, false));
            g.fillRoundedRectangle (slotRect, 6.0f);
        }
    }

    g.setColour (face);
    g.fillRect (RF (37, 0, 926, 540));

    // Header
    drawText (g, "InstaShadow", fonts().get (FontId::playfairItalic, 26.0f), inkDark, RF (59, 16, 240, 32));
    drawText (g, "MASTERING COMPRESSOR", fonts().get (FontId::plexCondRegular, 9.0f, 0.3f), dim, RF (59, 47, 240, 12));
    g.setColour (line);
    g.fillRect (RF (59, 74, 882, 1));

    auto tiny = fonts().get (FontId::plexCondRegular, 9.0f, 0.22f);
    centredText (g, "LINK", tiny, label, 861.0f, 47.0f, 12.0f, 0.22f);
    centredText (g, "BYPASS", tiny, label, 919.0f, 47.0f, 12.0f, 0.22f);

    // Stage titles with rules
    auto title = fonts().get (FontId::plexCondSemiBold, 12.0f, 0.3f);
    drawText (g, "OPTICAL", title, ink, RF (59, 90, 160, 15));
    drawText (g, "DISCRETE", title, ink, RF (709, 90, 160, 15));
    g.setColour (ink);
    g.fillRect (RF (59, 110, 200, 1));
    g.fillRect (RF (709, 110, 232, 1));

    auto caption = fonts().get (FontId::plexCondMedium, 10.5f, 0.24f);
    auto small = fonts().get (FontId::plexCondMedium, 10.0f, 0.24f);
    centredText (g, "THRESHOLD", caption, ink, 159.0f, 302.0f, 14.0f, 0.24f);
    centredText (g, "GAIN", small, ink, 97.0f, 490.0f, 13.0f, 0.24f);
    centredText (g, "SC HPF", small, ink, 221.0f, 490.0f, 13.0f, 0.24f);
    centredText (g, "THRESHOLD", small, ink, 757.0f, 306.0f, 13.0f, 0.24f);
    centredText (g, "GAIN", small, ink, 903.0f, 306.0f, 13.0f, 0.24f);
    centredText (g, "OUTPUT", small, ink, 654.0f, 508.0f, 13.0f, 0.24f);

    // Transformer / output section
    g.setColour (line);
    g.fillRect (RF (281, 406, 406, 1));
    drawText (g, "TRANSFORMER", small, ink, RF (281, 418, 200, 13));

    auto rowLabel = fonts().get (FontId::plexCondMedium, 9.5f, 0.2f);
    drawText (g, "RATIO", rowLabel, ink, RF (709, 439, 60, 13));
    drawText (g, "ATTACK", rowLabel, ink, RF (709, 471, 60, 13));
    drawText (g, "RELEASE", rowLabel, ink, RF (709, 503, 60, 13));
}
