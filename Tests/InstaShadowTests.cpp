// Command-line tests for InstaShadow's state, presets, editor recall and
// meter repaint behaviour. Build with -DINSTASHADOW_BUILD_TESTS=ON.
//
//   InstaShadowTests [--snapshots <folder>]
//
// Exit code 0 = all passed.

#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

namespace
{
    int failures = 0, checks = 0;

    void check (bool ok, const juce::String& what)
    {
        ++checks;
        if (! ok) ++failures;
        std::cout << (ok ? "  ok    " : "  FAIL  ") << what << std::endl;
    }

    void section (const juce::String& name) { std::cout << "\n== " << name << std::endl; }

    juce::RangedAudioParameter& param (InstaShadowProcessor& p, const char* id)
    {
        return *p.getState().getParameter (id);
    }

    float value (InstaShadowProcessor& p, const char* id)
    {
        return p.getState().getRawParameterValue (id)->load();
    }

    void setValue (InstaShadowProcessor& p, const char* id, float v)
    {
        auto& prm = param (p, id);
        prm.setValueNotifyingHost (prm.convertTo0to1 (v));
    }

    juce::MemoryBlock getState (InstaShadowProcessor& p)
    {
        juce::MemoryBlock mb;
        p.getStateInformation (mb);
        return mb;
    }

    void pump (int ms)
    {
        juce::MessageManager::getInstance()->runDispatchLoopUntil (ms);
    }

    // The v1.1 getStateInformation, verbatim apart from the source of the values.
    struct V11Values
    {
        float optoThreshold = -27.3f, optoGain = 4.2f, optoScHpf = 150.0f; bool optoBypass = true;
        float vcaThreshold = -11.5f, vcaGain = 2.5f; int vcaRatio = 4, vcaAttack = 5, vcaRelease = 0; bool vcaBypass = false;
        int transformerType = 3; float outputGain = -3.5f; bool stereoLink = false, globalBypass = true;
    };

    juce::MemoryBlock makeV11State (const V11Values& v)
    {
        juce::MemoryBlock destData;
        auto xml = std::make_unique<juce::XmlElement> ("InstaShadowState");
        xml->setAttribute ("optoThreshold", (double) v.optoThreshold);
        xml->setAttribute ("optoGain", (double) v.optoGain);
        xml->setAttribute ("optoScHpf", (double) v.optoScHpf);
        xml->setAttribute ("optoBypass", v.optoBypass);
        xml->setAttribute ("vcaThreshold", (double) v.vcaThreshold);
        xml->setAttribute ("vcaGain", (double) v.vcaGain);
        xml->setAttribute ("vcaRatio", v.vcaRatio);
        xml->setAttribute ("vcaAttack", v.vcaAttack);
        xml->setAttribute ("vcaRelease", v.vcaRelease);
        xml->setAttribute ("vcaBypass", v.vcaBypass);
        xml->setAttribute ("transformerType", v.transformerType);
        xml->setAttribute ("outputGain", (double) v.outputGain);
        xml->setAttribute ("stereoLink", v.stereoLink);
        xml->setAttribute ("globalBypass", v.globalBypass);
        juce::AudioProcessor::copyXmlToBinary (*xml, destData);
        return destData;
    }

    // The v1.1 setStateInformation, reading into V11Values: checks that an
    // older InstaShadow can still read a state saved by this version.
    bool readAsV11 (const juce::MemoryBlock& mb, V11Values& v)
    {
        auto xml = juce::AudioProcessor::getXmlFromBinary (mb.getData(), (int) mb.getSize());
        if (xml == nullptr || ! xml->hasTagName ("InstaShadowState"))
            return false;
        v.optoThreshold = (float) xml->getDoubleAttribute ("optoThreshold", -20.0);
        v.optoGain = (float) xml->getDoubleAttribute ("optoGain", 0.0);
        v.optoScHpf = (float) xml->getDoubleAttribute ("optoScHpf", 90.0);
        v.optoBypass = xml->getBoolAttribute ("optoBypass", false);
        v.vcaThreshold = (float) xml->getDoubleAttribute ("vcaThreshold", -20.0);
        v.vcaGain = (float) xml->getDoubleAttribute ("vcaGain", 0.0);
        v.vcaRatio = xml->getIntAttribute ("vcaRatio", 1);
        v.vcaAttack = xml->getIntAttribute ("vcaAttack", 2);
        v.vcaRelease = xml->getIntAttribute ("vcaRelease", 2);
        v.vcaBypass = xml->getBoolAttribute ("vcaBypass", false);
        v.transformerType = xml->getIntAttribute ("transformerType", 0);
        v.outputGain = (float) xml->getDoubleAttribute ("outputGain", 0.0);
        v.stereoLink = xml->getBoolAttribute ("stereoLink", true);
        v.globalBypass = xml->getBoolAttribute ("globalBypass", false);
        return true;
    }

