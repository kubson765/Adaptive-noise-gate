/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Adaptive_noise_gateAudioProcessor::Adaptive_noise_gateAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}


Adaptive_noise_gateAudioProcessor::~Adaptive_noise_gateAudioProcessor()
{
}

//==============================================================================
const juce::String Adaptive_noise_gateAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Adaptive_noise_gateAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Adaptive_noise_gateAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Adaptive_noise_gateAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Adaptive_noise_gateAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Adaptive_noise_gateAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Adaptive_noise_gateAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Adaptive_noise_gateAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Adaptive_noise_gateAudioProcessor::getProgramName (int index)
{
    return {};
}

void Adaptive_noise_gateAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Adaptive_noise_gateAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused();
    jassert(sampleRate > 0.0);
    currentSampleRate = sampleRate;

    const float blockDuration = static_cast<float>(samplesPerBlock) / static_cast<float>(sampleRate);

    constexpr float detectorTimeMs = 20.0f;
    constexpr float noiseTimeSec = 2.0f;

    const float attackMs = apvts.getRawParameterValue("attack")->load();
    const float releaseMs = apvts.getRawParameterValue("release")->load();
    const float holdMs = apvts.getRawParameterValue("hold")->load();
    holdSamplesLimit = static_cast<int>(holdMs * 0.001f * currentSampleRate);

    detectorCoeff = 1.0f - std::exp(-blockDuration / (detectorTimeMs * 0.001f));

    attackCoeff = 1.0f - std::exp(-1.0f / (attackMs * 0.001f * static_cast<float>(sampleRate)));

    releaseCoeff = 1.0f - std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(sampleRate)));

    noiseCoeff = 1.0f - std::exp(-blockDuration / noiseTimeSec);

    DBG("detectorCoeff = " + juce::String(detectorCoeff));
    DBG("noiseCoeff = " + juce::String(noiseCoeff));

    

    // State reset
    envelopeDb = -100.0f;
    noiseFloorDb = -100.0f;
    currentGainDb = 0.0f;
    currentGainLinear = 1.0f;
    targetGainDb = -apvts.getRawParameterValue("range")->load();
    gateState = GateState::Closed;
    holdCounterSamples = 0;

}

