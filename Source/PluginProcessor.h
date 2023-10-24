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
#include "ConnectionStatusManager.h"

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

    String getSenderIP() const {
        return senderIP;
    }

    void lookForBeacons();

private:
    juce::ThreadPool threadPool;

    String senderIP;
    std::atomic<bool> isConnected = false;

    std::unique_ptr<DatagramSocket> beaconSocket;

    // Port used to broadcast UDP beacon
    int UDP_BROADCAST_PORT = 50000;
    // Port used to send UDP connection acknowledgement
    int UDP_CONNECTION_ACKNOWLEDGEMENT_PORT = 50001;
    // Port used to receive UDP MIDI messages from netz
    int UDP_MIDI_RECEIVE_PORT = 50002;
    // Port used to hold TCP connection in order to keep track of connection status
    int TCP_CONNECTION_PORT = 50003;

    std::unique_ptr<MessageReceiver> messageReceiver;
    std::unique_ptr<ConnectionStatusManager> connectionStatusManager;

    // This one is connected to the message receiver thread
    moodycamel::ReaderWriterQueue<MIDIMessage> messageQueue;
    moodycamel::ReaderWriterQueue<MIDIMessage> editorQueue;
    moodycamel::ReaderWriterQueue<bool> connectionLostQ;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NetzMIDIReceiverAudioProcessor)
};



#endif //NETZ_MIDI_RECEIVER_PLUGINPROCESSOR_H
