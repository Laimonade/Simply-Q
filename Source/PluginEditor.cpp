/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimplyQueueAudioProcessorEditor::SimplyQueueAudioProcessorEditor (SimplyQueueAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
// TODO: Use an enum/look up table instead of using raw names (allows to iterates through it)
lowCutFreqSliderAttachment(audioProcessor.apvts, "Low-Cut Freq", lowCutFreqSlider),
highCutFreqSliderAttachment(audioProcessor.apvts, "High-Cut Freq", highCutFreqSlider),
peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
peakQSliderAttachment(audioProcessor.apvts, "Peak Q", peakQSlider),
lowCutSlopeSliderAttachment(audioProcessor.apvts, "Low-Cut Slope", lowCutSlopeSlider),
highCutSlopeSliderAttachment(audioProcessor.apvts, "High-Cut Slope", highCutSlopeSlider)

{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    for (auto* sliders : getSliders())
    {
        addAndMakeVisible(sliders);
    }
    
    
    setSize (600, 400);
}

SimplyQueueAudioProcessorEditor::~SimplyQueueAudioProcessorEditor()
{
}

//==============================================================================
void SimplyQueueAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText("Let's queue together", getLocalBounds(), juce::Justification::centred, 1);
}

void SimplyQueueAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    auto bounds = getLocalBounds();
    
    // Top 1/3 or display: response of EQ
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    
    // 1/3 of the display on left
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    // 1/3 of display right (width = 2/3, so * 0.5 = 1/3)
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);
    
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQSlider.setBounds(bounds);
    
}

std::vector<juce::Component*> SimplyQueueAudioProcessorEditor::getSliders()
{
    return
    {
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &peakFreqSlider,
        &peakGainSlider,
        &peakQSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider
    };
}
