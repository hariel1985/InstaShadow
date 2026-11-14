// Loads a built InstaShadow (VST3 or AU) the way a DAW does and checks what
// issue #1 is about: parameters, presets/programs, and whether a state set by
// the host survives while the editor is open.
//
//   InstaShadowHostTest <path to InstaShadow.vst3 or .component>
//
// Works with v1.1 binaries too (to show the old behaviour).

#include <JuceHeader.h>

namespace
{
    int failures = 0, checks = 0;

    void check (bool ok, const juce::String& what)
    {
        ++checks;
        if (! ok) ++failures;
        std::cout << (ok ? "  ok    " : "  FAIL  ") << what << std::endl;
    }

    void info (const juce::String& s) { std::cout << "  info  " << s << std::endl; }

    void pump (int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil (ms); }

    constexpr juce::uint32 xmlMagic = 0x21324356;   // AudioProcessor::copyXmlToBinary

    // Finds the plugin's own XML (InstaShadowState) inside a host state blob
    // (JUCE's VST3 host wraps the IComponent state in XML + base64).
    struct Located { juce::MemoryBlock component; size_t offset = 0; juce::uint32 size = 0; };

    std::unique_ptr<juce::XmlElement> hostXml (const juce::MemoryBlock& mb)
    {
        return juce::AudioProcessor::getXmlFromBinary (mb.getData(), (int) mb.getSize());
    }

    bool locate (const juce::MemoryBlock& hostState, Located& out)
    {
        auto xml = hostXml (hostState);
        if (xml == nullptr) return false;
        auto* comp = xml->getChildByName ("IComponent");
        if (comp == nullptr) return false;
        out.component.reset();
        out.component.fromBase64Encoding (comp->getAllSubText());
        auto* d = static_cast<const char*> (out.component.getData());
        for (size_t i = 0; i + 8 < out.component.getSize(); ++i)
        {
            juce::uint32 magic, size;
            std::memcpy (&magic, d + i, 4);
            if (juce::ByteOrder::swapIfBigEndian (magic) != xmlMagic) continue;
            std::memcpy (&size, d + i + 4, 4);
            out.offset = i;
            out.size = juce::ByteOrder::swapIfBigEndian (size);
            return true;
        }
        return false;
    }

    std::unique_ptr<juce::XmlElement> pluginXml (const juce::MemoryBlock& hostState)
    {
        Located l;
        if (! locate (hostState, l)) return nullptr;
        auto* d = static_cast<const char*> (l.component.getData()) + l.offset + 8;
        return juce::parseXML (juce::String::fromUTF8 (d, (int) l.size));
    }

    juce::MemoryBlock withPluginXml (const juce::MemoryBlock& hostState, const juce::XmlElement& newXml)
    {
        Located l;
        if (! locate (hostState, l)) return {};
        auto text = newXml.toString (juce::XmlElement::TextFormat().singleLine().withoutHeader());
        juce::MemoryBlock blob;
        juce::MemoryOutputStream out (blob, false);
        out.write (l.component.getData(), l.offset);
        out.writeInt ((int) xmlMagic);
        out.writeInt ((int) text.getNumBytesAsUTF8() + 1);
        out.write (text.toRawUTF8(), text.getNumBytesAsUTF8() + 1);
        auto restStart = l.offset + 8 + l.size;
        if (restStart < l.component.getSize())
            out.write (static_cast<const char*> (l.component.getData()) + restStart, l.component.getSize() - restStart);
        out.flush();

        auto xml = hostXml (hostState);
        auto* comp = xml->getChildByName ("IComponent");
        comp->deleteAllTextElements();
        comp->addTextElement (blob.toBase64Encoding());
        juce::MemoryBlock result;
        juce::AudioProcessor::copyXmlToBinary (*xml, result);
        return result;
    }

    // The values to recall (different from every default).
    void setRecallValues (juce::XmlElement& x)
    {
        x.setAttribute ("optoThreshold", -31.5);
        x.setAttribute ("optoGain", 4.5);
        x.setAttribute ("vcaThreshold", -12.25);
        x.setAttribute ("vcaRatio", 4);
        x.setAttribute ("transformerType", 2);
        x.setAttribute ("outputGain", -2.0);
        x.setAttribute ("stereoLink", 0);
        // A v1.2 state carries PARAM children; drop them so the attributes decide.
        while (auto* c = x.getChildByName ("PARAM"))
            x.removeChildElement (c, true);
        x.removeAttribute ("presetName");
    }

    bool hasRecallValues (const juce::XmlElement& x)
    {
        auto near = [] (double a, double b) { return std::abs (a - b) < 1.0e-3; };
        return near (x.getDoubleAttribute ("optoThreshold"), -31.5) && near (x.getDoubleAttribute ("optoGain"), 4.5)
            && near (x.getDoubleAttribute ("vcaThreshold"), -12.25) && x.getIntAttribute ("vcaRatio") == 4
            && x.getIntAttribute ("transformerType") == 2 && near (x.getDoubleAttribute ("outputGain"), -2.0)
            && ! x.getBoolAttribute ("stereoLink");
    }

