/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DelayAudioProcessor::DelayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
                        params(apvts)
#endif
{
    lowCutFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    highCutFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
}

DelayAudioProcessor::~DelayAudioProcessor()
{
}

//==============================================================================
const juce::String DelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void DelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    params.prepareToPlay(sampleRate);
    params.reset();
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = juce::uint32(samplesPerBlock);
    spec.numChannels = 2;
    
    delayLine.prepare(spec);
    
    double numSamples = Parameters::maxDelayTime / 1000.0 * sampleRate;
    int maxDelayInSamples = int(std::ceil(numSamples));
    delayLine.setMaximumDelayInSamples(maxDelayInSamples);
    delayLine.reset();
    
    feedbackL = 0.0f;
    feedbackR = 0.0f;
    
    reverseBuffer.setSize(2, maxDelayInSamples);
    reverseBuffer.clear();
    reverseBufferIndex = 0;
    
    
    lowCutFilter.prepare(spec);
    lowCutFilter.reset();
    
    highCutFilter.prepare(spec);
    highCutFilter.reset();
    
    tempo.reset();
}

void DelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
#endif

void DelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, [[maybe_unused]] juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

 
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    params.update();
    tempo.update(getPlayHead());
    float syncedTime = float(tempo.getMillisecondsForNoteLength(params.delayNote));
    if (syncedTime > Parameters::maxDelayTime)
    {
        syncedTime = Parameters::maxDelayTime;
    }
    
    reverseActive = params.reverseDelayParam->get();
    
    float sampleRate = float(getSampleRate());
    
    
    float* channelDataL = buffer.getWritePointer(0);
    float* channelDataR = buffer.getWritePointer(1);
    
    int delayBufferSize = reverseBuffer.getNumSamples();
    
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        params.smoothen();
        
        float delayTime = params.tempoSync ? syncedTime : params.delayTime;
        float delayInSamples = delayTime / 1000.0f * sampleRate;
        delayLine.setDelay(delayInSamples);
        
        lowCutFilter.setCutoffFrequency(params.lowCut);
        highCutFilter.setCutoffFrequency(params.highCut);
        
        float dryL = channelDataL[sample];
        float dryR = channelDataR[sample];
        
        float mono = (dryL + dryR) * 0.5f;
        
        if (reverseActive)
        {
            
            if (!prevReverseActive)
            {
                // Set the reverse read pointer to "delayInSamples" behind the current write pointer.
                reverseBlockSampleCount = 0;
                reverseReadPointer = (reverseBufferIndex + delayBufferSize - static_cast<int>(delayInSamples)) % delayBufferSize;
                prevReverseActive = true;
            }
            reverseBuffer.setSample(0, reverseBufferIndex, mono*params.panL + feedbackR);
            reverseBuffer.setSample(1, reverseBufferIndex, mono*params.panR + feedbackL);
            
            reverseBufferIndex = (reverseBufferIndex + 1) % delayBufferSize;
            
            
            int currentReverseIndex = (reverseBlockStart + static_cast<int>(delayInSamples) - 1 - reverseBlockSampleCount) % delayBufferSize;
            
                    
            float wetL = reverseBuffer.getSample(0, currentReverseIndex);
            float wetR = reverseBuffer.getSample(1, currentReverseIndex);
            
            reverseBlockSampleCount++;
            
            if (reverseBlockSampleCount >= static_cast<int>(delayInSamples))
            {
                reverseBlockSampleCount = 0;
                reverseBlockStart = (reverseBufferIndex + delayBufferSize - static_cast<int>(delayInSamples)) % delayBufferSize;
            }
            
            feedbackL = wetL * params.feedback;
            feedbackL = lowCutFilter.processSample(0, feedbackL);
            feedbackL = highCutFilter.processSample(0, feedbackL);
            
            feedbackR = wetR * params.feedback;
            feedbackR = lowCutFilter.processSample(1, feedbackR);
            feedbackR = highCutFilter.processSample(1, feedbackR);
            
            float mixL = dryL * (1.0f - params.mix) + wetL * params.mix;
            float mixR = dryR * (1.0f - params.mix) + wetR * params.mix;
            
            channelDataL[sample] = mixL * params.gain;
            channelDataR[sample] = mixR * params.gain;
            
            // channelDataL[sample] = dryL * (1.0f - params.mix) + wetL * params.mix;
            // channelDataR[sample] = dryR * (1.0f - params.mix) + wetR * params.mix;
              
            
        }
        else
        {
            prevReverseActive = false;
            
            delayLine.pushSample(0, mono*params.panL + feedbackR);
            delayLine.pushSample(1, mono*params.panR + feedbackL);
            
            float wetL = delayLine.popSample(0);
            float wetR = delayLine.popSample(1);
            
            feedbackL = wetL * params.feedback;
            feedbackL = lowCutFilter.processSample(0, feedbackL);
            feedbackL = highCutFilter.processSample(0, feedbackL);
            
            feedbackR = wetR * params.feedback;
            feedbackR = lowCutFilter.processSample(1, feedbackR);
            feedbackR = highCutFilter.processSample(1, feedbackR);
            
            float mixL = dryL * (1.0f - params.mix) + wetL * params.mix;
            float mixR = dryL * (1.0f - params.mix) + wetR * params.mix;
            
            channelDataL[sample] = mixL * params.gain;
            channelDataR[sample] = mixR * params.gain;
        }
       
    }
}

//==============================================================================
bool DelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DelayAudioProcessor::createEditor()
{
    return new DelayAudioProcessorEditor (*this);
}

//==============================================================================
void DelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void DelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayAudioProcessor();
}


