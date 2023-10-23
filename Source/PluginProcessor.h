//
// Created by Max on 21/10/2023.
//

#ifndef NETZ_MIDI_RECEIVER_PLUGINPROCESSOR_H
#define NETZ_MIDI_RECEIVER_PLUGINPROCESSOR_H

#pragma once

#include <JuceHeader.h>
#include <readerwriterqueue.h>
#include "MessageReceiver.h"
#include "MIDIMessage.h"

class NetzMIDIReceiverAudioProcessor  : public juce::AudioProcessor
{
public:
    NetzMIDIReceiverAudioProcessor();
    ~NetzMIDIReceiverAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isMidiEffect() const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool getConnected() const {
        return isConnected;
    }

    void lookForBeacons();

private:
    juce::ThreadPool threadPool;

    String senderIP;
    std::atomic<bool> isConnected = false;

    int BROADCAST_PORT = 12345;
    int RECEIVE_PORT = 12346;

    std::unique_ptr<MessageReceiver> messageReceiver;

    // This one is connected to the message receiver thread
    moodycamel::ReaderWriterQueue<MIDIMessage> messageQueue;
    moodycamel::ReaderWriterQueue<MIDIMessage> editorQueue;
    moodycamel::ReaderWriterQueue<bool> connectionLostQ;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NetzMIDIReceiverAudioProcessor)
};



#endif //NETZ_MIDI_RECEIVER_PLUGINPROCESSOR_H
