//
// Created by Max on 21/10/2023.
//

#ifndef NETZ_MIDI_RECEIVER_PLUGINEDITOR_H
#define NETZ_MIDI_RECEIVER_PLUGINEDITOR_H

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class NetzMIDIReceiverAudioProcessorEditor  :
        public juce::AudioProcessorEditor,
        public juce::Timer
{
public:
    NetzMIDIReceiverAudioProcessorEditor (NetzMIDIReceiverAudioProcessor&);
    ~NetzMIDIReceiverAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;



private:
    // Reference to our processor
    NetzMIDIReceiverAudioProcessor& processor;

    juce::ThreadPool threadPool;

    String senderIP;
    std::atomic<bool> isConnected = false;

    // TODO: Add any Component objects or other member variables here.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NetzMIDIReceiverAudioProcessorEditor)
};

#endif //NETZ_MIDI_RECEIVER_PLUGINEDITOR_H
