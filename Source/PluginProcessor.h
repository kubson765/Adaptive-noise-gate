/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class Adaptive_noise_gateAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    Adaptive_noise_gateAudioProcessor();
    ~Adaptive_noise_gateAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

    //==============================================================================
    float getEnvelopeDb() const noexcept { return atomicEnvelopeDb.load(std::memory_order_relaxed); }
    float getNoiseFloorDb() const noexcept { return atomicNoiseFloorDb.load(std::memory_order_relaxed); }
    float getThresholdDb() const noexcept { return atomicThresholdDb.load(std::memory_order_relaxed); }
    float isGateOpen() const noexcept { return atomicGateOpen.load(std::memory_order_relaxed); }


private:

    static juce::AudioProcessorValueTreeState::ParameterLayout
    createParameterLayout();

    double currentSampleRate = 44100.0;     // Double for JUCE type compliance
    float envelopeDb = -100.0f;
    float noiseFloorDb = -100.0f;
    float currentGainDb = 0.0f;
    float currentGainLinear = 1.0f;
    float targetGainDb = 0.0f;
    float targetGainLinear = 1.0f;

    enum class GateState {Closed, Attack, Open, Hold, Release};
    GateState gateState = GateState::Closed;

    int holdCounterSamples = 0;
    int holdSamplesLimit = 0;

    float detectorCoeff = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float noiseCoeff = 0.0f;

    // ===== Atomic variables for UI visualization synchronization and safety =====
    std::atomic<float> atomicEnvelopeDb{ -100.0f };
    std::atomic<float> atomicNoiseFloorDb{ -100.0f };
    std::atomic<float> atomicThresholdDb{ -45.0f };
    std::atomic<bool> atomicGateOpen{ false };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Adaptive_noise_gateAudioProcessor)

};
