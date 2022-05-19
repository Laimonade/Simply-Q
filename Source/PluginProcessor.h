/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>


//==============================================================================

// Enum to express the slope settings
enum SlopeSettings
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};


// Extracting parameters of apvts, data structure representing all parameters values
// Parameters from parameterValueTreeState
struct ChainSettings
{
    float peakFreq {0}, peakGainInDecibels {0}, peakQuality {1.0f};
    float lowCutFreq {0}, highCutFreq {0};
    
    // Init the cut by the 12db filter
    SlopeSettings lowCutSlope {SlopeSettings::Slope_12}, highCutSlope {SlopeSettings::Slope_12};
};

// Helper function giving all the values to the data struct above
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);


//==============================================================================
/**
*/
class SimplyQueueAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimplyQueueAudioProcessor();
    ~SimplyQueueAudioProcessor() override;

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
    
    
    // ======================== ADDED =============================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(); // Static as it doesn't use any member variable
    
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()}; //Binding GUI control to the DSP in processor

private:
    
    using Filter = juce::dsp::IIR::Filter<float>; // Creating a juce dsp filter 'type alias'
    
    // We want cutfilter to have a max of 48db reponse. Each filter are 12db response, so we need to
    // 'side-chain' 4 of them to obtain this selectable 12db-48db. We do this using a processor chain.
    // We pass the processor a single context (audio samples) for the 4 filters.
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    
    // Mono chain: Low cut --> Parametric --> High cut
    // We create a mono chain by having 2 cut filters for the low&high cut
    // and a normal filter for parametric
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    
    // Creating 2 mono chain for stereo processing
    MonoChain leftChain, rightChain;
    
    // Enum representing each filter in the chain. Goes along with the MonoChain above defining each:
    // cut filter, filter, cut filter
    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut
    };
    
    // Update peak filter with the chain settings
    void updatePeakFilter(const ChainSettings& chainSettings);
    
    // Juce coefficient Alias
    using Coefficients = Filter::CoefficientsPtr;
    
    // Helper function to update peak filter coefficients 
    static void updateCoefficients(Coefficients& old, const Coefficients& replacements);
    
    // Template function
    template<int Index, typename ChainType, typename CoefficientType>
    void update(ChainType& chain, const CoefficientType& coefficients)
    {
        updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
        chain.template setBypassed<Index> (false);
    }
    
    // Low/high cut filter coefficient update
    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(ChainType& leftLowCut,
                         const CoefficientType& cutCoefficients,
                         const SlopeSettings& lowCutSlope)
    {
        
        // Bypass all link in chain
        leftLowCut.template setBypassed<0>(true);
        leftLowCut.template setBypassed<1>(true);
        leftLowCut.template setBypassed<2>(true);
        leftLowCut.template setBypassed<3>(true);
        
        // Updates filter coefficients depending on which slope is selected
        switch (lowCutSlope)
        {
            // Reversing case order and removing breaks to avoid duplicate code
            case Slope_48:
            {
                update<3>(leftLowCut, cutCoefficients);
            }
            case Slope_36:
            {
                update<2>(leftLowCut, cutCoefficients);
            }
            case Slope_24:
            {
                update<1>(leftLowCut, cutCoefficients);
            }
            case Slope_12:
            {
                update<0>(leftLowCut, cutCoefficients);
            }
        }
    }
    
    // Function update LPF / HPF
    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);
    
    // Function updating all the filters
    void updateFilters();
    
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimplyQueueAudioProcessor)
};
