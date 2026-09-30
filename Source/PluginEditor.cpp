/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Adaptive_noise_gateAudioProcessorEditor::Adaptive_noise_gateAudioProcessorEditor (Adaptive_noise_gateAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),

    thresholdAttachment(
        p.apvts,
        "threshold",
        thresholdSlider),

    attackAttachment(
        p.apvts,
        "attack",
        attackSlider),

    holdAttachment(
        p.apvts,
        "hold",
        holdSlider),

    releaseAttachment(
        p.apvts,
        "release",
        releaseSlider),

    rangeAttachment(
        p.apvts,
        "range",
        rangeSlider),

    noiseMarginAttachment(
        p.apvts,
        "noiseMargin",
        noiseMarginSlider),

    bypassAttachment(
        p.apvts,
        "bypass",
        bypassButton)
{
    setupSlider(thresholdSlider);
    setupSlider(attackSlider);
    setupSlider(holdSlider);
    setupSlider(releaseSlider);
    setupSlider(rangeSlider);
    setupSlider(noiseMarginSlider);

    setupLabel(thresholdLabel, "Threshold");
    setupLabel(attackLabel, "Attack");
    setupLabel(holdLabel, "Hold");
    setupLabel(releaseLabel, "Release");
    setupLabel(rangeLabel, "Range");
    setupLabel(noiseMarginLabel, "Noise Margin");


    bypassButton.setButtonText("Bypass");
    addAndMakeVisible(bypassButton);


    thresholdSlider.setName("Threshold");
    attackSlider.setName("Attack");
    holdSlider.setName("Hold");
    releaseSlider.setName("Release");
    rangeSlider.setName("Range");
    noiseMarginSlider.setName("Noise Margin");

    thresholdSlider.setTooltip("Manual gate threshold in decibels");
    attackSlider.setTooltip("Time used to open the gate");
    holdSlider.setTooltip("Time during which the gate stays open");
    releaseSlider.setTooltip("Time used to close the gate");
    rangeSlider.setTooltip("Maximum attenuation applied by the gate");
    noiseMarginSlider.setTooltip("Distance above the estimated noise floor");

    setSize(700, 450);
}

Adaptive_noise_gateAudioProcessorEditor::~Adaptive_noise_gateAudioProcessorEditor()
{
}

//==============================================================================
void Adaptive_noise_gateAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
}

void Adaptive_noise_gateAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    auto topArea = area.removeFromTop(50);
    bypassButton.setBounds(topArea.removeFromRight(100));

    auto sliderArea = area.reduced(10);

    auto row = sliderArea.removeFromTop(160);

    auto thresholdArea = row.removeFromLeft(105).reduced(5);
    auto attackArea = row.removeFromLeft(105).reduced(5);
    auto holdArea = row.removeFromLeft(105).reduced(5);
    auto releaseArea = row.removeFromLeft(105).reduced(5);
    auto rangeArea = row.removeFromLeft(105).reduced(5);
    auto noiseMarginArea = row.reduced(5);

    thresholdLabel.setBounds(thresholdArea.removeFromTop(25));
    thresholdSlider.setBounds(thresholdArea);

    attackLabel.setBounds(attackArea.removeFromTop(25));
    attackSlider.setBounds(attackArea);

    holdLabel.setBounds(holdArea.removeFromTop(25));
    holdSlider.setBounds(holdArea);

    releaseLabel.setBounds(releaseArea.removeFromTop(25));
    releaseSlider.setBounds(releaseArea);

    rangeLabel.setBounds(rangeArea.removeFromTop(25));
    rangeSlider.setBounds(rangeArea);

    noiseMarginLabel.setBounds(noiseMarginArea.removeFromTop(25));
    noiseMarginSlider.setBounds(noiseMarginArea);

}


void Adaptive_noise_gateAudioProcessorEditor::setupSlider(
    juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        false,
        80,
        20);

    addAndMakeVisible(slider);
}

void Adaptive_noise_gateAudioProcessorEditor::setupLabel(
    juce::Label& label,
    const juce::String& text)
{
    label.setText(
        text,
        juce::dontSendNotification);

    label.setJustificationType(
        juce::Justification::centred);

    label.setColour(
        juce::Label::textColourId,
        juce::Colours::white);

    addAndMakeVisible(label);
}
