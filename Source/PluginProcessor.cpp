//
// Created by Max on 21/10/2023.
//

#include "PluginProcessor.h"
#include "PluginEditor.h"

NetzMIDIReceiverAudioProcessor::NetzMIDIReceiverAudioProcessor()
        : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
)
{
}

NetzMIDIReceiverAudioProcessor::~NetzMIDIReceiverAudioProcessor() {
    isConnected = true; // Just to shut down the thread

    if(beaconSocket)
        beaconSocket->shutdown();

    threadPool.removeAllJobs(true, 1000, nullptr);
}

const juce::String NetzMIDIReceiverAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool NetzMIDIReceiverAudioProcessor::acceptsMidi() const {
    return true;
}

bool NetzMIDIReceiverAudioProcessor::producesMidi() const {
    return true;
}

bool NetzMIDIReceiverAudioProcessor::isMidiEffect() const {
    return true;
}

double NetzMIDIReceiverAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int NetzMIDIReceiverAudioProcessor::getNumPrograms() {
    return 1;
}

int NetzMIDIReceiverAudioProcessor::getCurrentProgram() {
    return 0;
}

void NetzMIDIReceiverAudioProcessor::setCurrentProgram(int index) {
}

const juce::String NetzMIDIReceiverAudioProcessor::getProgramName(int index) {
    return {};
}

void NetzMIDIReceiverAudioProcessor::changeProgramName(int index, const juce::String &newName) {
}

void NetzMIDIReceiverAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    messageReceiver = std::make_unique<MessageReceiver>(UDP_MIDI_RECEIVE_PORT, messageQueue, editorQueue);
    connectionStatusManager = std::make_unique<ConnectionStatusManager>(TCP_CONNECTION_PORT, connectionLostQ);

    // Add a job to the thread pool using a lambda
    threadPool.addJob([&]() {
        lookForBeacons();
    });
}

void NetzMIDIReceiverAudioProcessor::releaseResources() {
    // Send the bye message to Unity if we're connected
    connectionStatusManager->goodbye();
    connectionStatusManager->cleanUp();

    isConnected = true; // Just to shut down the thread

    // Release any resources that were allocated in prepareToPlay
    threadPool.removeAllJobs(true, 1000, nullptr);

    if(beaconSocket)
        beaconSocket->shutdown();
    beaconSocket.reset();
}

void NetzMIDIReceiverAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) {
    // Process incoming MIDI messages and generate output MIDI messages
    // Look for MIDI messages in the queue
    midiMessages.clear();

    MIDIMessage message;
    while (messageQueue.try_dequeue(message)) {
        auto note = message.note;
        auto velocity = (uint8) message.velocity;
        auto channel = message.channel;

        switch (message.type) {
            case NOTE_ON:
                midiMessages.addEvent(MidiMessage::noteOn(channel, note, velocity), 0);
                break;
            case NOTE_OFF:
                midiMessages.addEvent(MidiMessage::noteOff(channel, note), 0);
                // Reset pitch bend
                midiMessages.addEvent(MidiMessage::pitchWheel(channel, 8192), 0);
                break;
            case PITCH_BEND:
                // JUCE pitch wheel is in range 0 to 16383
                midiMessages.addEvent(MidiMessage::pitchWheel(channel, message.pitchBend), 0);
                break;
            case HEARTBEAT:

                break;
            case UNDEFINED:
                break;
        }
        
    }

    // Clear audio buffer
    buffer.clear();

    bool connectionLost = false;
    while(connectionLostQ.try_dequeue(connectionLost)){
        if(connectionLost){
            DBG("Connection lost.");
            // Execute the lambda on the message thread
            juce::MessageManager::callAsync([this]() {
                threadPool.addJob([&]() {
                    lookForBeacons();
                });
            });
        }
    }
}

void NetzMIDIReceiverAudioProcessor::lookForBeacons() {
    isConnected = false;
    // This port is used to broadcast our UDP beacon, essentially telling Unity that we're here
    beaconSocket = std::make_unique<DatagramSocket>(true);
    // This socket just listens for the connection acknowledgement from Unity
    std::unique_ptr<DatagramSocket> connectionAckSocket = std::make_unique<DatagramSocket>(true);
    connectionAckSocket->bindToPort(UDP_CONNECTION_ACKNOWLEDGEMENT_PORT);

    // This string is sent to Unity as a UDP broadcast, if unity finds it, it connects to us
    String broadcastData = "netz-midi-receiver: ping.";
    // Convert JUCE string to MemoryBlock for sending
    MemoryBlock broadcastDataBlock(broadcastData.toRawUTF8(), broadcastData.getNumBytesAsUTF8());

    while (!isConnected) {
        // Send the request
        bool sendResult = beaconSocket->write("255.255.255.255", UDP_BROADCAST_PORT, broadcastDataBlock.getData(),
                                              broadcastDataBlock.getSize());

        if (!sendResult) {
            DBG("Failed to send request.");
            juce::Thread::sleep(100);
            continue;
        }

        // Buffer to hold the response from Unity
        char buffer[512];
        // These fields will be populated with the sender's IP and port
        senderIP = "";
        int senderPort;

        // Try to receive the response from Unity
        int bytesRead = connectionAckSocket->read(buffer, sizeof(buffer), false, senderIP, senderPort);

        // If we get a response from these two ports it's most likely from Unity
        if (senderPort == UDP_BROADCAST_PORT || senderPort == UDP_CONNECTION_ACKNOWLEDGEMENT_PORT) {
            // Good if we get some data back
            if (bytesRead > 0) {
                String response = String::fromUTF8(buffer, bytesRead);
                DBG("Received response " + response + " from " + senderIP);
                // Check if the response contains the string we're looking for
                if(!response.contains("NetzUnityAck")){
                    juce::Thread::sleep(100);
                    DBG("Acknowledgement correct, connecting to " + senderIP);
                    continue;
                }
                // Yay, we're connected!
                isConnected = true;
                connectionAckSocket->shutdown();

                // Start MIDI message receiver thread if it's not already running
                // (this is the case if we've lost connection and reconnected)
                if(!messageReceiver->isThreadRunning())
                    messageReceiver->startThread();
                // (Re-)start connection status manager thread
                // Always clean up the old thread first
                connectionStatusManager->cleanUp();
                connectionStatusManager->startThread();
            } else {
                DBG("Failed to receive a response.");
            }
        }

        juce::Thread::sleep(100);
    }

    if(beaconSocket){
        beaconSocket->shutdown();
    }
    beaconSocket.reset();
}

bool NetzMIDIReceiverAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *NetzMIDIReceiverAudioProcessor::createEditor() {
    return new NetzMIDIReceiverAudioProcessorEditor(*this, editorQueue);
}

void NetzMIDIReceiverAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    // Store the current state of your plugin in the destData object for saving presets or sessions
}

void NetzMIDIReceiverAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
    // Restore the state of your plugin from the data object when loading presets or sessions
}

// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new NetzMIDIReceiverAudioProcessor();
}
