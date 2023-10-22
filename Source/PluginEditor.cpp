//
// Created by Max on 21/10/2023.
//

#include "PluginEditor.h"

#include "PluginProcessor.h"
#include "PluginEditor.h"

NetzMIDIReceiverAudioProcessorEditor::NetzMIDIReceiverAudioProcessorEditor (NetzMIDIReceiverAudioProcessor& p)
        : AudioProcessorEditor (&p), processor (p)
{
    // Set the size of the editor window.
    setSize (400, 300);
    startTimer(2000);

    // Add a job to the thread pool using a lambda
    threadPool.addJob([&]()
    {
        std::unique_ptr<DatagramSocket> socket = std::make_unique<DatagramSocket>(true);
        std::unique_ptr<DatagramSocket> receiveSocket = std::make_unique<DatagramSocket>(true);
        receiveSocket->bindToPort(12346);

        String requestData = "SomeRequestData";

        // Convert JUCE string to MemoryBlock for sending
        MemoryBlock requestDataBlock(requestData.toRawUTF8(), requestData.getNumBytesAsUTF8());

        while(!isConnected){
            // Send the request
            bool sendResult = socket->write("255.255.255.255", 12345, requestDataBlock.getData(), requestDataBlock.getSize());

            if (!sendResult)
            {
                DBG("Failed to send request.");
                return 1; // Exit with an error code
            }

            char buffer[512]; // Assuming 512 bytes is enough for the response; adjust as needed
            int senderPort;

            // Receive the server response
            int bytesRead = receiveSocket->read(buffer, sizeof(buffer), true);

            if (bytesRead > 0)
            {
                String serverResponse = String::fromUTF8(buffer, bytesRead);
                DBG("Received " + serverResponse + " from " + senderIP);
                isConnected = true;
            }
            else
            {
                DBG("Failed to receive a response.");
            }

            juce::Thread::sleep(100);
        }
    });
}

NetzMIDIReceiverAudioProcessorEditor::~NetzMIDIReceiverAudioProcessorEditor()
{
    // Destructor. Any cleanup code can be added here.
    threadPool.removeAllJobs(true, 100, nullptr);
}

void NetzMIDIReceiverAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Clear the background
    if(!isConnected)
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    else
        g.fillAll (juce::Colours::darkgreen);

    // You can add additional drawing code here.
    g.setFont (juce::Font (16.0f));
    g.setColour (juce::Colours::white);

    // Get position to draw randomly on screen
    int x = Random::getSystemRandom().nextInt(getWidth()) - 100;
    int y = Random::getSystemRandom().nextInt(getHeight()) - 100;
    // Create a rectangle to draw text in
    Rectangle<int> textArea(x, y, 300, 100);

    String connectedText = "Connected to " + senderIP;
    g.drawText (isConnected ? "Connected to " + senderIP + "!" : "Not connected.", textArea, juce::Justification::centred, true);
}

void NetzMIDIReceiverAudioProcessorEditor::resized()
{
    // This is called when the NetzMIDIReceiverAudioProcessorEditor is resized.
    // You can adjust the positions and sizes of any child components here.
}

void NetzMIDIReceiverAudioProcessorEditor::timerCallback()
{
    repaint();
}