    void checkV11Values (InstaShadowProcessor& p, const V11Values& v, const juce::String& prefix)
    {
        auto near = [] (float a, float b) { return std::abs (a - b) < 1.0e-4f; };
        check (near (value (p, ParamIDs::optoThreshold), v.optoThreshold), prefix + "optoThreshold");
        check (near (value (p, ParamIDs::optoGain), v.optoGain), prefix + "optoGain");
        check (near (value (p, ParamIDs::optoScHpf), v.optoScHpf), prefix + "optoScHpf");
        check ((value (p, ParamIDs::optoBypass) >= 0.5f) == v.optoBypass, prefix + "optoBypass");
        check (near (value (p, ParamIDs::vcaThreshold), v.vcaThreshold), prefix + "vcaThreshold");
        check (near (value (p, ParamIDs::vcaGain), v.vcaGain), prefix + "vcaGain");
        check (juce::roundToInt (value (p, ParamIDs::vcaRatio)) == v.vcaRatio, prefix + "vcaRatio");
        check (juce::roundToInt (value (p, ParamIDs::vcaAttack)) == v.vcaAttack, prefix + "vcaAttack");
        check (juce::roundToInt (value (p, ParamIDs::vcaRelease)) == v.vcaRelease, prefix + "vcaRelease");
        check ((value (p, ParamIDs::vcaBypass) >= 0.5f) == v.vcaBypass, prefix + "vcaBypass");
        check (juce::roundToInt (value (p, ParamIDs::transformerType)) == v.transformerType, prefix + "transformerType");
        check (near (value (p, ParamIDs::outputGain), v.outputGain), prefix + "outputGain");
        check ((value (p, ParamIDs::stereoLink) >= 0.5f) == v.stereoLink, prefix + "stereoLink");
        check ((value (p, ParamIDs::globalBypass) >= 0.5f) == v.globalBypass, prefix + "globalBypass");

        // ...and the engine got them too
        auto& e = p.getEngine();
        check (near (e.optoThresholdDb.load(), v.optoThreshold) && e.vcaRatioIndex.load() == v.vcaRatio
               && e.transformerType.load() == v.transformerType && e.globalBypass.load() == v.globalBypass,
               prefix + "engine follows the parameters");
    }

    // Sets every parameter to a random legal value; returns the normalised values.
    std::map<juce::String, float> randomise (InstaShadowProcessor& p, juce::Random& rng)
    {
        std::map<juce::String, float> values;
        for (auto* id : ParamIDs::all)
        {
            auto& prm = param (p, id);
            auto steps = prm.getNumSteps();
            float norm = rng.nextFloat();
            if (steps > 1 && steps < 100)
                norm = (float) rng.nextInt (steps) / (float) (steps - 1);
            prm.setValueNotifyingHost (norm);
            values[id] = prm.getValue();
        }
        return values;
    }

    void fillSine (juce::AudioBuffer<float>& b, double& phase, float amplitude, double freq, double sr)
    {
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            auto s = amplitude * (float) std::sin (phase);
            phase += juce::MathConstants<double>::twoPi * freq / sr;
            for (int ch = 0; ch < b.getNumChannels(); ++ch)
                b.setSample (ch, i, s);
        }
    }
}

// ============================================================

