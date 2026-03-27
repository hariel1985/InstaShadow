#include "PluginProcessor.h"
#include "PluginEditor.h"

InstaShadowProcessor::InstaShadowProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

InstaShadowProcessor::~InstaShadowProcessor() {}

void InstaShadowProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
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

void InstaShadowProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    engine.processBlock (buffer);
}

juce::AudioProcessorEditor* InstaShadowProcessor::createEditor()
{
    return new InstaShadowEditor (*this);
}

void InstaShadowProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto xml = std::make_unique<juce::XmlElement> ("InstaShadowState");

    xml->setAttribute ("optoThreshold", (double) engine.optoThresholdDb.load());
    xml->setAttribute ("optoGain", (double) engine.optoGainDb.load());
    xml->setAttribute ("optoScHpf", (double) engine.optoScHpfHz.load());
    xml->setAttribute ("optoBypass", engine.optoBypass.load());
    xml->setAttribute ("vcaThreshold", (double) engine.vcaThresholdDb.load());
    xml->setAttribute ("vcaGain", (double) engine.vcaGainDb.load());
    xml->setAttribute ("vcaRatio", engine.vcaRatioIndex.load());
    xml->setAttribute ("vcaAttack", engine.vcaAttackIndex.load());
    xml->setAttribute ("vcaRelease", engine.vcaReleaseIndex.load());
    xml->setAttribute ("vcaBypass", engine.vcaBypass.load());
    xml->setAttribute ("transformerType", engine.transformerType.load());
    xml->setAttribute ("outputGain", (double) engine.outputGainDb.load());
    xml->setAttribute ("stereoLink", engine.stereoLink.load());
    xml->setAttribute ("globalBypass", engine.globalBypass.load());

    copyXmlToBinary (*xml, destData);
}

void InstaShadowProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName ("InstaShadowState"))
        return;

    engine.optoThresholdDb.store ((float) xml->getDoubleAttribute ("optoThreshold", -20.0));
    engine.optoGainDb.store ((float) xml->getDoubleAttribute ("optoGain", 0.0));
    engine.optoScHpfHz.store ((float) xml->getDoubleAttribute ("optoScHpf", 90.0));
    engine.optoBypass.store (xml->getBoolAttribute ("optoBypass", false));
    engine.vcaThresholdDb.store ((float) xml->getDoubleAttribute ("vcaThreshold", -20.0));
    engine.vcaGainDb.store ((float) xml->getDoubleAttribute ("vcaGain", 0.0));
    engine.vcaRatioIndex.store (xml->getIntAttribute ("vcaRatio", 1));
    engine.vcaAttackIndex.store (xml->getIntAttribute ("vcaAttack", 2));
    engine.vcaReleaseIndex.store (xml->getIntAttribute ("vcaRelease", 2));
    engine.vcaBypass.store (xml->getBoolAttribute ("vcaBypass", false));
    engine.transformerType.store (xml->getIntAttribute ("transformerType", 0));
    engine.outputGainDb.store ((float) xml->getDoubleAttribute ("outputGain", 0.0));
    engine.stereoLink.store (xml->getBoolAttribute ("stereoLink", true));
    engine.globalBypass.store (xml->getBoolAttribute ("globalBypass", false));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new InstaShadowProcessor();
}
