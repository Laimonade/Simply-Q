/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                                        juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
        
    }
};


//==============================================================================
/**
*/
class SimplyQueueAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SimplyQueueAudioProcessorEditor (SimplyQueueAudioProcessor&);
    ~SimplyQueueAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimplyQueueAudioProcessor& audioProcessor;
    
    // Adding sliders
    CustomRotarySlider lowCutFreqSlider,
    highCutFreqSlider,
    peakFreqSlider,
    peakGainSlider,
    peakQSlider,
    lowCutSlopeSlider,
    highCutSlopeSlider;
    
    // apvts alias to connect GUI sliders to DSP
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    
    // Creating 1 attachment for each slider
    Attachment lowCutFreqSliderAttachment,
    highCutFreqSliderAttachment,
    peakFreqSliderAttachment,
    peakGainSliderAttachment,
    peakQSliderAttachment,
    lowCutSlopeSliderAttachment,
    highCutSlopeSliderAttachment;
    
    // Vectorise sliders for each of access
    std::vector<juce::Component*> getSliders();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimplyQueueAudioProcessorEditor)
};
