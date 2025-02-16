/*
  ==============================================================================

    Tempo.h
    Created: 6 Feb 2025 11:13:40pm
    Author:  Taha Cheema

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
class Tempo {
public:
    void reset() noexcept;
    void update(const juce::AudioPlayHead* playhead) noexcept; 
    double getMillisecondsForNoteLength(int index) const noexcept;
    double getTempo() const noexcept 
    {
        return bpm; 
    }
private:
    double bpm = 120.0;
};