void Adaptive_noise_gateAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Adaptive_noise_gateAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Adaptive_noise_gateAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();




    const bool isBypassed =
        apvts.getRawParameterValue("bypass")->load();
    const float noiseMarginDb = apvts.getRawParameterValue("noiseMargin")->load();
    const float rangeDb = apvts.getRawParameterValue("range")->load();

    if (isBypassed)
        return;

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    const bool useAdaptiveThreshold = apvts.getRawParameterValue("useAdaptive")->load() > 0.5f;
    const float thresholdDb = apvts.getRawParameterValue("threshold")->load();
    const float effectiveThreshold = useAdaptiveThreshold
        ? (noiseFloorDb + noiseMarginDb)
        : thresholdDb;

    float peakLevel = 0.0f;
    float rmsLevel = 0.0f;

    for (int channel = 0; channel < numChannels; ++channel)
    {
        peakLevel = juce::jmax(
            peakLevel,
            buffer.getMagnitude(
                channel,
                0,
                numSamples));

        rmsLevel = juce::jmax(
            rmsLevel,
            buffer.getRMSLevel(
                channel,
                0,
                numSamples));
    }

    const auto peakDb = juce::Decibels::gainToDecibels(peakLevel, -100.0f);

    const auto rmsDb = juce::Decibels::gainToDecibels(rmsLevel, -100.0f);

    envelopeDb += detectorCoeff * (rmsDb - envelopeDb);

    if (noiseFloorDb > envelopeDb)
        noiseFloorDb = envelopeDb;
    else { 
        noiseFloorDb += noiseCoeff * (envelopeDb - noiseFloorDb); 
    }

    DBG("Peak: " + juce::String(peakDb) + " dB, RMS: " + juce::String(rmsDb) + " dB, Envelope: "
        + juce::String(envelopeDb) + " dB, Floor: " + juce::String(noiseFloorDb) + "dB, Effective THRESHOLD: " + juce::String(effectiveThreshold));

    const float openThresholdDb = effectiveThreshold;
    constexpr float hysteresisDb = 3.0f;
    const float closeThresholdDb = effectiveThreshold - hysteresisDb;

    switch (gateState)
    {
    case GateState::Closed:
        targetGainDb = -rangeDb;
        if (envelopeDb > openThresholdDb)
            gateState = GateState::Attack;
        break;

    case GateState::Attack:
        if (envelopeDb > openThresholdDb) {
            targetGainDb = 0.0f;

            if (std::abs(currentGainDb - targetGainDb) < 0.1f) {
                gateState = GateState::Open;
            }
        }
        else
            gateState = GateState::Release;

        break;

    case GateState::Open:
        targetGainDb = 0.0f;
        if (envelopeDb < closeThresholdDb) {
            holdCounterSamples = 0;
            gateState = GateState::Hold;
        }
        break;

    case GateState::Hold:
        if (envelopeDb > openThresholdDb) {
            gateState = GateState::Open;
            targetGainDb = 0.0f;
            break;
        }

        holdCounterSamples += numSamples;
        if (holdCounterSamples >= holdSamplesLimit)
            gateState = GateState::Release;
        break;

    case GateState::Release:
        if (envelopeDb > openThresholdDb)
            gateState = GateState::Attack;
        else {
            targetGainDb = -rangeDb;
        }

        if (std::abs(currentGainDb - targetGainDb) < 0.1f)
            gateState = GateState::Closed;
        break;
    }


    // Applying the gain
    std::array<float*, 2> channelPtrs;

    for (int channel{ 0 }; channel < numChannels; ++channel)
        channelPtrs[channel] = buffer.getWritePointer(channel);

    for (int sample{ 0 }; sample < numSamples; ++sample) {
        if (currentGainDb < targetGainDb)
            currentGainDb += attackCoeff * (targetGainDb - currentGainDb);
        else
            currentGainDb += releaseCoeff * (targetGainDb - currentGainDb);

        currentGainLinear = juce::Decibels::decibelsToGain(currentGainDb);

        for (int channel{ 0 }; channel < numChannels; ++channel)
            channelPtrs[channel][sample] *= currentGainLinear;
    }

    atomicEnvelopeDb.store(envelopeDb, std::memory_order_relaxed);
    atomicNoiseFloorDb.store(noiseFloorDb, std::memory_order_relaxed);
    atomicThresholdDb.store(effectiveThreshold, std::memory_order_relaxed);
    atomicGateOpen.store(gateState == GateState::Open || gateState == GateState::Attack || gateState == GateState::Hold, std::memory_order_relaxed);
}

//==============================================================================
bool Adaptive_noise_gateAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Adaptive_noise_gateAudioProcessor::createEditor()
{
    return new Adaptive_noise_gateAudioProcessorEditor (*this);
}

//==============================================================================
void Adaptive_noise_gateAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    auto state = apvts.copyState();

    std::unique_ptr<juce::XmlElement> xml(
        state.createXml());

    copyXmlToBinary(*xml, destData);
}

void Adaptive_noise_gateAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xmlState(
        getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr &&
        xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(
            juce::ValueTree::fromXml(*xmlState));
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Adaptive_noise_gateAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout
Adaptive_noise_gateAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back(
        std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{ "bypass", 1 }, "Bypass", false));
    
    parameters.push_back(
        std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{ "useAdaptive", 2 }, "Adaptive Gate", false));

    parameters.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ "threshold", 3 },
            "Threshold",
            juce::NormalisableRange<float>(-80.0f, 0.0f, 0.1f),
            -45.0f));

    parameters.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ "attack", 4 },
            "Attack",
            juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f),
            5.0f));

    parameters.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ "hold", 5 },
            "Hold",
            juce::NormalisableRange<float>(0.0f, 500.0f, 1.0f),
            100.0f));

    parameters.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ "release", 6 },
            "Release",
            juce::NormalisableRange<float>(10.0f, 2000.0f, 1.0f),
            180.0f));

    parameters.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ "range", 7 },
            "Range",
            juce::NormalisableRange<float>(0.0f, 80.0f, 0.1f),
            35.0f));

    parameters.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ "noiseMargin", 8 },
            "Noise Margin",
            juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f),
            8.0f));

    return { parameters.begin(), parameters.end() };
}
