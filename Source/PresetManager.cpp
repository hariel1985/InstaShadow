#include "PresetManager.h"

namespace
{
    constexpr const char* propName  = "presetName";
    constexpr const char* presetTag = "InstaShadowPreset";

    bool valuesEqual (juce::RangedAudioParameter* p, float a, float b)
    {
        auto range = p->getNormalisableRange();
        return std::abs (a - b) <= 1.0e-4f * (range.end - range.start);
    }
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& s)
    : apvts (s), userFolder (getDefaultUserPresetFolder())
{
    for (auto* id : ParamIDs::all)
        apvts.addParameterListener (id, this);

    refresh();
    setCurrent (0, juce::String::fromUTF8 (getFactoryPresets().front().name), captureValues());
}

PresetManager::~PresetManager()
{
    cancelPendingUpdate();
    for (auto* id : ParamIDs::all)
        apvts.removeParameterListener (id, this);
}

juce::File PresetManager::getDefaultUserPresetFolder()
{
   #if JUCE_MAC
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
               .getChildFile ("Library/Audio/Presets/InstaShadow");
   #else
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("InstaShadow/Presets");
   #endif
}

void PresetManager::refresh()
{
    entries.clear();

    const auto& factory = getFactoryPresets();
    for (int i = 0; i < (int) factory.size(); ++i)
        entries.push_back ({ juce::String::fromUTF8 (factory[(size_t) i].name), true, i, {} });

    if (userFolder.isDirectory())
    {
        auto files = userFolder.findChildFiles (juce::File::findFiles, false, juce::String ("*") + fileExtension);
        std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
                   { return a.getFileNameWithoutExtension().compareNatural (b.getFileNameWithoutExtension()) < 0; });

        for (auto& f : files)
        {
            auto xml = juce::XmlDocument::parse (f);
            auto name = (xml != nullptr && xml->hasTagName (presetTag))
                          ? xml->getStringAttribute ("name", f.getFileNameWithoutExtension())
                          : f.getFileNameWithoutExtension();
            entries.push_back ({ name, false, -1, f });
        }
    }

    // Keep pointing at the same preset after a rescan.
    currentIndex = -1;
    for (int i = 0; i < (int) entries.size(); ++i)
        if (entries[(size_t) i].name == currentName) { currentIndex = i; break; }
}

int PresetManager::getCurrentFactoryIndex() const
{
    if (currentIndex >= 0 && currentIndex < (int) entries.size() && entries[(size_t) currentIndex].isFactory)
        return entries[(size_t) currentIndex].factoryIndex;
    return -1;
}

std::map<juce::String, float> PresetManager::captureValues() const
{
    std::map<juce::String, float> values;
    for (auto* id : ParamIDs::all)
        if (ParamIDs::isPresetParam (id))
            if (auto* p = apvts.getParameter (id))
                values[id] = p->convertFrom0to1 (p->getValue());
    return values;
}

void PresetManager::applyValues (const std::map<juce::String, float>& values)
{
    for (auto& [id, value] : values)
    {
        if (! ParamIDs::isPresetParam (id))
            continue;

        if (auto* p = apvts.getParameter (id))
        {
            auto norm = p->convertTo0to1 (value);
            if (std::abs (norm - p->getValue()) < 1.0e-7f)
                continue;

            p->beginChangeGesture();
            p->setValueNotifyingHost (norm);
            p->endChangeGesture();
        }
    }
}

std::map<juce::String, float> PresetManager::readUserPreset (const juce::File& f) const
{
    std::map<juce::String, float> values;
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr || ! xml->hasTagName (presetTag))
        return values;

    for (auto* child : xml->getChildWithTagNameIterator ("PARAM"))
    {
        auto id = child->getStringAttribute ("id");
        if (apvts.getParameter (id) != nullptr && ParamIDs::isPresetParam (id))
            values[id] = (float) child->getDoubleAttribute ("value");
    }
    return values;
}

bool PresetManager::loadFactory (int factoryIndex)
{
    for (int i = 0; i < (int) entries.size(); ++i)
        if (entries[(size_t) i].isFactory && entries[(size_t) i].factoryIndex == factoryIndex)
            return load (i);
    return false;
}

bool PresetManager::load (int index)
{
    if (index < 0 || index >= (int) entries.size())
        return false;

    const auto& e = entries[(size_t) index];
    std::map<juce::String, float> values;

    if (e.isFactory)
    {
        const auto& fp = getFactoryPresets()[(size_t) e.factoryIndex];
        for (auto* id : ParamIDs::all)
            if (ParamIDs::isPresetParam (id))
                values[id] = fp.valueFor (id);
    }
    else
    {
        values = readUserPreset (e.file);
        if (values.empty())
            return false;
    }

    applyValues (values);
    setCurrent (index, e.name, captureValues());
    return true;
}