    juce::String describe (const juce::XmlElement& x)
    {
        return "optoThreshold=" + x.getStringAttribute ("optoThreshold") + " vcaRatio=" + x.getStringAttribute ("vcaRatio")
             + " transformerType=" + x.getStringAttribute ("transformerType") + " outputGain=" + x.getStringAttribute ("outputGain");
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    if (argc < 2) { std::cout << "usage: InstaShadowHostTest <plugin>" << std::endl; return 2; }

    juce::AudioPluginFormatManager formats;
    juce::addDefaultFormatsToManager (formats);

    juce::String path = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]).getFullPathName();
    juce::OwnedArray<juce::PluginDescription> found;
    for (auto* f : formats.getFormats())
        if (f->fileMightContainThisPluginType (path))
            f->findAllTypesForFile (found, path);

    if (found.isEmpty()) { std::cout << "no plugin found in " << path << std::endl; return 2; }

    auto& desc = *found[0];
    std::cout << "== " << desc.pluginFormatName << " " << desc.name << " " << desc.version << std::endl;

    juce::String error;
    auto plugin = formats.createPluginInstance (desc, 48000.0, 512, error);
    if (plugin == nullptr) { std::cout << "load failed: " << error << std::endl; return 2; }

    plugin->enableAllBuses();
    plugin->prepareToPlay (48000.0, 512);

    auto& params = plugin->getParameters();
    info ("host sees " + juce::String (params.size()) + " parameters, " + juce::String (plugin->getNumPrograms()) + " programs");
    check (params.size() >= 14, "the host sees the plugin's parameters (automation, generic UI)");
    juce::StringArray names;
    for (auto* p : params) names.add (p->getName (32));
    info ("parameters: " + names.joinIntoString (", "));

    juce::StringArray programs;
    for (int i = 0; i < plugin->getNumPrograms(); ++i) programs.add ("'" + plugin->getProgramName (i) + "'");
    info ("programs: " + programs.joinIntoString (", "));
    check (plugin->getNumPrograms() > 1, "factory presets are offered to the host");

    // --- The issue #1 scenario: the host recalls a preset while the editor is open
    std::unique_ptr<juce::AudioProcessorEditor> editor (plugin->createEditorIfNeeded());
    check (editor != nullptr, "editor created");
    pump (300);

    juce::MemoryBlock original;
    plugin->getStateInformation (original);
    auto xml = pluginXml (original);
    check (xml != nullptr && xml->hasTagName ("InstaShadowState"), "plugin state found in the host state");
    if (xml == nullptr) return 1;

    setRecallValues (*xml);
    auto preset = withPluginXml (original, *xml);
    plugin->setStateInformation (preset.getData(), (int) preset.getSize());   // what REAPER does when a preset is chosen

    juce::MemoryBlock rightAfter;
    plugin->getStateInformation (rightAfter);
    auto afterXml = pluginXml (rightAfter);
    info ("state right after the recall: " + describe (*afterXml));

    pump (500);   // the editor is open
    juce::MemoryBlock later;
    plugin->getStateInformation (later);
    auto laterXml = pluginXml (later);
    info ("state 500 ms later:           " + describe (*laterXml));
    check (hasRecallValues (*laterXml), "the recalled preset is still there 500 ms later (editor open)");

    // Save it as a host preset and load it into a fresh instance
    auto second = formats.createPluginInstance (desc, 48000.0, 512, error);
    second->enableAllBuses();
    second->prepareToPlay (48000.0, 512);
    second->setStateInformation (later.getData(), (int) later.getSize());
    juce::MemoryBlock secondState;
    second->getStateInformation (secondState);
    check (hasRecallValues (*pluginXml (secondState)), "host preset loads into a new instance");

    // Program change through the host
    if (plugin->getNumPrograms() > 1)
    {
        plugin->setCurrentProgram (1);
        pump (200);
        juce::MemoryBlock progState;
        plugin->getStateInformation (progState);
        auto px = pluginXml (progState);
        info ("after program 1: " + describe (*px) + " name '" + px->getStringAttribute ("presetName") + "'");
        check (std::abs (px->getDoubleAttribute ("vcaThreshold") + 14.0) < 1.0e-3 && px->getIntAttribute ("vcaRelease") == 5,
               "program change applies the factory preset");
    }

    // Parameter automation reaches the plugin and its state
    if (params.size() >= 14)
    {
        for (auto* p : params)
            if (p->getName (64) == "Optical Threshold")
                p->setValueNotifyingHost (0.25f);   // -30 dB
        pump (100);
        juce::MemoryBlock autoState;
        plugin->getStateInformation (autoState);
        check (std::abs (pluginXml (autoState)->getDoubleAttribute ("optoThreshold") + 30.0) < 1.0e-3,
               "automating a parameter changes the plugin state");
    }

    editor.reset();
    second.reset();
    plugin.reset();
    std::cout << (checks - failures) << "/" << checks << " checks passed" << std::endl;
    return failures == 0 ? 0 : 1;
}
