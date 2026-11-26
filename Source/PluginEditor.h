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
class Adaptive_noise_gateAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    Adaptive_noise_gateAudioProcessorEditor (Adaptive_noise_gateAudioProcessor&);
    ~Adaptive_noise_gateAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    //==============================================================================
    void timerCallback() override;

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

    // Toggle buttons
    juce::ToggleButton bypassButton;
    juce::ToggleButton useAdaptiveButton;

    // Bounds for plugin visualisation window (threshold over time)
    juce::Rectangle<int> visualizerBounds;

    // Attachments - input updates actual values during runtime
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

    // History buffer for scrolling waveform (wykres kroczący)
    static constexpr int historySize = 200;
    std::array<float, historySize> envelopeHistory;
    std::array<float, historySize> thresholdHistory;
    int writeIndex = 0;

    void setupSlider(juce::Slider& slider);
    void setupLabel(juce::Label& label, const juce::String& text);


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Adaptive_noise_gateAudioProcessorEditor)
};
