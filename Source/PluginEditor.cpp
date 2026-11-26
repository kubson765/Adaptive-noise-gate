<<<<<<< HEAD
/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
static const size_t WINDOW_PIXEL_WIDTH = 700;
static const size_t WINDOW_PIXEL_HEIGHT = 450;

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
        bypassButton),

    useAdaptiveAttachment(
        p.apvts,
        "useAdaptive",
        useAdaptiveButton)

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

    useAdaptiveButton.setButtonText("Adaptive Threshold");
    addAndMakeVisible(useAdaptiveButton);


    thresholdSlider.setName("Threshold");
    attackSlider.setName("Attack");
    holdSlider.setName("Hold");
    releaseSlider.setName("Release");
    rangeSlider.setName("Range");
    noiseMarginSlider.setName("Noise Margin");

    thresholdSlider.setTextValueSuffix(" dB");
    attackSlider.setTextValueSuffix(" ms");
    holdSlider.setTextValueSuffix(" ms");
    releaseSlider.setTextValueSuffix(" ms");
    rangeSlider.setTextValueSuffix(" dB");
    noiseMarginSlider.setTextValueSuffix(" dB");

    thresholdSlider.setTooltip("Manual gate threshold in decibels");
    attackSlider.setTooltip("Time used to open the gate");
    holdSlider.setTooltip("Time during which the gate stays open");
    releaseSlider.setTooltip("Time used to close the gate");
    rangeSlider.setTooltip("Maximum attenuation applied by the gate");
    noiseMarginSlider.setTooltip("Distance above the estimated noise floor");

    // Visualizer parameters
    envelopeHistory.fill(-100.0f);
    thresholdHistory.fill(-45.0f);
    startTimerHz(30);

    setSize(WINDOW_PIXEL_WIDTH, WINDOW_PIXEL_HEIGHT);
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

    // Painting the visualizer window
    g.reduceClipRegion(visualizerBounds);
    auto visBounds = visualizerBounds.toFloat();
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(visBounds, 6.0f);

    g.setColour(juce::Colours::grey);
    g.drawRoundedRectangle(visBounds, 6.0f, 1.0f);

    constexpr float minDb = -80.0f;
    constexpr float maxDb = 0.0f;

    // Information mesh - horizontal lines every 20dB
    g.setFont(11.0f);
    for (float db = -60.0f; db <= -20.0f; db += 20.0) {

        float y = juce::jmap(db, -80.0f, 0.0f, visBounds.getBottom(), visBounds.getY());
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawHorizontalLine(static_cast<int>(y), visBounds.getX(), visBounds.getRight());


    }

    juce::Path signalPath;
    juce::Path thresholdPath;

    float stepX = visBounds.getWidth() / (historySize - 1);
    signalPath.startNewSubPath(visBounds.getX(), visBounds.getBottom());

    for (int i{ 0 }; i < historySize;  ++i) {
        
        int idx = (writeIndex + i) % historySize;
        float x = visBounds.getX() + i*stepX;
        
        // Clipping the values at limits
        float envelopeDb = juce::jlimit(-80.0f, 0.0f, envelopeHistory[idx]);
        float thresholdDb = juce::jlimit(-80.0f, 0.0f, thresholdHistory[idx]);

        // Converting dB values into pixel positions
        float y_envelope = juce::jmap(envelopeDb, minDb, maxDb, visBounds.getBottom(), visBounds.getY());
        float y_threshold = juce::jmap(thresholdDb, minDb, maxDb, visBounds.getBottom(), visBounds.getY());

        signalPath.lineTo(x, y_envelope);
        
        if (i == 0)
            thresholdPath.startNewSubPath(x, y_threshold);
        else thresholdPath.lineTo(x, y_threshold);
    }

    if (audioProcessor.isGateOpen())
        g.setColour(juce::Colours::darkgreen.withAlpha(0.3f));
    else g.setColour(juce::Colours::grey.withAlpha(0.3f));
    
    // Current volume line fill
    signalPath.lineTo(visBounds.getRight(), visBounds.getBottom());
    signalPath.closeSubPath();
    g.fillPath(signalPath);
    g.strokePath(signalPath, juce::PathStrokeType(1.5f));

    // Threshold line draw
    g.setColour(juce::Colours::red);
    g.strokePath(thresholdPath, juce::PathStrokeType(2.0f));

}

void Adaptive_noise_gateAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    // Toggle buttons area and positions
    auto topArea = area.removeFromTop(50);
    bypassButton.setBounds(topArea.removeFromRight(100));
    useAdaptiveButton.setBounds(topArea.removeFromRight(100));

    // Sliders with labels area and positions
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

    // Visualization real-time graph area and position
    auto visualizerArea = area.removeFromTop(15);
    visualizerBounds = sliderArea;

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

void Adaptive_noise_gateAudioProcessorEditor::timerCallback()
{
    envelopeHistory[writeIndex] = audioProcessor.getEnvelopeDb();
    thresholdHistory[writeIndex] = audioProcessor.getThresholdDb();

    writeIndex = (writeIndex + 1) % historySize;

    repaint(visualizerBounds);
}
=======
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
        bypassButton),

    useAdaptiveAttachment(
        p.apvts,
        "useAdaptive",
        useAdaptiveButton)

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

    useAdaptiveButton.setButtonText("Adaptive Threshold");
    addAndMakeVisible(useAdaptiveButton);


    thresholdSlider.setName("Threshold");
    attackSlider.setName("Attack");
    holdSlider.setName("Hold");
    releaseSlider.setName("Release");
    rangeSlider.setName("Range");
    noiseMarginSlider.setName("Noise Margin");

    thresholdSlider.setTextValueSuffix(" dB");
    attackSlider.setTextValueSuffix(" ms");
    holdSlider.setTextValueSuffix(" ms");
    releaseSlider.setTextValueSuffix(" ms");
    rangeSlider.setTextValueSuffix(" dB");
    noiseMarginSlider.setTextValueSuffix(" dB");

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
    useAdaptiveButton.setBounds(topArea.removeFromRight(100));

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
>>>>>>> 11ef5eb29471dc9079cf94e09df4d96282e8031c
