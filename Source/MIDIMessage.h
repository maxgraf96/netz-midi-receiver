//
// Created by Max on 23/10/2023.
//

#ifndef NETZ_MIDI_RECEIVER_MIDIMESSAGE_H
#define NETZ_MIDI_RECEIVER_MIDIMESSAGE_H

#include <JuceHeader.h>

enum MIDIMessageType {
    NOTE_ON = 0,
    NOTE_OFF = 1,
    PITCH_BEND = 2,
    CHANNEL_PRESSURE = 3,
    UNDEFINED = 4
};

struct MIDIMessage {
    MIDIMessageType type;
    int channel;
    int note;
    int velocity;
    int pitchBend; // Or aftertouch, depending on message type

    MIDIMessage(){
        this->type = UNDEFINED;
        this->channel = 0;
        this->note = 0;
        this->velocity = 0;
    }

    MIDIMessage(MIDIMessageType type, int channel, int note, int velocity, bool isNoteOn) {
        this->type = type;
        this->channel = channel;
        this->note = note;
        this->velocity = velocity;
    }

    MIDIMessage(MIDIMessageType type, int channel, int pitchBend) {
        this->type = type;
        this->channel = channel;
        this->pitchBend = pitchBend;
    }

    String toString() const {
        if(type == NOTE_ON || type == NOTE_OFF)
            return "Type: " + String(type) + ", Channel: " + String(channel) + ", Note: " + String(note) + ", Velocity: " + String(velocity);
        else if(type == PITCH_BEND)
            return "Type: " + String(type) + ", Channel: " + String(channel) + ", Pitch Bend: " + String(pitchBend);
        else if(type == CHANNEL_PRESSURE)
            return "Type: " + String(type) + ", Channel: " + String(channel) + ", Aftertouch: " + String(pitchBend);
        else
            return "Type: " + String(type) + ", Channel: " + String(channel);
    }
};

#endif //NETZ_MIDI_RECEIVER_MIDIMESSAGE_H
