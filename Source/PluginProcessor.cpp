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
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
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
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    // 
    //for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    //    buffer.clear (i, 0, buffer.getNumSamples());

    const bool isBypassed =
        apvts.getRawParameterValue("bypass")->load();

    if (isBypassed)
        return;

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

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

    const auto peakDb =
        juce::Decibels::gainToDecibels(peakLevel, -100.0f);

    const auto rmsDb =
        juce::Decibels::gainToDecibels(rmsLevel, -100.0f);

    DBG("Peak: " + juce::String(peakDb)
        + " dB, RMS: " + juce::String(rmsDb) + " dB");




    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // Test modyfikowania sygnału (ściszenie)
        for (int sample{ 0 }; sample < buffer.getNumSamples(); ++sample) {
            channelData[sample] *= 0.2f;
        }

        // ..do something to the data...
    }
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
            "bypass",
            "Bypass",
            false));

    parameters.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            "threshold",
            "Threshold",
            juce::NormalisableRange<float>(-80.0f, 0.0f, 0.1f),
            -45.0f));

    parameters.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            "attack",
            "Attack",
            juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f),
            5.0f));

    parameters.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            "hold",
            "Hold",
            juce::NormalisableRange<float>(0.0f, 500.0f, 1.0f),
            100.0f));

    parameters.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            "release",
            "Release",
            juce::NormalisableRange<float>(10.0f, 2000.0f, 1.0f),
            180.0f));

    parameters.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            "range",
            "Range",
            juce::NormalisableRange<float>(0.0f, 80.0f, 0.1f),
            35.0f));

    parameters.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            "noiseMargin",
            "Noise Margin",
            juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f),
            8.0f));

    return { parameters.begin(), parameters.end() };
}