void PresetManager::loadNext()
{
    if (entries.empty()) return;
    load (currentIndex < 0 ? 0 : (currentIndex + 1) % (int) entries.size());
}

void PresetManager::loadPrevious()
{
    if (entries.empty()) return;
    auto n = (int) entries.size();
    load (currentIndex < 0 ? n - 1 : (currentIndex + n - 1) % n);
}

bool PresetManager::saveUserPreset (const juce::String& rawName)
{
    auto name = rawName.trim();
    if (name.isEmpty())
        return false;

    if (! userFolder.isDirectory() && ! userFolder.createDirectory())
        return false;

    juce::XmlElement xml (presetTag);
    xml.setAttribute ("name", name);
    xml.setAttribute ("pluginVersion", JucePlugin_VersionString);

    auto values = captureValues();
    for (auto* id : ParamIDs::all)
    {
        if (! ParamIDs::isPresetParam (id))
            continue;
        auto* child = xml.createNewChildElement ("PARAM");
        child->setAttribute ("id", id);
        child->setAttribute ("value", (double) values[id]);
    }

    auto file = userFolder.getChildFile (juce::File::createLegalFileName (name) + fileExtension);
    if (! xml.writeTo (file))
        return false;

    currentName = name;  // so refresh() finds it
    refresh();
    setCurrent (currentIndex, name, values);
    return true;
}

bool PresetManager::canDelete (int index) const
{
    return index >= 0 && index < (int) entries.size() && ! entries[(size_t) index].isFactory;
}

int PresetManager::findUserPreset (const juce::File& f) const
{
    for (int i = 0; i < (int) entries.size(); ++i)
        if (! entries[(size_t) i].isFactory && entries[(size_t) i].file == f)
            return i;
    return -1;
}

bool PresetManager::deleteUserPreset (int index)
{
    if (! canDelete (index))
        return false;

    auto file = entries[(size_t) index].file;
    bool wasCurrent = index == currentIndex;

    if (file.exists() && ! (moveToTrash && file.moveToTrash() && ! file.exists()))
        if (! file.deleteFile())
            return false;

    if (wasCurrent)
    {
        // The sound doesn't change; it just no longer has a preset behind it.
        currentName = {};
        refresh();
        setCurrent (-1, {}, captureValues());
    }
    else
    {
        refresh();   // the current preset's index may have moved up
        sendChangeMessage();
    }
    return true;
}

void PresetManager::setCurrent (int index, const juce::String& name, const std::map<juce::String, float>& values)
{
    currentIndex = index;
    currentName = name;
    currentValues = values;
    modified = false;
    storeInState();
    sendChangeMessage();
}

void PresetManager::storeInState()
{
    apvts.state.setProperty (propName, currentName, nullptr);
}

void PresetManager::restoreFromState()
{
    auto name = apvts.state.getProperty (propName).toString();
    if (name.isEmpty())
    {
        // Old state (v1.0/1.1) had no preset name.
        currentName = {};
        currentIndex = -1;
        currentValues = captureValues();
        modified = false;
        sendChangeMessage();
        return;
    }

    currentName = name;
    refresh();

    if (currentIndex >= 0)
    {
        const auto& e = entries[(size_t) currentIndex];
        std::map<juce::String, float> values;
        if (e.isFactory)
        {
            const auto& fp = getFactoryPresets()[(size_t) e.factoryIndex];
            for (auto* id : ParamIDs::all)
                if (ParamIDs::isPresetParam (id))
                    values[id] = fp.valueFor (id);
        }
        else
        {
            values = readUserPreset (e.file);
        }
        currentValues = values;
    }
    else
    {
        currentValues = captureValues();
    }

    updateModified();
    sendChangeMessage();
}

void PresetManager::updateModified()
{
    bool m = false;
    for (auto& [id, value] : currentValues)
        if (auto* p = apvts.getParameter (id))
            if (! valuesEqual (p, value, p->convertFrom0to1 (p->getValue())))
                { m = true; break; }

    if (m != modified)
    {
        modified = m;
        sendChangeMessage();
    }
}

void PresetManager::parameterChanged (const juce::String&, float)
{
    // May come from the audio thread (automation): check on the message thread.
    triggerAsyncUpdate();
}

void PresetManager::handleAsyncUpdate()
{
    updateModified();
}

void PresetManager::switchCompareSlot (int slot)
{
    slot = juce::jlimit (0, 1, slot);
    if (slot == compareSlot)
        return;

    compareValues[compareSlot] = captureValues();
    if (! compareValues[slot].empty())
        applyValues (compareValues[slot]);
    compareSlot = slot;
    sendChangeMessage();
}
