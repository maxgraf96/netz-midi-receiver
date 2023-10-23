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
                    char buffer[16];
                    int bytesRead = clientSocket->read(buffer, sizeof(buffer), true);

                    if (bytesRead > 0)
                    {
                        String message(buffer, bytesRead);
                        DBG("TCP: Received: " << message);

                        // Get first character of message as integer
                        int type = message.substring(0, 1).getIntValue();
                        int note, velocity;
                        // TODO Update
                        int channel = 1;
                        switch (type) {
                            case 1:
                                // Note on
                                note = message.substring(1, 2).getIntValue();
                                velocity = message.substring(2, 3).getIntValue();
                                messageQueue.enqueue(MIDIMessage(channel, note, velocity, true));
                                editorQueue.enqueue(MIDIMessage(channel, note, velocity, true));
                                break;
                            case 2:
                                // Note off
                                note = message.substring(1, 2).getIntValue();
                                messageQueue.enqueue(MIDIMessage(channel, note, 0, false));
                                editorQueue.enqueue(MIDIMessage(channel, note, 0, false));
                                break;
                            default:
                                break;
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
