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
        : juce::Thread("Message Receiver"), socket(nullptr), messageQueue(q), editorQueue(editorQueue) {
        this->receivePort = receivePort;
    }

    void run() override
    {
        socket = std::make_unique<StreamingSocket>();

        if (socket->createListener(receivePort, "0.0.0.0"))  // Bind to receivePort on any interface
        {
            std::unique_ptr<StreamingSocket> clientSocket;

            while (!threadShouldExit())
            {
                clientSocket.reset(socket->waitForNextConnection());

                while (clientSocket)
                {
                    int buffer[3]; // Receive 3 ints -> 3 * 4 bytes
                    int bytesRead = clientSocket->read(buffer, sizeof(buffer), true);

                    if (bytesRead > 0)
                    {
                        if (JUCE_LITTLE_ENDIAN)
                        {
                            for (int & i : buffer)
                            {
                                i = ByteOrder::swap(i);
                            }
                        }
                        // Convert char array to int array
                        int note = buffer[0];
                        int channel = buffer[1];
                        int velocity = buffer[2];

                        if(note < 0 || note > 128)
                            continue;

                        if(velocity > 0){
                            // Note on
                            messageQueue.enqueue(MIDIMessage(channel, note, velocity, true));
                            editorQueue.enqueue(MIDIMessage(channel, note, velocity, true));
                        } else {
                            // Note off
                            messageQueue.enqueue(MIDIMessage(channel, note, 0, false));
                            editorQueue.enqueue(MIDIMessage(channel, note, 0, false));
                        }
                    }
                }
            }
        }
    }

    ~MessageReceiver()
    {
        signalThreadShouldExit();
        stopThread(1000); // Wait for the thread to stop.
        waitForThreadToExit(1000);
    }

private:
    std::unique_ptr<StreamingSocket> socket;
    moodycamel::ReaderWriterQueue<MIDIMessage>& messageQueue;
    moodycamel::ReaderWriterQueue<MIDIMessage>& editorQueue;
    int receivePort;
};

#endif //NETZ_MIDI_RECEIVER_MESSAGERECEIVER_H
