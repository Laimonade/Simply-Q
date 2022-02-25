/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimplyQueueAudioProcessor::SimplyQueueAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SimplyQueueAudioProcessor::~SimplyQueueAudioProcessor()
{
}

//==============================================================================
const juce::String SimplyQueueAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimplyQueueAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimplyQueueAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimplyQueueAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimplyQueueAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimplyQueueAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimplyQueueAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimplyQueueAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimplyQueueAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimplyQueueAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimplyQueueAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
   
    // Prepare filter before use by passing a ProcessSpec object to processor
    // chain which will then pass this through each filters in the chain.
    juce::dsp::ProcessSpec spec;
    
    spec.maximumBlockSize = samplesPerBlock; // Max num of samples per block
    
    spec.numChannels = 1; // Mono processing for each chain
    
    spec.sampleRate = sampleRate; // Sample rate used
    
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    // Using helper function of the struct getChainSettings
    auto chainSettings = getChainSettings(apvts);
    
    // Updsting the peak filter coefficients
    updatePeakFilter(chainSettings);


    // Creates 1 IIR filter coefficient object for every 2 orders
    // If 12db selected = order of 2 (1 coefficient), If 48db selected = order of 8 (4 coefficients = 4 filter of 12db activated)
    // dB slope choice [0,1,2,3] --> +1 * 2 --> [2, 4, 6, 8]
    auto cutCoefficients  = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, sampleRate, (chainSettings.lowCutSlope + 1) * 2);
    
    // Init low cut filter chain
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    
    // Bypass all link in chain
    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);

    switch ( chainSettings.lowCutSlope )
    {
        case Slope_12:
        {
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            break;
        }
            
        case Slope_24:
        {
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            *leftLowCut.get<1>().coefficients = *cutCoefficients[1];
            leftLowCut.setBypassed<0>(false);
            leftLowCut.setBypassed<1>(false);
            break;
        }
        case Slope_36:
        {
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            *leftLowCut.get<1>().coefficients = *cutCoefficients[1];
            *leftLowCut.get<2>().coefficients = *cutCoefficients[2];
            leftLowCut.setBypassed<0>(false);
            leftLowCut.setBypassed<1>(false);
            leftLowCut.setBypassed<2>(false);
            break;
        }
        case Slope_48:
        {
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            *leftLowCut.get<1>().coefficients = *cutCoefficients[1];
            *leftLowCut.get<2>().coefficients = *cutCoefficients[2];
            *leftLowCut.get<3>().coefficients = *cutCoefficients[3];
            leftLowCut.setBypassed<0>(false);
            leftLowCut.setBypassed<1>(false);
            leftLowCut.setBypassed<2>(false);
            leftLowCut.setBypassed<3>(false);
            break;

        }
 
    }
    
    
    // Init right low cut filter chain
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    
    // Bypass all link in chain
    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);

    switch ( chainSettings.lowCutSlope )
    {
        case Slope_12:
        {
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
            break;
        }
            
        case Slope_24:
        {
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            *rightLowCut.get<1>().coefficients = *cutCoefficients[1];
            rightLowCut.setBypassed<0>(false);
            rightLowCut.setBypassed<1>(false);
            break;
        }
        case Slope_36:
        {
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            *rightLowCut.get<1>().coefficients = *cutCoefficients[1];
            *rightLowCut.get<2>().coefficients = *cutCoefficients[2];
            rightLowCut.setBypassed<0>(false);
            rightLowCut.setBypassed<1>(false);
            rightLowCut.setBypassed<2>(false);
            break;
        }
        case Slope_48:
        {
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            *rightLowCut.get<1>().coefficients = *cutCoefficients[1];
            *rightLowCut.get<2>().coefficients = *cutCoefficients[2];
            *rightLowCut.get<3>().coefficients = *cutCoefficients[3];
            rightLowCut.setBypassed<0>(false);
            rightLowCut.setBypassed<1>(false);
            rightLowCut.setBypassed<2>(false);
            rightLowCut.setBypassed<3>(false);
            break;

        }
 
    }
    
    
}

void SimplyQueueAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimplyQueueAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimplyQueueAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // ---------------------- Updating Peak Parameters from GUI ---------------------------
    //                 Check 'PrepareToPlay' for same code with explaination
    // ------------------------------------------------------------------------------------
    
    // Using helper function of the struct getChainSettings
    auto chainSettings = getChainSettings(apvts);
    
    // Init right low cut filter chain
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
   
    auto cutCoefficients  = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, getSampleRate(), (chainSettings.lowCutSlope + 1) * 2);

    updatePeakFilter(chainSettings);

    updateCutFilter(leftLowCut, cutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, cutCoefficients, chainSettings.lowCutSlope);

    
    
    // TODO: remove code after test
    
//    // Init right low cut filter chain
//    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    
    // Bypass all link in chain
