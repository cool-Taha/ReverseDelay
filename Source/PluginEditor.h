/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Parameters.h"
#include "RotaryKnob.h"
#include "LookAndFeel.h"

//==============================================================================
/**
*/
class DelayAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                   private juce::AudioProcessorParameter::Listener
{
public:
    DelayAudioProcessorEditor (DelayAudioProcessor&);
    ~DelayAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    
    DelayAudioProcessor& audioProcessor;
    
    RotaryKnob gainKnob { "Gain", audioProcessor.apvts, gainParamID, true };
    RotaryKnob mixKnob { "Mix", audioProcessor.apvts, mixParamID };
    RotaryKnob delayTimeKnob { "Time", audioProcessor.apvts, delayTimeParamID };
    RotaryKnob feedbackKnob { "Feedback", audioProcessor.apvts, feedbackParamID };
    RotaryKnob stereoKnob { "Stereo", audioProcessor.apvts, stereoParamID, true };
    
    RotaryKnob lowCutKnob { "Low Cut", audioProcessor.apvts, lowCutParamID };
    RotaryKnob highCutKnob { "High Cut", audioProcessor.apvts, highCutParamID };
    
    RotaryKnob delayNoteKnob { "Note", audioProcessor.apvts, delayNoteParamID };
    
    juce::GroupComponent delayGroup, feedbackGroup, outputGroup;

    MainLookAndFeel mainLF;
    
    juce::TextButton reverseDelayButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reverseDelayAttachment;
    
    juce::TextButton tempoSyncButton;
    juce::AudioProcessorValueTreeState::ButtonAttachment tempoSyncAttachment
    {
        audioProcessor.apvts, tempoSyncParamID.getParamID(), tempoSyncButton
    };
    
    void parameterValueChanged(int, float) override;
    void parameterGestureChanged(int, bool) override { }
    void updateDelayKnobs(bool tempoSyncActive);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayAudioProcessorEditor)
};