static void testParameters()
{
    section ("Host parameters");
    InstaShadowProcessor p;
    auto& params = p.getParameters();
    check (params.size() == ParamIDs::numParams, "host sees " + juce::String (params.size()) + " parameters (v1.1: 0)");

    for (auto* id : ParamIDs::all)
    {
        auto* prm = p.getState().getParameter (id);
        check (prm != nullptr && prm->isAutomatable(), juce::String ("parameter '") + id + "' exists and is automatable");
    }

    check (p.getBypassParameter() == p.getState().getParameter (ParamIDs::globalBypass), "host bypass = globalBypass parameter");
    check (juce::approximatelyEqual (value (p, ParamIDs::optoThreshold), -20.0f)
           && juce::roundToInt (value (p, ParamIDs::vcaRatio)) == 1
           && value (p, ParamIDs::stereoLink) >= 0.5f, "defaults as in v1.1");
}

static void testStateRoundTrip()
{
    section ("State round trip (every parameter)");
    juce::Random rng (1234);

    for (int round = 0; round < 20; ++round)
    {
        InstaShadowProcessor a;
        auto values = randomise (a, rng);
        a.setThemeIndex (round % 3);
        auto mb = getState (a);

        InstaShadowProcessor b;
        b.setStateInformation (mb.getData(), (int) mb.getSize());

        bool allEqual = true;
        juce::String diffs;
        for (auto& [id, norm] : values)
        {
            auto got = b.getState().getParameter (id)->getValue();
            if (std::abs (got - norm) > 1.0e-5f) { allEqual = false; diffs << id << " "; }
        }
        check (allEqual, "round " + juce::String (round) + ": all " + juce::String (ParamIDs::numParams)
                         + " parameters restored" + (diffs.isNotEmpty() ? " (differs: " + diffs + ")" : ""));
        if (round < 3)
            check (b.getThemeIndex() == round % 3, "round " + juce::String (round) + ": theme restored");
    }
}

static void testLegacyState()
{
    section ("v1.1 state still loads");
    V11Values v;
    auto old = makeV11State (v);

    InstaShadowProcessor p;
    p.setStateInformation (old.getData(), (int) old.getSize());
    checkV11Values (p, v, "v1.1 attribute ");

    // Missing attributes take the v1.1 defaults
    juce::XmlElement partial ("InstaShadowState");
    partial.setAttribute ("optoThreshold", -31.0);
    juce::MemoryBlock mb;
    juce::AudioProcessor::copyXmlToBinary (partial, mb);
    InstaShadowProcessor q;
    setValue (q, ParamIDs::vcaGain, 7.0f);
    q.setStateInformation (mb.getData(), (int) mb.getSize());
    V11Values defaults;
    defaults.optoThreshold = -31.0f; defaults.optoGain = 0.0f; defaults.optoScHpf = 90.0f; defaults.optoBypass = false;
    defaults.vcaThreshold = -20.0f; defaults.vcaGain = 0.0f; defaults.vcaRatio = 1; defaults.vcaAttack = 2; defaults.vcaRelease = 2;
    defaults.vcaBypass = false; defaults.transformerType = 0; defaults.outputGain = 0.0f; defaults.stereoLink = true; defaults.globalBypass = false;
    checkV11Values (q, defaults, "partial v1.1 state, ");

    section ("New state is still readable by v1.1");
    auto saved = getState (p);
    V11Values back;
    check (readAsV11 (saved, back), "v1.1 parser accepts the new state");
    check (std::abs (back.optoThreshold - v.optoThreshold) < 1e-4f && back.vcaRatio == v.vcaRatio
           && back.transformerType == v.transformerType && back.stereoLink == v.stereoLink
           && back.globalBypass == v.globalBypass && std::abs (back.outputGain - v.outputGain) < 1e-4f,
           "v1.1 reads the same values from it");
}

