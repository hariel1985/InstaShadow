#include "PluginEditor.h"

InstaShadowEditor::InstaShadowEditor (InstaShadowProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setSize (1000, 600);
    setResizable (true, true);
    setResizeLimits (800, 500, 1400, 900);

    // Title
    titleLabel.setText ("INSTASHADOW", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::accent);
    addAndMakeVisible (titleLabel);

    versionLabel.setText (kInstaShadowVersion, juce::dontSendNotification);
    versionLabel.setJustificationType (juce::Justification::centredLeft);
    versionLabel.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::textSecondary);
    addAndMakeVisible (versionLabel);

    // Link
    linkToggle.setToggleState (true, juce::dontSendNotification);
    addAndMakeVisible (linkToggle);
    linkLabel.setText ("LINK", juce::dontSendNotification);
    linkLabel.setJustificationType (juce::Justification::centredRight);
    linkLabel.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::textSecondary);
    addAndMakeVisible (linkLabel);

    // Bypass
    bypassToggle.setToggleState (false, juce::dontSendNotification);
    addAndMakeVisible (bypassToggle);
    bypassLabel.setText ("BYPASS", juce::dontSendNotification);
    bypassLabel.setJustificationType (juce::Justification::centredRight);
    bypassLabel.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::textSecondary);
    addAndMakeVisible (bypassLabel);

    // Panels
    addAndMakeVisible (opticalPanel);
    addAndMakeVisible (discretePanel);
    addAndMakeVisible (transformerPanel);
    addAndMakeVisible (outputPanel);

    // Needle meters (default: GR)
    needleMeterL.setLabel ("OPTICAL GR");
    needleMeterL.setMode (NeedleVuMeter::GR);
    addAndMakeVisible (needleMeterL);
    needleMeterR.setLabel ("DISCRETE GR");
    needleMeterR.setMode (NeedleVuMeter::GR);
    addAndMakeVisible (needleMeterR);

    // Bar meters (default: input level)
    barMeterL.setLabel ("INPUT L");
    barMeterL.setBarColour (juce::Colour (0xff00cc44));
    addAndMakeVisible (barMeterL);
    barMeterR.setLabel ("INPUT R");
    barMeterR.setBarColour (juce::Colour (0xff00cc44));
    addAndMakeVisible (barMeterR);

    // Meter swap button
    meterSwapButton.onClick = [this]
    {
        metersSwapped = ! metersSwapped;
        if (metersSwapped)
        {
            needleMeterL.setLabel ("INPUT L");
            needleMeterL.setMode (NeedleVuMeter::VU);
            needleMeterR.setLabel ("INPUT R");
            needleMeterR.setMode (NeedleVuMeter::VU);
            barMeterL.setLabel ("OPTICAL GR");
            barMeterL.setBarColour (juce::Colour (0xffff8833));
            barMeterR.setLabel ("DISCRETE GR");
            barMeterR.setBarColour (juce::Colour (0xff4488ff));
        }
        else
        {
            needleMeterL.setLabel ("OPTICAL GR");
            needleMeterL.setMode (NeedleVuMeter::GR);
            needleMeterR.setLabel ("DISCRETE GR");
            needleMeterR.setMode (NeedleVuMeter::GR);
            barMeterL.setLabel ("INPUT L");
            barMeterL.setBarColour (juce::Colour (0xff00cc44));
            barMeterR.setLabel ("INPUT R");
            barMeterR.setBarColour (juce::Colour (0xff00cc44));
        }
    };
    addAndMakeVisible (meterSwapButton);

    syncKnobsToEngine();
    startTimerHz (30);
}

InstaShadowEditor::~InstaShadowEditor()
{
    setLookAndFeel (nullptr);
    stopTimer();
}

