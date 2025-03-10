/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DelayAudioProcessorEditor::DelayAudioProcessorEditor (DelayAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    
    delayGroup.setText("Delay");
    delayGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    delayGroup.addAndMakeVisible(delayTimeKnob);
    reverseDelayButton.setButtonText("Reverse");
    reverseDelayButton.setClickingTogglesState(true);
    reverseDelayButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::red);
    delayGroup.addAndMakeVisible(reverseDelayButton);
    delayGroup.addChildComponent(delayNoteKnob);
    
    tempoSyncButton.setButtonText("Sync");
    tempoSyncButton.setClickingTogglesState(true);
    tempoSyncButton.setBounds(0, 0, 70, 27);
    tempoSyncButton.setLookAndFeel(ButtonLookAndFeel::get());
    delayGroup.addAndMakeVisible(tempoSyncButton);
    addAndMakeVisible(delayGroup);
    
    feedbackGroup.setText("Feedback");
    feedbackGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    feedbackGroup.addAndMakeVisible(stereoKnob);
    feedbackGroup.addAndMakeVisible(feedbackKnob);
    feedbackGroup.addAndMakeVisible(lowCutKnob);
    feedbackGroup.addAndMakeVisible(highCutKnob);
    addAndMakeVisible(feedbackGroup);
    
    reverseDelayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.apvts, "reverseDelay", reverseDelayButton);
    outputGroup.setText("Output");
    outputGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    outputGroup.addAndMakeVisible(gainKnob);
    outputGroup.addAndMakeVisible(mixKnob);
    addAndMakeVisible(outputGroup);
    
    // Einem Knob eine eigene Farbe zuweisen.
    // gainKnob.slider.setColour(juce::Slider::rotarySliderFillColourId,
    //                           juce::Colours::blue);
    
    setLookAndFeel(&mainLF);
    
    setSize (500, 330);
    
    updateDelayKnobs(audioProcessor.params.tempoSyncParam->get());
    audioProcessor.params.tempoSyncParam->addListener(this);
}

DelayAudioProcessorEditor::~DelayAudioProcessorEditor()
{
    audioProcessor.params.tempoSyncParam->removeListener(this);
    setLookAndFeel(nullptr);
}

//==============================================================================
void DelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    
    g.fillAll (Colors::background);
    
    auto rect = getLocalBounds().withHeight(40);
    g.setColour(Colors::header);
    g.fillRect(rect);
    
    auto image = juce::ImageCache::getFromMemory(BinaryData::logo_png,
                                                 BinaryData::logo_pngSize);
    
    int destWidth = image.getWidth() / 2;
    int destHeight = image.getHeight() / 2;
    g.drawImage(image,
                getWidth() / 2 - destWidth / 2, 3, destWidth, destHeight,
                0, 0, image.getWidth(), image.getHeight());
    
}

void DelayAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    int y = 50;
    int height = bounds.getHeight() - 60;
    
    delayGroup.setBounds(10, y, 110, height);
    
    outputGroup.setBounds(bounds.getWidth() - 160, y, 150, height);
    
    feedbackGroup.setBounds(delayGroup.getRight() + 10, y,
                            outputGroup.getX() - delayGroup.getRight() - 20,
                            height);
    
    delayTimeKnob.setTopLeftPosition(20, 20);
    mixKnob.setTopLeftPosition(20, 20);
    gainKnob.setTopLeftPosition(mixKnob.getX(), mixKnob.getBottom() + 10);
    
    feedbackKnob.setTopLeftPosition(20, 20);
    stereoKnob.setTopLeftPosition(feedbackKnob.getRight() + 20, 20);
    
    lowCutKnob.setTopLeftPosition(feedbackKnob.getX(), feedbackKnob.getBottom() + 10);
    highCutKnob.setTopLeftPosition(lowCutKnob.getRight() + 20, lowCutKnob.getY());
    
    delayTimeKnob.setTopLeftPosition(20, 20);
    tempoSyncButton.setTopLeftPosition(20, delayTimeKnob.getBottom() + 10);
    delayNoteKnob.setTopLeftPosition(delayTimeKnob.getX(), delayTimeKnob.getY());
    
    reverseDelayButton.setBounds(30, 230, 70, 30);
}

void DelayAudioProcessorEditor::parameterValueChanged(int, float value)
{
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        updateDelayKnobs(value != 0.0f);
    }
    else
    {
        juce::MessageManager::callAsync([this, value]
        {
            updateDelayKnobs(value != 0.0f);
        });
    }
}

void DelayAudioProcessorEditor::updateDelayKnobs(bool tempoSyncActive)
{
    delayTimeKnob.setVisible(!tempoSyncActive);
    delayNoteKnob.setVisible(tempoSyncActive);
}