static void testPrograms()
{
    section ("Programs (factory presets) in the host API");
    InstaShadowProcessor p;
    const auto& factory = getFactoryPresets();
    check (p.getNumPrograms() == (int) factory.size() && p.getNumPrograms() > 1,
           "getNumPrograms() = " + juce::String (p.getNumPrograms()) + " (v1.1: 1, unnamed)");
    bool named = true;
    for (int i = 0; i < p.getNumPrograms(); ++i)
        named = named && p.getProgramName (i).isNotEmpty();
    check (named, "every program has a name, e.g. '" + p.getProgramName (1) + "'");

    p.setCurrentProgram (1);
    check (p.getCurrentProgram() == 1, "setCurrentProgram(1) -> getCurrentProgram() == 1");
    check (std::abs (value (p, ParamIDs::vcaThreshold) - factory[1].vcaThreshold) < 1e-4f
           && juce::roundToInt (value (p, ParamIDs::vcaRelease)) == factory[1].vcaRelease
           && juce::roundToInt (value (p, ParamIDs::transformerType)) == factory[1].transformerType,
           "program values applied");
    check (p.getPresetManager().getCurrentName() == p.getProgramName (1) && ! p.getPresetManager().isModified(),
           "preset name shown, not modified");

    // Edit, then the host re-sends the same program (Ableton/others do after a state load): keep the edit.
    setValue (p, ParamIDs::outputGain, 4.0f);
    pump (20);
    check (p.getPresetManager().isModified(), "editing a parameter marks the preset modified");
    p.setCurrentProgram (1);
    check (std::abs (value (p, ParamIDs::outputGain) - 4.0f) < 1e-4f, "re-sending the current program keeps the edit");

    auto mb = getState (p);
    InstaShadowProcessor q;
    q.setStateInformation (mb.getData(), (int) mb.getSize());
    check (q.getPresetManager().getCurrentName() == p.getProgramName (1) && q.getCurrentProgram() == 1,
           "state restores the preset name and program number");
    check (q.getPresetManager().isModified(), "...and that it was modified");

    // Global bypass is not part of a preset
    setValue (p, ParamIDs::globalBypass, 1.0f);
    p.setCurrentProgram (2);
    check (value (p, ParamIDs::globalBypass) >= 0.5f, "loading a preset leaves the global bypass alone");
}

static void testUserPresets()
{
    section ("User presets (Save button)");
    auto folder = juce::File::createTempFile ("instashadow-presets");
    folder.createDirectory();

    {
        InstaShadowProcessor p;
        auto& pm = p.getPresetManager();
        pm.setUserPresetFolder (folder);

        juce::Random rng (99);
        randomise (p, rng);
        setValue (p, ParamIDs::globalBypass, 0.0f);
        std::map<juce::String, float> saved;
        for (auto* id : ParamIDs::all) saved[id] = param (p, id).getValue();

        check (pm.saveUserPreset ("My Bus Glue"), "save user preset");
        auto file = folder.getChildFile (juce::String ("My Bus Glue") + PresetManager::fileExtension);
        check (file.existsAsFile(), "preset file written: " + file.getFileName());
        check (pm.getCurrentName() == "My Bus Glue" && ! pm.isModified(), "current preset = the saved one");

        p.setCurrentProgram (0);
        check (pm.getCurrentName() == "Default", "switched to another preset");

        int index = -1;
        for (int i = 0; i < (int) pm.getEntries().size(); ++i)
            if (pm.getEntries()[(size_t) i].name == "My Bus Glue") index = i;
        check (index == (int) getFactoryPresets().size(), "user preset listed after the factory presets");
        check (pm.load (index), "load user preset");

        bool same = true;
        for (auto& [id, norm] : saved)
            if (ParamIDs::isPresetParam (id) && std::abs (param (p, id.toRawUTF8()).getValue() - norm) > 1e-5f) same = false;
        check (same, "every preset parameter restored from the file");

        pm.loadNext();
        check (pm.getCurrentIndex() == 0, "next after the last preset wraps to the first");
        pm.loadPrevious();
        check (pm.getCurrentName() == "My Bus Glue", "previous goes back to it");
    }

    folder.deleteRecursively();
}

