#pragma once
#include <JuceHeader.h>
#include "Parameters.h"

// ============================================================
// Factory presets (also exposed to the host as programs) and user
// presets (one file per preset in the user's preset folder).
//
// The combined list is: factory presets first, then user presets
// sorted by name. The current preset's name and index travel with
// the plugin state, so a host preset or project shows the right name.
// ============================================================
class PresetManager : public juce::ChangeBroadcaster,
                      private juce::AudioProcessorValueTreeState::Listener,
                      private juce::AsyncUpdater
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState&);
    ~PresetManager() override;

    static constexpr const char* fileExtension = ".ispreset";

    struct Entry
    {
        juce::String name;
        bool isFactory = true;
        int factoryIndex = -1;  // for factory presets
        juce::File file;        // for user presets
    };

    // The combined list (rescans the user folder on refresh()).
    const std::vector<Entry>& getEntries() const { return entries; }
    void refresh();

    int getCurrentIndex() const { return currentIndex; }   // -1 if none matches
    juce::String getCurrentName() const { return currentName; }
    bool isModified() const { return modified; }

    // Loads entry i of the combined list (wraps around for next/previous).
    bool load (int index);
    void loadNext();
    void loadPrevious();
    bool loadFactory (int factoryIndex);

    // Saves the current settings as a user preset (overwrites one of the same name).
    bool saveUserPreset (const juce::String& name);

    // Deletes user preset i of the combined list: its file goes to the Trash (or is
    // deleted where the system has none). Factory presets can't be deleted. If it
    // was the current preset, the settings stay as they are, without a preset name.
    bool canDelete (int index) const;
    bool deleteUserPreset (int index);
    int findUserPreset (const juce::File&) const;   // index in the combined list, or -1

    // Tests delete the files directly instead of filling the Trash.
    void setMoveToTrash (bool shouldMoveToTrash) { moveToTrash = shouldMoveToTrash; }

    // Folder of user presets. Tests point it elsewhere.
    juce::File getUserPresetFolder() const { return userFolder; }
    void setUserPresetFolder (const juce::File& f) { userFolder = f; refresh(); }
    static juce::File getDefaultUserPresetFolder();

    // State integration: the name/index are stored in the APVTS state tree.
    void restoreFromState();

    // A change message is sent whenever the name, index or modified flag changed.

    // A/B compare: two sets of settings; switching stores the current one.
    void switchCompareSlot (int slot);
    int getCompareSlot() const { return compareSlot; }

    // Factory preset index for the host's program API (-1 if a user preset / none).
    int getCurrentFactoryIndex() const;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::File userFolder;
    std::vector<Entry> entries;
    int currentIndex = 0;
    juce::String currentName;
    bool modified = false;
    bool applying = false;
    bool moveToTrash = true;

    // Parameter values of the current preset (to detect edits).
    std::map<juce::String, float> currentValues;

    std::map<juce::String, float> compareValues[2];
    int compareSlot = 0;

    void applyValues (const std::map<juce::String, float>& values);
    std::map<juce::String, float> readUserPreset (const juce::File&) const;
    std::map<juce::String, float> captureValues() const;
    void setCurrent (int index, const juce::String& name, const std::map<juce::String, float>& values);
    void storeInState();
    void updateModified();

    void parameterChanged (const juce::String& id, float) override;
    void handleAsyncUpdate() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
