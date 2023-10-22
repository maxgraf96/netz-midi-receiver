//
// Created by Max on 21/10/2023.
//

#include "PluginProcessor.h"
#include "PluginEditor.h"

NetzMIDIReceiverAudioProcessor::NetzMIDIReceiverAudioProcessor()
        : AudioProcessor(BusesProperties()
                                 .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                 .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

NetzMIDIReceiverAudioProcessor::~NetzMIDIReceiverAudioProcessor()
{
}

const juce::String NetzMIDIReceiverAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NetzMIDIReceiverAudioProcessor::acceptsMidi() const
{
    return true;
}

bool NetzMIDIReceiverAudioProcessor::producesMidi() const
{
    return true;
}

bool NetzMIDIReceiverAudioProcessor::isMidiEffect() const
{
    return true;
}

double NetzMIDIReceiverAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NetzMIDIReceiverAudioProcessor::getNumPrograms()
{
    return 1;
}

int NetzMIDIReceiverAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NetzMIDIReceiverAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String NetzMIDIReceiverAudioProcessor::getProgramName (int index)
{
    return {};
}

void NetzMIDIReceiverAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void NetzMIDIReceiverAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Initialize your MIDI processing and audio processing objects here
}

void NetzMIDIReceiverAudioProcessor::releaseResources()
{
    // Release any resources that were allocated in prepareToPlay
}

void NetzMIDIReceiverAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Process incoming MIDI messages and generate output MIDI messages
    // Also process audio if needed
}

bool NetzMIDIReceiverAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* NetzMIDIReceiverAudioProcessor::createEditor()
{
    return new NetzMIDIReceiverAudioProcessorEditor (*this);
}

void NetzMIDIReceiverAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Store the current state of your plugin in the destData object for saving presets or sessions
}

void NetzMIDIReceiverAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore the state of your plugin from the data object when loading presets or sessions
}

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NetzMIDIReceiverAudioProcessor();
}
