#pragma once
#include <JuceHeader.h>
#include "CompressorEngine.h"
#include "Parameters.h"
#include "PresetManager.h"

class InstaShadowProcessor : public juce::AudioProcessor
{
public:
    InstaShadowProcessor();
    ~InstaShadowProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    using AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Programs = factory presets (host preset menus, VST3 program list, AU factory presets).
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    CompressorEngine& getEngine() { return engine; }
    juce::AudioProcessorValueTreeState& getState() { return apvts; }
    PresetManager& getPresetManager() { return presetManager; }

    // Editor preferences, stored in the plugin state.
    int getThemeIndex() const;
    void setThemeIndex (int);
    juce::Point<int> getEditorSize() const;
    void setEditorSize (juce::Point<int>);

    static constexpr int stateVersion = 2;

private:
    CompressorEngine engine;
    juce::AudioProcessorValueTreeState apvts;
    PresetManager presetManager;

    struct RawParams
    {
        std::atomic<float>* optoThreshold = nullptr; std::atomic<float>* optoGain = nullptr;
        std::atomic<float>* optoScHpf = nullptr;     std::atomic<float>* optoBypass = nullptr;
        std::atomic<float>* vcaThreshold = nullptr;  std::atomic<float>* vcaGain = nullptr;
        std::atomic<float>* vcaRatio = nullptr;      std::atomic<float>* vcaAttack = nullptr;
        std::atomic<float>* vcaRelease = nullptr;    std::atomic<float>* vcaBypass = nullptr;
        std::atomic<float>* transformerType = nullptr; std::atomic<float>* outputGain = nullptr;
        std::atomic<float>* stereoLink = nullptr;    std::atomic<float>* globalBypass = nullptr;
    } raw;

    // Program changes from a non-message thread are applied on the message thread.
    struct ProgramChanger : public juce::AsyncUpdater
    {
        explicit ProgramChanger (InstaShadowProcessor& p) : owner (p) {}
        ~ProgramChanger() override { cancelPendingUpdate(); }
        void handleAsyncUpdate() override { owner.presetManager.loadFactory (owner.pendingProgram.load()); }
        InstaShadowProcessor& owner;
    } programChanger;
    std::atomic<int> pendingProgram { 0 };

    void pushParametersToEngine();
    void loadLegacyState (const juce::XmlElement&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstaShadowProcessor)
};