static void testDeletePresets (const juce::File& snapshotFolder)
{
    section ("Deleting user presets");
    auto folder = juce::File::createTempFile ("instashadow-delete");
    folder.createDirectory();
    auto fileOf = [&] (const juce::String& name) { return folder.getChildFile (name + PresetManager::fileExtension); };
    auto indexOf = [] (PresetManager& pm, const juce::String& name)
    {
        for (int i = 0; i < (int) pm.getEntries().size(); ++i)
            if (pm.getEntries()[(size_t) i].name == name) return i;
        return -1;
    };
    const int numFactory = (int) getFactoryPresets().size();

    {
        InstaShadowProcessor p;
        auto& pm = p.getPresetManager();
        pm.setUserPresetFolder (folder);
        pm.setMoveToTrash (false);

        juce::Random rng (7);
        for (auto* name : { "A Glue", "B Punch", "C Air" })
        {
            randomise (p, rng);
            pm.saveUserPreset (name);
        }
        check ((int) pm.getEntries().size() == numFactory + 3 && pm.getCurrentName() == "C Air", "three user presets saved");

        bool factoryRefused = true;
        for (int i = 0; i < numFactory; ++i)
            factoryRefused = factoryRefused && ! pm.canDelete (i) && ! pm.deleteUserPreset (i);
        check (factoryRefused && (int) pm.getEntries().size() == numFactory + 3, "factory presets can't be deleted");
        check (! pm.deleteUserPreset (-1) && ! pm.deleteUserPreset (1000), "out-of-range index refused");

        // Another preset: the current one stays current (its index moves up)
        auto aFile = fileOf ("A Glue");
        check (pm.deleteUserPreset (indexOf (pm, "A Glue")), "delete a user preset that isn't the current one");
        check (! aFile.exists() && indexOf (pm, "A Glue") < 0 && pm.findUserPreset (aFile) < 0, "...its file and entry are gone");
        check (pm.getCurrentName() == "C Air" && pm.getCurrentIndex() == indexOf (pm, "C Air") && ! pm.isModified(),
               "...the current preset is still current, at its new index");

        // The current preset: the sound stays, without a name
        std::map<juce::String, float> before;
        for (auto* id : ParamIDs::all) before[id] = param (p, id).getValue();
        check (pm.deleteUserPreset (pm.getCurrentIndex()), "delete the current preset");
        bool same = true;
        for (auto& [id, norm] : before)
            if (std::abs (param (p, id.toRawUTF8()).getValue() - norm) > 1e-6f) same = false;
        check (same, "...the settings don't change");
        check (! fileOf ("C Air").exists() && pm.getCurrentIndex() == -1 && pm.getCurrentName().isEmpty() && ! pm.isModified(),
               "...no current preset any more");

        auto mb = getState (p);
        InstaShadowProcessor q;
        q.getPresetManager().setUserPresetFolder (folder);
        q.setStateInformation (mb.getData(), (int) mb.getSize());
        check (q.getPresetManager().getCurrentName().isEmpty() && q.getPresetManager().getCurrentIndex() == -1
                 && std::abs (param (q, ParamIDs::optoThreshold).getValue() - before[ParamIDs::optoThreshold]) < 1e-6f,
               "...a saved state recalls the settings without a preset name");

        pm.loadNext();
        check (pm.getCurrentIndex() == 0, "next after deleting the current preset goes to the first");
        check ((int) pm.getEntries().size() == numFactory + 1 && indexOf (pm, "B Punch") == numFactory, "one user preset left");

        // A file removed behind the plugin's back
        fileOf ("B Punch").deleteFile();
        check (pm.deleteUserPreset (indexOf (pm, "B Punch")), "deleting a preset whose file is already gone succeeds");
        check ((int) pm.getEntries().size() == numFactory, "only the factory presets are left");
    }

    section ("Deleting presets in the editor (confirmation, long list)");
    {
        InstaShadowProcessor p;
        auto& pm = p.getPresetManager();
        pm.setUserPresetFolder (folder);
        pm.setMoveToTrash (false);
        for (int i = 1; i <= 40; ++i)
            pm.saveUserPreset ("Preset " + juce::String (i).paddedLeft ('0', 2));

        std::unique_ptr<juce::AudioProcessorEditor> base (p.createEditor());
        auto* ed = dynamic_cast<InstaShadowEditor*> (base.get());
        auto& panel = ed->getDeletePanel();
        pump (100);

        ed->askToDeletePreset (2);
        check (! panel.isVisible(), "no confirmation for a factory preset");

        auto f05 = fileOf ("Preset 05");
        ed->askToDeletePreset (indexOf (pm, "Preset 05"));
        check (panel.isVisible() && panel.getPresetName() == "Preset 05", "delete asks for confirmation first");
        check (f05.existsAsFile(), "...nothing deleted while it asks");
        panel.cancel();
        check (! panel.isVisible() && f05.existsAsFile() && indexOf (pm, "Preset 05") >= 0, "Cancel keeps the preset");

        ed->askToDeletePreset (indexOf (pm, "Preset 05"));
        check (panel.keyPressed (juce::KeyPress (juce::KeyPress::escapeKey)) && ! panel.isVisible() && f05.existsAsFile(),
               "Escape keeps the preset");

        ed->askToDeletePreset (indexOf (pm, "Preset 05"));
        panel.confirm();
        check (! panel.isVisible() && ! f05.exists() && indexOf (pm, "Preset 05") < 0, "Delete removes it");

        auto f06 = fileOf ("Preset 06");
        ed->askToDeletePreset (indexOf (pm, "Preset 06"));
        check (panel.keyPressed (juce::KeyPress (juce::KeyPress::returnKey)) && ! f06.exists(), "Return confirms");

        // The list changes while it asks: the right file is still the one deleted
        ed->askToDeletePreset (indexOf (pm, "Preset 20"));
        pm.saveUserPreset ("Preset 00");   // shifts every user preset down by one
        panel.confirm();
        check (! fileOf ("Preset 20").exists() && fileOf ("Preset 21").existsAsFile() && fileOf ("Preset 19").existsAsFile(),
               "the file asked about is the one deleted, even if the list changed");
        check ((int) pm.getEntries().size() == numFactory + 38, "38 user presets left");

        // The menu with a long list stays inside the editor and scrolls there
        // (JUCE places menus relative to a screen; a process without one, e.g. started over ssh on a Mac, can't open them)
        const auto& content = ed->getContent();
        if (juce::Desktop::getInstance().getDisplays().displays.isEmpty())
            std::cout << "  info  no screen available: preset menu checks skipped" << std::endl;
        else
        {
            juce::Array<juce::Component*> childrenBefore;
            for (auto* c : content.getChildren()) childrenBefore.add (c);
            auto desktopBefore = juce::Desktop::getInstance().getNumComponents();
            ed->showPresetMenu();   // (checked right away: a background test process closes menus on its next tick)
            juce::Component* menuWindow = nullptr;
            for (auto* c : content.getChildren())
                if (! childrenBefore.contains (c)) menuWindow = c;
            check (menuWindow != nullptr && menuWindow->isVisible(), "the preset menu opens inside the editor");
            check (juce::Desktop::getInstance().getNumComponents() == desktopBefore, "...not as a separate window");
            if (menuWindow != nullptr)
            {
                std::cout << "  info  menu bounds " << menuWindow->getBounds().toString() << " (editor 1000 x 540, 49 entries)" << std::endl;
                check (content.getLocalBounds().contains (menuWindow->getBounds()), "...within the editor's bounds, however long the list");
                if (snapshotFolder != juce::File())
                {
                    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
                    auto file = snapshotFolder.getChildFile ("preset-menu-long@2x.png");
                    file.deleteFile();
                    juce::FileOutputStream out (file);
                    juce::PNGImageFormat png;
                    check (out.openedOk() && png.writeImageToStream (img, out), "snapshot " + file.getFileName());
                }
            }
        }

        if (snapshotFolder != juce::File())
        {
            auto snap = [&] (const juce::String& name)
            {
                auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
                auto file = snapshotFolder.getChildFile (name);
                file.deleteFile();
                juce::FileOutputStream out (file);
                juce::PNGImageFormat png;
                check (out.openedOk() && png.writeImageToStream (img, out), "snapshot " + name);
            };
            juce::PopupMenu::dismissAllActiveMenus();
            pump (100);
            for (int t = 0; t < numThemes; ++t)
            {
                ed->setTheme ((ThemeId) t);
                ed->askToDeletePreset (indexOf (pm, "Preset 07"));
                pump (50);
                snap ("delete-confirm-" + juce::String (t) + "@2x.png");
                panel.cancel();
            }
        }
        juce::PopupMenu::dismissAllActiveMenus();
        pump (100);
        base.reset();
    }

    folder.deleteRecursively();
}

