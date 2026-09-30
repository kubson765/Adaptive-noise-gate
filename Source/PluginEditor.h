/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class Adaptive_noise_gateAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    Adaptive_noise_gateAudioProcessorEditor (Adaptive_noise_gateAudioProcessor&);
    ~Adaptive_noise_gateAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Adaptive_noise_gateAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Adaptive_noise_gateAudioProcessorEditor)
};