//    rightLowCut.setBypassed<0>(true);
//    rightLowCut.setBypassed<1>(true);
//    rightLowCut.setBypassed<2>(true);
//    rightLowCut.setBypassed<3>(true);
//
//    switch ( chainSettings.lowCutSlope )
//    {
//        case Slope_12:
//        {
//            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
//            rightLowCut.setBypassed<0>(false);
//            break;
//        }
//
//        case Slope_24:
//        {
//            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
//            *rightLowCut.get<1>().coefficients = *cutCoefficients[1];
//            rightLowCut.setBypassed<0>(false);
//            rightLowCut.setBypassed<1>(false);
//            break;
//        }
//        case Slope_36:
//        {
//            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
//            *rightLowCut.get<1>().coefficients = *cutCoefficients[1];
//            *rightLowCut.get<2>().coefficients = *cutCoefficients[2];
//            rightLowCut.setBypassed<0>(false);
//            rightLowCut.setBypassed<1>(false);
//            rightLowCut.setBypassed<2>(false);
//            break;
//        }
//        case Slope_48:
//        {
//            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
//            *rightLowCut.get<1>().coefficients = *cutCoefficients[1];
//            *rightLowCut.get<2>().coefficients = *cutCoefficients[2];
//            *rightLowCut.get<3>().coefficients = *cutCoefficients[3];
//            rightLowCut.setBypassed<0>(false);
//            rightLowCut.setBypassed<1>(false);
//            rightLowCut.setBypassed<2>(false);
//            rightLowCut.setBypassed<3>(false);
//            break;
//
//        }
//
//    }
    
    
    
    
    
    
    
    
    
    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    
    // ------------------------------------------------------------------------------------
    // Processor chain needs a processing context to be passed to it in order to run the audio through the links in the chain.
    // We supply this context using an audio block instance
    
    // Create an audio block from the current buffer
    juce::dsp::AudioBlock<float> block(buffer);
    
    // Get the audio blocks for each channels
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    // Wrap an audio block into a context which we can pass to filters
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    // Process current block using the filters
    leftChain.process(leftContext);
    rightChain.process(rightContext);
    
//    for (int channel = 0; channel < totalNumInputChannels; ++channel)
//    {
//
//        auto* channelData = buffer.getWritePointer (channel);
//
//        // ..do something to the data...
//    }
}

//==============================================================================
bool SimplyQueueAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimplyQueueAudioProcessor::createEditor()
{
//    return new SimplyQueueAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void SimplyQueueAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimplyQueueAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

// Getting the parameter's values using the ChainSettings structure
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    settings.lowCutFreq = apvts.getRawParameterValue("Low-Cut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("High-Cut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Q")->load();
    settings.lowCutSlope = static_cast<SlopeSettings> (apvts.getRawParameterValue("Low-Cut Slope")->load());
    settings.highCutSlope = static_cast<SlopeSettings> (apvts.getRawParameterValue("High-Cut Slope")->load());

    return settings;
}

void SimplyQueueAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings)
{
    
    // Produce coefficients from the IIR DSP class for the GUI user helper function chainSettings
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
                                                                                chainSettings.peakFreq,
                                                                                chainSettings.peakQuality,
                                                                                juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
    
    // ------------------------------------------------------------------------------------
    // Access peak filter link and assign some coefficients
    // PeakCoefficient object is a Reference-counted wrapper of an array allocated on the heap.
    // To copy its values, we need to dereference it as it's a pointer.
    // Allocation on the heap in an audio callback is BAD
    // ------------------------------------------------------------------------------------

    // Accessing each individual links in a the chain of filter.
    // Index in chain represent each filter
//    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
//    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

// Getting the coefficients from above (left/rightChain.get), so we dereference
void SimplyQueueAudioProcessor::updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
    *old = *replacements;
}


juce::AudioProcessorValueTreeState::ParameterLayout SimplyQueueAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Low-Cut Freq", "Low-Cut Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0, 0.25f), 20.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("High-Cut Freq", "High-Cut Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0, 0.25f), 20000.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq", "Peak Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0, 0.25f), 750.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain", "Peak Gain", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.5f, 0.25f), 0.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Q", "Peak Q", juce::NormalisableRange<float>(0.1f, 10.0f, 0.05f, 0.25f), 1.0f));
    
    
    juce::StringArray dbStringArray;
    for (int i = 0; i < 4; i++)
    {
        juce::String str;
        str << (12 + i*12);
        str << "db/Oct";
        dbStringArray.add(str);
    }
    
    layout.add(std::make_unique<juce::AudioParameterChoice>("Low-Cut Slope", "Low-Cut Slope", dbStringArray, 0));
    
    layout.add(std::make_unique<juce::AudioParameterChoice>("High-Cut Slope", "High-Cut Slope", dbStringArray, 0));
    
    return layout;
 
}




//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimplyQueueAudioProcessor();
}
