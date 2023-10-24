//
// Created by Max on 22/10/2023.
//

#ifndef NETZ_MIDI_RECEIVER_MESSAGERECEIVER_H
#define NETZ_MIDI_RECEIVER_MESSAGERECEIVER_H

#include <JuceHeader.h>
#include <readerwriterqueue.h>
#include "MIDIMessage.h"

class MessageReceiver : public juce::Thread
{
public:
    MessageReceiver(int receivePort, moodycamel::ReaderWriterQueue<MIDIMessage>& q, moodycamel::ReaderWriterQueue<MIDIMessage>& editorQueue)
        : juce::Thread("Message Receiver"),
        messageQueue(q),
        editorQueue(editorQueue)
        {
        this->receivePort = receivePort;
    }

    struct DataBuffer {
        MIDIMessageType type;
        int channel;
        int note;
        int velocity;
        float pitchBend;
    };

    void run() override
    {
        receiveSocket = std::make_unique<DatagramSocket>(false);
        receiveSocket->bindToPort(receivePort);

        while (!threadShouldExit())
        {
            // Receive 4 mixed ints/floats
            DataBuffer buffer{};
            int bytesRead = receiveSocket->read(&buffer, sizeof(buffer), true);

            if (bytesRead > 0)
            {
                // Convert char array to int array
                MIDIMessageType type = buffer.type;
                int channel = buffer.channel;
                int note = buffer.note;
                int velocity = buffer.velocity;
                // Map pitch bend from our bend in semitones (-7 to 7) to JUCE's 0-16383 range
                int bend = (int) ((buffer.pitchBend + 7.0f) / 14 * 16383);

                switch(type){
                    case NOTE_ON:
                        // Note on
                        messageQueue.enqueue(MIDIMessage(type, channel, note, velocity, true));
                        editorQueue.enqueue(MIDIMessage(type, channel, note, velocity, true));
                        break;
                    case NOTE_OFF:
                        // Note off
                        messageQueue.enqueue(MIDIMessage(type, channel, note, 0, false));
                        editorQueue.enqueue(MIDIMessage(type, channel, note, 0, false));
                        break;
                    case PITCH_BEND:
                        // Pitch bend
                        messageQueue.enqueue(MIDIMessage(type, channel, bend));
                        break;
                    case HEARTBEAT:
                        // Heartbeat
                        DBG("Heartbeat received.");
                        messageQueue.enqueue(MIDIMessage(type, channel, 0));
                        editorQueue.enqueue(MIDIMessage(type, channel, 0));
                        break;
                    default:
                        break;
                }

            }
        }
    }

    void closeSocket(){
        if(receiveSocket)
            receiveSocket->shutdown();
    }

    ~MessageReceiver()
    {
        signalThreadShouldExit();
        closeSocket();
        stopThread(1000);
    }

private:
    std::unique_ptr<DatagramSocket> receiveSocket;
    moodycamel::ReaderWriterQueue<MIDIMessage>& messageQueue;
    moodycamel::ReaderWriterQueue<MIDIMessage>& editorQueue;
    int receivePort;
};

#endif //NETZ_MIDI_RECEIVER_MESSAGERECEIVER_H
