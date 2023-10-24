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
    messageReceiver = std::make_unique<MessageReceiver>(12347, messageQueue, editorQueue, connectionLostQ);

    // Add a job to the thread pool using a lambda
    threadPool.addJob([&]() {
        lookForBeacons();
    });
}

void NetzMIDIReceiverAudioProcessor::releaseResources() {
    // Release any resources that were allocated in prepareToPlay
    threadPool.removeAllJobs(true, 1000, nullptr);
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
    std::unique_ptr<DatagramSocket> socket = std::make_unique<DatagramSocket>(true);
    std::unique_ptr<DatagramSocket> receiveSocket = std::make_unique<DatagramSocket>(true);
    receiveSocket->bindToPort(RECEIVE_PORT);

    String broadcastData = "netz-midi-receiver: ping.";

    // Convert JUCE string to MemoryBlock for sending
    MemoryBlock broadcastDataBlock(broadcastData.toRawUTF8(), broadcastData.getNumBytesAsUTF8());

    while (!isConnected) {
        // Send the request
        bool sendResult = socket->write("255.255.255.255", BROADCAST_PORT, broadcastDataBlock.getData(),
                                        broadcastDataBlock.getSize());

        if (!sendResult) {
            DBG("Failed to send request.");
            return;
        }

        char buffer[512]; // Assuming 512 bytes is enough for the response; adjust as needed
        int senderPort;

        // Receive the server response
        int bytesRead = receiveSocket->read(buffer, sizeof(buffer), false, senderIP, senderPort);

        if (senderPort == RECEIVE_PORT || senderPort == BROADCAST_PORT) {
            if (bytesRead > 0) {
                String serverResponse = String::fromUTF8(buffer, bytesRead);
                DBG("Received " + serverResponse + " from " + senderIP);
                isConnected = true;
                receiveSocket->shutdown();

                if(!messageReceiver->isThreadRunning())
                    messageReceiver->startThread();
            } else {
                DBG("Failed to receive a response.");
            }
        }

        juce::Thread::sleep(100);
    }
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
