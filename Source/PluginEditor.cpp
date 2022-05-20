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
    using namespace juce;
    
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);
    
    // Area where we draw response curve
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    auto width = responseArea.getWidth();
    
    // Obtaining element in the chain
    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    
    // Needed for the getMagToFreq function
    auto sampleRate = audioProcessor.getSampleRate();
    
    // Vector to store magnitudes
    std::vector<double> magnitudes;
    
    // Pre-allocation one magnitude per pixel
    magnitudes.resize(width);
    
    // Iterate through each pixel and comput magnitude at that frequency
    for(int i = 0; i < width; ++i)
    {
        // Magnitudes are expressed in gain units (multiplicative)
        double magnitude = 1.0;
        
        // Mapping normalised number to its frequency within the human hearing range
        auto frequency = mapToLog10(double(i) / double(width), 20.0, 20000.0);
        
        // If peak is bypassed, no need to do the computation
        if (!monoChain.isBypassed<ChainPositions::Peak>())
            magnitude *= peak.coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        
        // If low cut is bypassed, no need to do the computation
        if (!lowcut.isBypassed<0>())
            magnitude *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        if (!lowcut.isBypassed<1>())
            magnitude *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        if (!lowcut.isBypassed<2>())
            magnitude *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        if (!lowcut.isBypassed<3>())
            magnitude *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        
        // If high cut is bypassed, no need to do the computation
        if (!highcut.isBypassed<0>())
            magnitude *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        if (!highcut.isBypassed<1>())
            magnitude *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        if (!highcut.isBypassed<2>())
            magnitude *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        if (!highcut.isBypassed<3>())
            magnitude *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        
        // Convert magnitude into decibels and store it
        magnitudes[i] = Decibels::gainToDecibels(magnitude);
    }
    
    // Convert vector of magnitude into a path to draw it
    Path responseCurve;
    
    // Max and min position in the window
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    
    // Mapping from source input (-24 to +24) to output (the window position)
    // ±24 because peak control can go to ±24)
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };
    
    // Start new subpath with first magnitude.
    // Starts at left edge of component with 1st value = 1st value of magnitudes through the map
    responseCurve.startNewSubPath(responseArea.getX(), map(magnitudes.front()));
    
    for(size_t i = 1; i < magnitudes.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(magnitudes[i]));
    }
    
    g.setColour(Colours::orchid);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.0f, 1.0f);
    
    g.setColour(Colours::mintcream);
    g.strokePath(responseCurve, PathStrokeType(2.0f));
    
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

void SimplyQueueAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue)
{
    // We set our atomic flag to true, triggering the GUI update
    parametersChanged.set(true);
}

void SimplyQueueAudioProcessorEditor::timerCallback()
{
    if(parametersChanged.compareAndSetBool(false, true))
    {
        // Update mono chain, signal repaint
        // Mono chain from apvts is private so we need to add 'free' functions
        
        parametersChanged.set(false);
    }
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
