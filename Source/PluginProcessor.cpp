#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr const char* stateTag   = "InstaShadowState";
    constexpr const char* propTheme  = "theme";
    constexpr const char* propWidth  = "editorWidth";
    constexpr const char* propHeight = "editorHeight";
    constexpr const char* propVersion = "stateVersion";

    // Last theme chosen in any instance: the default for new instances.
    juce::PropertiesFile::Options globalSettingsOptions()
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "InstaShadow";
        o.filenameSuffix = ".settings";
        o.folderName = "InstaShadow";
        o.osxLibrarySubFolder = "Application Support";
        return o;
    }
}

InstaShadowProcessor::InstaShadowProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, stateTag, createParameterLayout()),
      presetManager (apvts),
      programChanger (*this)
{
    raw.optoThreshold   = apvts.getRawParameterValue (ParamIDs::optoThreshold);
    raw.optoGain        = apvts.getRawParameterValue (ParamIDs::optoGain);
    raw.optoScHpf       = apvts.getRawParameterValue (ParamIDs::optoScHpf);
    raw.optoBypass      = apvts.getRawParameterValue (ParamIDs::optoBypass);
    raw.vcaThreshold    = apvts.getRawParameterValue (ParamIDs::vcaThreshold);
    raw.vcaGain         = apvts.getRawParameterValue (ParamIDs::vcaGain);
    raw.vcaRatio        = apvts.getRawParameterValue (ParamIDs::vcaRatio);
    raw.vcaAttack       = apvts.getRawParameterValue (ParamIDs::vcaAttack);
    raw.vcaRelease      = apvts.getRawParameterValue (ParamIDs::vcaRelease);
    raw.vcaBypass       = apvts.getRawParameterValue (ParamIDs::vcaBypass);
    raw.transformerType = apvts.getRawParameterValue (ParamIDs::transformerType);
    raw.outputGain      = apvts.getRawParameterValue (ParamIDs::outputGain);
    raw.stereoLink      = apvts.getRawParameterValue (ParamIDs::stereoLink);
    raw.globalBypass    = apvts.getRawParameterValue (ParamIDs::globalBypass);

    apvts.state.setProperty (propVersion, stateVersion, nullptr);

    juce::PropertiesFile settings (globalSettingsOptions());
    apvts.state.setProperty (propTheme, juce::jlimit (0, 2, settings.getIntValue (propTheme, 0)), nullptr);

    pushParametersToEngine();
}

InstaShadowProcessor::~InstaShadowProcessor() {}

void InstaShadowProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    pushParametersToEngine();
    engine.prepare (sampleRate, samplesPerBlock);
}

void InstaShadowProcessor::releaseResources() {}

bool InstaShadowProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void InstaShadowProcessor::pushParametersToEngine()
{
    auto b = [] (std::atomic<float>* p) { return p->load() >= 0.5f; };
    auto i = [] (std::atomic<float>* p) { return juce::roundToInt (p->load()); };

    engine.optoThresholdDb.store (raw.optoThreshold->load());
    engine.optoGainDb.store      (raw.optoGain->load());
    engine.optoScHpfHz.store     (raw.optoScHpf->load());
    engine.optoBypass.store      (b (raw.optoBypass));
    engine.vcaThresholdDb.store  (raw.vcaThreshold->load());
    engine.vcaGainDb.store       (raw.vcaGain->load());
    engine.vcaRatioIndex.store   (i (raw.vcaRatio));
    engine.vcaAttackIndex.store  (i (raw.vcaAttack));
    engine.vcaReleaseIndex.store (i (raw.vcaRelease));
    engine.vcaBypass.store       (b (raw.vcaBypass));
    engine.transformerType.store (i (raw.transformerType));
    engine.outputGainDb.store    (raw.outputGain->load());
    engine.stereoLink.store      (b (raw.stereoLink));
    engine.globalBypass.store    (b (raw.globalBypass));
}

void InstaShadowProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    pushParametersToEngine();
    engine.processBlock (buffer);
}

juce::AudioProcessorEditor* InstaShadowProcessor::createEditor()
{
    return new InstaShadowEditor (*this);
}

juce::AudioProcessorParameter* InstaShadowProcessor::getBypassParameter() const
{
    return apvts.getParameter (ParamIDs::globalBypass);
}

// ============================================================
// Programs (factory presets)
// ============================================================

int InstaShadowProcessor::getNumPrograms()
{
    return (int) getFactoryPresets().size();
}

int InstaShadowProcessor::getCurrentProgram()
{
    return juce::jmax (0, presetManager.getCurrentFactoryIndex());
}

void InstaShadowProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;

    // Some hosts re-send the current program after restoring a state; that
    // must not overwrite the restored (possibly edited) settings.
    if (index == presetManager.getCurrentFactoryIndex())
        return;

    if (juce::MessageManager::existsAndIsCurrentThread())
    {
        presetManager.loadFactory (index);
    }
    else
    {
        pendingProgram.store (index);
        programChanger.triggerAsyncUpdate();
    }
}

const juce::String InstaShadowProcessor::getProgramName (int index)
{
    const auto& presets = getFactoryPresets();
    if (index >= 0 && index < (int) presets.size())
        return juce::String::fromUTF8 (presets[(size_t) index].name);
    return {};
}

// ============================================================
// State
// ============================================================

void InstaShadowProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml == nullptr)
        return;

    // Also write the v1.0/1.1 attributes, so an older InstaShadow can still
    // open a project saved with this version.
    auto value = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    xml->setAttribute ("optoThreshold",   (double) value (ParamIDs::optoThreshold));
    xml->setAttribute ("optoGain",        (double) value (ParamIDs::optoGain));
    xml->setAttribute ("optoScHpf",       (double) value (ParamIDs::optoScHpf));
    xml->setAttribute ("optoBypass",      value (ParamIDs::optoBypass) >= 0.5f);
    xml->setAttribute ("vcaThreshold",    (double) value (ParamIDs::vcaThreshold));
    xml->setAttribute ("vcaGain",         (double) value (ParamIDs::vcaGain));
    xml->setAttribute ("vcaRatio",        juce::roundToInt (value (ParamIDs::vcaRatio)));
    xml->setAttribute ("vcaAttack",       juce::roundToInt (value (ParamIDs::vcaAttack)));
    xml->setAttribute ("vcaRelease",      juce::roundToInt (value (ParamIDs::vcaRelease)));
    xml->setAttribute ("vcaBypass",       value (ParamIDs::vcaBypass) >= 0.5f);
    xml->setAttribute ("transformerType", juce::roundToInt (value (ParamIDs::transformerType)));
    xml->setAttribute ("outputGain",      (double) value (ParamIDs::outputGain));
    xml->setAttribute ("stereoLink",      value (ParamIDs::stereoLink) >= 0.5f);
    xml->setAttribute ("globalBypass",    value (ParamIDs::globalBypass) >= 0.5f);

    copyXmlToBinary (*xml, destData);
}

void InstaShadowProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (stateTag))
        return;

    if (xml->getChildByName ("PARAM") == nullptr)
    {
        // v1.0/1.1: parameters were attributes of the root element.
        loadLegacyState (*xml);
    }
    else
    {
        auto tree = juce::ValueTree::fromXml (*xml);

        // Drop the compatibility attributes: the PARAM children are authoritative.
        for (auto* id : ParamIDs::all)
            tree.removeProperty (id, nullptr);

        // Keep the editor preferences of this instance if the state has none.
        if (! tree.hasProperty (propTheme))
            tree.setProperty (propTheme, apvts.state.getProperty (propTheme), nullptr);

        tree.setProperty (propVersion, stateVersion, nullptr);
        apvts.replaceState (tree);
    }

    pushParametersToEngine();
    presetManager.restoreFromState();
}

void InstaShadowProcessor::loadLegacyState (const juce::XmlElement& xml)
{
    // Same defaults as v1.1's setStateInformation.
    juce::ValueTree tree (stateTag);
    tree.setProperty (propVersion, stateVersion, nullptr);
    tree.setProperty (propTheme, apvts.state.getProperty (propTheme), nullptr);

    auto addParam = [&tree] (const char* id, double v)
    {
        juce::ValueTree p ("PARAM");
        p.setProperty ("id", id, nullptr);
        p.setProperty ("value", v, nullptr);
        tree.appendChild (p, nullptr);
    };

    auto boolAttr = [&xml] (const char* name, bool def) { return xml.getBoolAttribute (name, def) ? 1.0 : 0.0; };

    addParam (ParamIDs::optoThreshold,   xml.getDoubleAttribute ("optoThreshold", -20.0));
    addParam (ParamIDs::optoGain,        xml.getDoubleAttribute ("optoGain", 0.0));
    addParam (ParamIDs::optoScHpf,       xml.getDoubleAttribute ("optoScHpf", 90.0));
    addParam (ParamIDs::optoBypass,      boolAttr ("optoBypass", false));
    addParam (ParamIDs::vcaThreshold,    xml.getDoubleAttribute ("vcaThreshold", -20.0));
    addParam (ParamIDs::vcaGain,         xml.getDoubleAttribute ("vcaGain", 0.0));
    addParam (ParamIDs::vcaRatio,        juce::jlimit (0, CompressorEngine::numRatios - 1,   xml.getIntAttribute ("vcaRatio", 1)));
    addParam (ParamIDs::vcaAttack,       juce::jlimit (0, CompressorEngine::numAttacks - 1,  xml.getIntAttribute ("vcaAttack", 2)));
    addParam (ParamIDs::vcaRelease,      juce::jlimit (0, CompressorEngine::numReleases - 1, xml.getIntAttribute ("vcaRelease", 2)));
    addParam (ParamIDs::vcaBypass,       boolAttr ("vcaBypass", false));
    addParam (ParamIDs::transformerType, juce::jlimit (0, 3, xml.getIntAttribute ("transformerType", 0)));
    addParam (ParamIDs::outputGain,      xml.getDoubleAttribute ("outputGain", 0.0));
    addParam (ParamIDs::stereoLink,      boolAttr ("stereoLink", true));
    addParam (ParamIDs::globalBypass,    boolAttr ("globalBypass", false));

    apvts.replaceState (tree);
}

// ============================================================
// Editor preferences
// ============================================================

int InstaShadowProcessor::getThemeIndex() const
{
    return juce::jlimit (0, 2, (int) apvts.state.getProperty (propTheme, 0));
}

void InstaShadowProcessor::setThemeIndex (int index)
{
    index = juce::jlimit (0, 2, index);
    apvts.state.setProperty (propTheme, index, nullptr);

    juce::PropertiesFile settings (globalSettingsOptions());
    settings.setValue (propTheme, index);
}

juce::Point<int> InstaShadowProcessor::getEditorSize() const
{
    return { (int) apvts.state.getProperty (propWidth, 0), (int) apvts.state.getProperty (propHeight, 0) };
}

void InstaShadowProcessor::setEditorSize (juce::Point<int> size)
{
    apvts.state.setProperty (propWidth, size.x, nullptr);
    apvts.state.setProperty (propHeight, size.y, nullptr);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new InstaShadowProcessor();
}