static void testEditorRecall()
{
    section ("Issue #1: a restored state survives an open editor");
    InstaShadowProcessor p;
    p.prepareToPlay (48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    pump (100);

    V11Values v;
    v.globalBypass = false;
    auto old = makeV11State (v);
    p.setStateInformation (old.getData(), (int) old.getSize());   // e.g. REAPER loading a preset
    pump (400);   // v1.1's editor wrote its knob values back into the engine every 33 ms here

    checkV11Values (p, v, "after 400 ms with the editor open: ");
    auto mb = getState (p);
    InstaShadowProcessor q;
    q.setStateInformation (mb.getData(), (int) mb.getSize());
    checkV11Values (q, v, "saved again: ");

    // A preset loaded while the editor is open
    p.setCurrentProgram (4);
    pump (200);
    check (std::abs (value (p, ParamIDs::vcaThreshold) - getFactoryPresets()[4].vcaThreshold) < 1e-4f,
           "program change with the editor open sticks");
    ed.reset();
}

static void testMeters (const juce::File& snapshotFolder)
{
    InstaShadowProcessor p;
    const double sr = 48000.0;
    const int block = 1600;   // one 30 Hz tick's worth of audio
    p.prepareToPlay (sr, block);
    setValue (p, ParamIDs::optoThreshold, -30.0f);
    setValue (p, ParamIDs::vcaThreshold, -24.0f);
    setValue (p, ParamIDs::vcaRatio, 3.0f);

    std::unique_ptr<juce::AudioProcessorEditor> base (p.createEditor());
    auto* ed = dynamic_cast<InstaShadowEditor*> (base.get());
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    double phase = 0.0;

    for (int t = 0; t < numThemes; ++t)
    {
        auto themeId = (ThemeId) t;

        // Silence until the meters stop: the optical cell releases over many seconds and
        // the needles follow it. Then nothing may repaint while the input stays silent.
        auto settle = [&] (const juce::String& what)
        {
            int ticks = 0, quietTicks = 0;
            auto last = ed->meterRepaints.count.load();
            for (; ticks < 30 * 60 && ! (quietTicks >= 90 && ed->getTimerRate() == InstaShadowEditor::idleRate); ++ticks)
            {
                buf.clear();
                p.processBlock (buf, midi);
                ed->stepMetersForTest();
                auto now = ed->meterRepaints.count.load();
                quietTicks = (now == last) ? quietTicks + 1 : 0;
                last = now;
            }
            check (quietTicks >= 90 && ed->getTimerRate() == InstaShadowEditor::idleRate,
                   what + ": meters at rest " + juce::String ((ticks - quietTicks) / 30.0, 1)
                   + " s after the input went silent (opto GR " + juce::String (p.getEngine().optoGrDb.load(), 3) + " dB)");
            auto before = ed->meterRepaints.count.load();
            for (int i = 0; i < 300; ++i)
            {
                buf.clear();
                p.processBlock (buf, midi);
                ed->stepMetersForTest();
            }
            check (ed->meterRepaints.count.load() == before && ed->getTimerRate() == InstaShadowEditor::idleRate,
                   what + ": then 0 meter repaints in 10 s, timer at " + juce::String (ed->getTimerRate()) + " Hz");
        };
        const char* names[] = { "Graphite & Brass", "Mastering Console", "Ivory Rack" };
        section (juce::String ("Meters, theme ") + names[t]);
        ed->setTheme (themeId);

        // Silence first: should go idle with no repaints
        settle ("silence");
        auto before = ed->meterRepaints.count.load();

        // Audio: meters move and repaint
        before = ed->meterRepaints.count.load();
        for (int i = 0; i < 45; ++i)
        {
            fillSine (buf, phase, 0.5f, 220.0, sr);
            p.processBlock (buf, midi);
            ed->stepMetersForTest();
        }
        auto during = ed->meterRepaints.count.load() - before;
        check (ed->getTimerRate() == InstaShadowEditor::activeRate, "audio: timer at the active rate");
        check (during > 10, "audio: meters repainted " + juce::String (during) + " times in 45 ticks");
        check (p.getEngine().optoGrDb.load() < -0.5f && p.getEngine().vcaGrDb.load() < -0.5f, "audio: both stages reduce gain");

        // Painting a meter region: at 100 % size the background is not painted at all
        // (the meters are opaque); scaled, only a copy of the cached background is made.
        for (int width : { 1000, 1370 })
        {
            ed->setSize (width, width * designHeight / designWidth);
            juce::Component* meter = nullptr;
            for (auto* c : ed->getContent().getChildren())
                if (c->isVisible() && (dynamic_cast<NeedleMeter*> (c) != nullptr || dynamic_cast<SegmentMeter*> (c) != nullptr))
                    { meter = c; break; }

            juce::Image img (juce::Image::ARGB, ed->getWidth(), ed->getHeight(), true);
            {
                juce::Graphics g (img);
                ed->paintEntireComponent (g, true);   // first full paint renders the background once
            }
            auto area = ed->getLocalArea (meter, meter->getLocalBounds());
            juce::Graphics g (img);
            g.reduceClipRegion (area);
            auto contentBefore = ed->contentPaints;
            auto rendersBefore = ed->backgroundRenders;
            ed->paintEntireComponent (g, true);   // what the window does for a meter's dirty region
            if (width == 1000)
                check (ed->contentPaints == contentBefore, "100 %: a meter repaint does not touch the editor background");
            else
                check (ed->backgroundRenders == rendersBefore, "137 %: a meter repaint only copies the cached background");
        }
        ed->setSize (designWidth, designHeight);

        if (snapshotFolder != juce::File())
        {
            for (float scale : { 1.0f, 2.0f })
            {
                auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale);
                auto file = snapshotFolder.getChildFile (juce::String ("theme-") + juce::String (t) + "-"
                                                         + juce::String (names[t]).replace (" & ", "-").replace (" ", "-").toLowerCase()
                                                         + (scale > 1.0f ? "@2x" : "") + ".png");
                file.deleteFile();
                juce::FileOutputStream out (file);
                juce::PNGImageFormat png;
                check (out.openedOk() && png.writeImageToStream (img, out), "snapshot " + file.getFileName());
            }
        }

        // Back to silence: the needles fall back and the editor goes quiet
        settle ("silence after audio");

        // Host stops processing in the middle of audio: meters return to rest, then go quiet
        for (int i = 0; i < 20; ++i)
        {
            fillSine (buf, phase, 0.5f, 220.0, sr);
            p.processBlock (buf, midi);
            ed->stepMetersForTest();
        }
        for (int i = 0; i < 200; ++i)
            ed->stepMetersForTest();   // no processBlock calls at all
        before = ed->meterRepaints.count.load();
        for (int i = 0; i < 60; ++i)
            ed->stepMetersForTest();
        check (ed->getTimerRate() == InstaShadowEditor::idleRate && ed->meterRepaints.count.load() == before,
               "transport stopped (no processBlock): meters rest, no repaints");
    }

    // Input-level mode (click on a meter)
    section ("Meters, input-level mode");
    ed->setTheme (ThemeId::graphite);
    ed->setMeterMode (MeterMode::inputLevel);
    auto before = ed->meterRepaints.count.load();
    for (int i = 0; i < 30; ++i)
    {
        fillSine (buf, phase, 0.5f, 220.0, sr);
        p.processBlock (buf, midi);
        ed->stepMetersForTest();
    }
    check (ed->meterRepaints.count.load() > before, "input mode: meters move with audio");
    ed->setMeterMode (MeterMode::gainReduction);
    base.reset();
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    juce::File snapshots;
    for (int i = 1; i < argc - 1; ++i)
        if (juce::String (argv[i]) == "--snapshots")
            snapshots = juce::File::getCurrentWorkingDirectory().getChildFile (argv[i + 1]);
    if (snapshots != juce::File())
        snapshots.createDirectory();

    testParameters();
    testStateRoundTrip();
    testLegacyState();
    testPrograms();
    testUserPresets();
    testDeletePresets (snapshots);
    testEditorRecall();
    testMeters (snapshots);

    std::cout << "\n" << (checks - failures) << "/" << checks << " checks passed" << std::endl;
    return failures == 0 ? 0 : 1;
}
