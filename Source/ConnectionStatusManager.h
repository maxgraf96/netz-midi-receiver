//
// Created by Max on 24/10/2023.
//

#ifndef NETZ_MIDI_RECEIVER_CONNECTIONSTATUSMANAGER_H
#define NETZ_MIDI_RECEIVER_CONNECTIONSTATUSMANAGER_H

#include <JuceHeader.h>
#include <readerwriterqueue.h>
#include "MIDIMessage.h"

class ConnectionStatusManager : public juce::Thread
{
public:
    ConnectionStatusManager(int receivePort, moodycamel::ReaderWriterQueue<bool>& connectionLostQ)
            : juce::Thread("Message Receiver"),
              socket(nullptr),
              connectionLostQ(connectionLostQ)
    {
        this->tcpPort = receivePort;
    }

    void run() override
    {
        connectionLost = false;
        socket = std::make_unique<StreamingSocket>();

        // Bind to receivePort on any interface
        if (socket->createListener(tcpPort, "0.0.0.0"))
        {
            while (!connectionLost && !threadShouldExit())
            {
                clientSocket.reset(socket->waitForNextConnection());

                while (clientSocket)
                {
                    char buffer[64];
                    int bytesRead = clientSocket->read(&buffer, sizeof(buffer), true);

                    if (bytesRead > 0)
                    {
                        // Don't need anything here, just wait for the socket to close
                    } else if(bytesRead < 0){
                        // If this thread is being destroyed, we can't do anything with the queue anymore
                        if(closedFromOutside)
                            break;

                        // Socket closed, wait for another connection
                        DBG("Connection closed.");
                        connectionLostQ.enqueue(true);
                        connectionLost = true;
                        break;
                    }

                    juce::Thread::sleep(999);
                }
            }
        }
        cleanUp();
    }

    void goodbye(){
        if(clientSocket)
            clientSocket->write("close", 5);
    }

    void cleanUp(){
        connectionLost = true;
        signalThreadShouldExit();
        // Close socket
        if(socket)
            socket->close();
        socket.reset();
    }

    ~ConnectionStatusManager()
    {
        closedFromOutside = true;
        // If we have a valid clientSocket, send a message to close the connection
        if(clientSocket){
            clientSocket->write("close", 5);
            clientSocket->close();
        }
        stopThread(1000);
    }

    // Flag for when we destroy this from outside, happens e.g. when deleting the plugin from the DAW
    // In this case, we can't do anything with the queue anymore since it might have been deleted already
    bool closedFromOutside = false;

    bool connectionLost = false;
private:
    std::unique_ptr<StreamingSocket> socket;
    std::unique_ptr<StreamingSocket> clientSocket;

    moodycamel::ReaderWriterQueue<bool>& connectionLostQ;
    int tcpPort;
};

#endif //NETZ_MIDI_RECEIVER_CONNECTIONSTATUSMANAGER_H
