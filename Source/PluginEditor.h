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
                                        juce::Slider::TextEntryBoxPosition::NoTextBox){}
};

struct ResponseCurveComponent: juce::Component,
juce::AudioProcessorParameter::Listener,
juce::Timer
{
    ResponseCurveComponent(SimplyQueueAudioProcessor&);
    ~ResponseCurveComponent();
    
    void parameterValueChanged (int parameterIndex, float newValue) override;
    /** Indicates that a parameter change gesture has started.

        E.g. if the user is dragging a slider, this would be called with gestureIsStarting
        being true when they first press the mouse button, and it will be called again with
        gestureIsStarting being false when they release it.

        IMPORTANT NOTE: This will be called synchronously, and many audio processors will
        call it during their audio callback. This means that not only has your handler code
        got to be completely thread-safe, but it's also got to be VERY fast, and avoid
        blocking. If you need to handle this event on your message thread, use this callback
        to trigger an AsyncUpdater or ChangeBroadcaster which you can respond to later on the
        message thread.
    */
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {}

    void timerCallback() override;
    /** Starts the timer and sets the length of interval required.

        If the timer is already started, this will reset it, so the
        time between calling this method and the next timer callback
        will not be less than the interval length passed in.

        @param  intervalInMilliseconds  the interval to use (any value less
                                        than 1 will be rounded up to 1)
    */
    
    void paint(juce::Graphics& g) override;

private:
    SimplyQueueAudioProcessor& audioProcessor;
    
    juce::Atomic<bool> parametersChanged {false};
    
    // Using an instance of monochain used in audio processor, to update reponse curve
    MonoChain monoChain;

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
    
    ResponseCurveComponent responseCurveComponent;
    
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