void InstaShadowEditor::syncKnobsToEngine()
{
    auto& eng = processor.getEngine();

    opticalPanel.thresholdKnob.setValue (eng.optoThresholdDb.load(), juce::dontSendNotification);
    opticalPanel.gainKnob.setValue (eng.optoGainDb.load(), juce::dontSendNotification);
    opticalPanel.scHpfKnob.setValue (eng.optoScHpfHz.load(), juce::dontSendNotification);
    opticalPanel.bypassToggle.setToggleState (eng.optoBypass.load(), juce::dontSendNotification);

    discretePanel.thresholdKnob.setValue (eng.vcaThresholdDb.load(), juce::dontSendNotification);
    discretePanel.gainKnob.setValue (eng.vcaGainDb.load(), juce::dontSendNotification);
    discretePanel.ratioBox.setSelectedId (eng.vcaRatioIndex.load() + 1, juce::dontSendNotification);
    discretePanel.attackBox.setSelectedId (eng.vcaAttackIndex.load() + 1, juce::dontSendNotification);
    discretePanel.releaseBox.setSelectedId (eng.vcaReleaseIndex.load() + 1, juce::dontSendNotification);
    discretePanel.bypassToggle.setToggleState (eng.vcaBypass.load(), juce::dontSendNotification);

    transformerPanel.setSelectedType (eng.transformerType.load());
    outputPanel.outputGainKnob.setValue (eng.outputGainDb.load(), juce::dontSendNotification);
    linkToggle.setToggleState (eng.stereoLink.load(), juce::dontSendNotification);
    bypassToggle.setToggleState (eng.globalBypass.load(), juce::dontSendNotification);
}

void InstaShadowEditor::syncEngineFromKnobs()
{
    auto& eng = processor.getEngine();

    eng.optoThresholdDb.store ((float) opticalPanel.thresholdKnob.getValue());
    eng.optoGainDb.store ((float) opticalPanel.gainKnob.getValue());
    eng.optoScHpfHz.store ((float) opticalPanel.scHpfKnob.getValue());
    eng.optoBypass.store (opticalPanel.bypassToggle.getToggleState());

    eng.vcaThresholdDb.store ((float) discretePanel.thresholdKnob.getValue());
    eng.vcaGainDb.store ((float) discretePanel.gainKnob.getValue());
    eng.vcaRatioIndex.store (discretePanel.ratioBox.getSelectedId() - 1);
    eng.vcaAttackIndex.store (discretePanel.attackBox.getSelectedId() - 1);
    eng.vcaReleaseIndex.store (discretePanel.releaseBox.getSelectedId() - 1);
    eng.vcaBypass.store (discretePanel.bypassToggle.getToggleState());

    eng.transformerType.store (transformerPanel.getSelectedType());
    eng.outputGainDb.store ((float) outputPanel.outputGainKnob.getValue());
    eng.stereoLink.store (linkToggle.getToggleState());
    eng.globalBypass.store (bypassToggle.getToggleState());
}

void InstaShadowEditor::timerCallback()
{
    syncEngineFromKnobs();

    auto& eng = processor.getEngine();

    if (! metersSwapped)
    {
        // Default: needles = GR, bars = input
        needleMeterL.setGainReduction (eng.optoGrDb.load());
        needleMeterR.setGainReduction (eng.vcaGrDb.load());
        barMeterL.setInputLevel (eng.inputLevelL.load());
        barMeterR.setInputLevel (eng.inputLevelR.load());
    }
    else
    {
        // Swapped: needles = input, bars = GR
        needleMeterL.setLevel (eng.inputLevelL.load());
        needleMeterR.setLevel (eng.inputLevelR.load());
        barMeterL.setGainReduction (eng.optoGrDb.load());
        barMeterR.setGainReduction (eng.vcaGrDb.load());
    }

    // Output panel VU
    outputPanel.vuMeter.setLevel (eng.outputLevelL.load(), eng.outputLevelR.load());
}

