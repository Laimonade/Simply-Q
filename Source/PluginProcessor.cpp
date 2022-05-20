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
    
    // Update each filters using helper function
    updateFilters();
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
    
    // Update each filters using helper function
    updateFilters();
    
    /* Processor chain needs a processing context to be passed to it in order to run the audio through the links in the chain.
    // We supply this context using an audio block instance */
    
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
}

//==============================================================================
bool SimplyQueueAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimplyQueueAudioProcessor::createEditor()
{
    return new SimplyQueueAudioProcessorEditor (*this);
//    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void SimplyQueueAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    // The 'state' member is an instance (part of) the juce audio processor value tree state (apvts)
    // We use a memory output stream to write (serialise) the apvts state to the memory block.
    
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void SimplyQueueAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    // Restore parameters from apvts using a helper function
    
    // Checking if state is valid before using it as plugin state
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if(tree.isValid())
    {
        apvts.replaceState(tree);
        updateFilters();
    }
}

// Getting the parameter's values using the ChainSettings structure
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    
    // Gets settings in the range we have defined the sliders. For normalised values, use apvts.getParameter()
    settings.lowCutFreq = apvts.getRawParameterValue("Low-Cut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("High-Cut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Q")->load();
    settings.lowCutSlope = static_cast<SlopeSettings> (apvts.getRawParameterValue("Low-Cut Slope")->load());
    settings.highCutSlope = static_cast<SlopeSettings> (apvts.getRawParameterValue("High-Cut Slope")->load());

    return settings;
}

// Getting the coefficients from above (left/rightChain.get), so we dereference
void SimplyQueueAudioProcessor::updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
    // Reference counted objects allocated on the heap, so we need to dereference them to get underlying object
    *old = *replacements;
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
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}


void SimplyQueueAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings)
{
    // ------------------------------------------------------------------------------------
    // Creates 1 IIR filter coefficient object for every 2 orders
    // If 12db selected = order of 2 (1 coefficient), If 48db selected = order of 8 (4 coefficients = 4 filter of 12db activated)
    // dB slope choice [0,1,2,3] --> +1 * 2 --> [2, 4, 6, 8]
    // ------------------------------------------------------------------------------------
    
    auto lowCutCoefficients  = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, getSampleRate(), (chainSettings.lowCutSlope + 1) * 2);
    
    // Init right low cut filter chain
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    
    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
}

void SimplyQueueAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{
    // Low pass / high cut filter
    auto highCutCoefficients  = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq, getSampleRate(), (chainSettings.highCutSlope + 1) * 2);
    
    // Init high cut filter chain
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    
    // Updating the cut filter DSP from the GUI settings
    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

// Function updating all the filters
void SimplyQueueAudioProcessor::updateFilters()
{
    // Using helper function of the struct getChainSettings
    auto chainSettings = getChainSettings(apvts);
    
    // Updating all filters from the GUI parameters
    updateLowCutFilters(chainSettings);
    updateHighCutFilters(chainSettings);
    updatePeakFilter(chainSettings);
}


juce::AudioProcessorValueTreeState::ParameterLayout SimplyQueueAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Low-Cut Freq", "Low-Cut Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0, 0.25f), 20.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("High-Cut Freq", "High-Cut Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0, 0.25f), 20000.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq", "Peak Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0, 0.25f), 750.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain", "Peak Gain", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.5f, 1.0f), 0.0f)); // Norma range 0.5 = change of half of a decibel
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Q", "Peak Q", juce::NormalisableRange<float>(0.1f, 10.0f, 0.05f, 1.0f), 1.0f));
    
    
    juce::StringArray dbStringArray;
    for (int i = 0; i < 4; i++)
    {
        juce::String str;
        str << (12 + i*12);
        str << "db/Oct";
        dbStringArray.add(str);
    }
    
    // Choice of slopes for the LPF
    layout.add(std::make_unique<juce::AudioParameterChoice>("Low-Cut Slope", "Low-Cut Slope", dbStringArray, 0));
    
    // Choice of slopes for the HPF
    layout.add(std::make_unique<juce::AudioParameterChoice>("High-Cut Slope", "High-Cut Slope", dbStringArray, 0));
    
    return layout;
}




//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimplyQueueAudioProcessor();
}
