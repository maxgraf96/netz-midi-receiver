//
// Created by Max on 21/10/2023.
//

#ifndef NETZ_MIDI_RECEIVER_PLUGINEDITOR_H
#define NETZ_MIDI_RECEIVER_PLUGINEDITOR_H

#pragma once

#include <JuceHeader.h>
#include <readerwriterqueue.h>

#include "PluginProcessor.h"
#include "MIDIMessage.h"
#include "CustomLookAndFeel.h"

class NetzMIDIReceiverAudioProcessorEditor  :
        public juce::AudioProcessorEditor,
        public juce::Timer
{
public:
    NetzMIDIReceiverAudioProcessorEditor (NetzMIDIReceiverAudioProcessor&, moodycamel::ReaderWriterQueue<MIDIMessage>& messageQueue);
    ~NetzMIDIReceiverAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;

private:
    // Reference to our processor
    NetzMIDIReceiverAudioProcessor& processor;
    moodycamel::ReaderWriterQueue<MIDIMessage>& editorQueue;

    CustomLookAndFeel customLookAndFeel;
    Label titleLabel;
    Label connectedLabel;
    juce::TextEditor textEditor;
    juce::Viewport viewport;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NetzMIDIReceiverAudioProcessorEditor)
};

#endif //NETZ_MIDI_RECEIVER_PLUGINEDITOR_H
