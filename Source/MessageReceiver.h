//
// Created by Max on 22/10/2023.
//

#ifndef NETZ_MIDI_RECEIVER_MESSAGERECEIVER_H
#define NETZ_MIDI_RECEIVER_MESSAGERECEIVER_H

#include <JuceHeader.h>

class MessageReceiver : public juce::Thread
{
public:
    MessageReceiver(int receivePort) : juce::Thread("Message Receiver"), socket(nullptr) {
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
    int receivePort;
};

#endif //NETZ_MIDI_RECEIVER_MESSAGERECEIVER_H
