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
    
    // Parameter modifiers
    juce::Slider thresholdSlider;
    juce::Slider attackSlider;
    juce::Slider holdSlider;
    juce::Slider releaseSlider;
    juce::Slider rangeSlider;
    juce::Slider noiseMarginSlider;

    // Slider names' labels
    juce::Label thresholdLabel;
    juce::Label attackLabel;
    juce::Label holdLabel;
    juce::Label releaseLabel;
    juce::Label rangeLabel;
    juce::Label noiseMarginLabel;


    juce::ToggleButton bypassButton;
    juce::ToggleButton useAdaptiveButton;

    juce::AudioProcessorValueTreeState::SliderAttachment
        thresholdAttachment;

    juce::AudioProcessorValueTreeState::SliderAttachment
        attackAttachment;

    juce::AudioProcessorValueTreeState::SliderAttachment
        holdAttachment;

    juce::AudioProcessorValueTreeState::SliderAttachment
        releaseAttachment;

    juce::AudioProcessorValueTreeState::SliderAttachment
        rangeAttachment;

    juce::AudioProcessorValueTreeState::SliderAttachment
        noiseMarginAttachment;

    juce::AudioProcessorValueTreeState::ButtonAttachment
        bypassAttachment;
    
    juce::AudioProcessorValueTreeState::ButtonAttachment
        useAdaptiveAttachment;

    void setupSlider(juce::Slider& slider);
    void setupLabel(juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Adaptive_noise_gateAudioProcessorEditor)
};