void InstaShadowEditor::paint (juce::Graphics& g)
{
    g.fillAll (InstaShadowLookAndFeel::bgDark);
    lookAndFeel.drawBackgroundTexture (g, getLocalBounds());

    float scale = (float) getHeight() / 600.0f;
    int topBarH = (int) (36.0f * scale);

    g.setColour (InstaShadowLookAndFeel::bgMedium.withAlpha (0.7f));
    g.fillRect (0, 0, getWidth(), topBarH);
    g.setColour (InstaShadowLookAndFeel::bgLight.withAlpha (0.3f));
    g.drawHorizontalLine (topBarH, 0, (float) getWidth());
}

void InstaShadowEditor::resized()
{
    auto bounds = getLocalBounds();
    float scale = (float) getHeight() / 600.0f;

    int topBarH = (int) (36.0f * scale);
    int pad = (int) (6.0f * scale);

    // ===== TOP BAR =====
    auto topBar = bounds.removeFromTop (topBarH).reduced (pad, 0);
    titleLabel.setFont (lookAndFeel.getBoldFont (20.0f * scale));
    titleLabel.setBounds (topBar.removeFromLeft ((int) (140 * scale)));
    versionLabel.setFont (lookAndFeel.getRegularFont (13.0f * scale));
    versionLabel.setBounds (topBar.removeFromLeft ((int) (50 * scale)));

    auto bypassArea = topBar.removeFromRight ((int) (30 * scale));
    bypassToggle.setBounds (bypassArea);
    bypassLabel.setFont (lookAndFeel.getRegularFont (11.0f * scale));
    bypassLabel.setBounds (topBar.removeFromRight ((int) (55 * scale)));
    topBar.removeFromRight (pad);
    auto linkArea = topBar.removeFromRight ((int) (30 * scale));
    linkToggle.setBounds (linkArea);
    linkLabel.setFont (lookAndFeel.getRegularFont (11.0f * scale));
    linkLabel.setBounds (topBar.removeFromRight ((int) (40 * scale)));

    auto content = bounds.reduced (pad);

    // ===== MAIN: 3 columns =====
    int sideW = (int) (content.getWidth() * 0.22f);
    auto mainRow = content;

    // Left: Optical
    opticalPanel.setBounds (mainRow.removeFromLeft (sideW));
    mainRow.removeFromLeft (pad);

    // Right: Discrete
    auto rightCol = mainRow.removeFromRight (sideW);
    discretePanel.setBounds (rightCol);
    mainRow.removeFromRight (pad);

    // Center column: VU meters, GR bars, Transformer, Output — all stacked
    auto centerArea = mainRow;

    // Two needle meters side by side (~30%)
    int vuH = (int) (centerArea.getHeight() * 0.30f);
    auto needleRow = centerArea.removeFromTop (vuH);
    int needleW = (needleRow.getWidth() - pad) / 2;
    needleMeterL.setBounds (needleRow.removeFromLeft (needleW));
    needleRow.removeFromLeft (pad);
    needleMeterR.setBounds (needleRow);
    centerArea.removeFromTop (pad);

    // Swap button (compact, between needles and bars)
    meterSwapButton.setBounds (centerArea.removeFromTop (20).reduced (centerArea.getWidth() / 4, 0));
    centerArea.removeFromTop (pad);

    // Two bar meters (~10%)
    int barH = (int) (centerArea.getHeight() * 0.10f);
    barMeterL.setBounds (centerArea.removeFromTop (barH));
    centerArea.removeFromTop (pad);
    barMeterR.setBounds (centerArea.removeFromTop (barH));
    centerArea.removeFromTop (pad);

    // Transformer + Output side by side in remaining center space
    auto botCenter = centerArea;
    int botHalf = botCenter.getWidth() / 2;
    transformerPanel.setBounds (botCenter.removeFromLeft (botHalf - pad / 2));
    botCenter.removeFromLeft (pad);
    outputPanel.setBounds (botCenter);
}
