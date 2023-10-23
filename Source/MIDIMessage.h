//
// Created by Max on 23/10/2023.
//

#ifndef NETZ_MIDI_RECEIVER_MIDIMESSAGE_H
#define NETZ_MIDI_RECEIVER_MIDIMESSAGE_H

#include <JuceHeader.h>

struct MIDIMessage {
    int channel;
    int note;
    int velocity;
    bool isNoteOn;

    MIDIMessage(){
        this->channel = 0;
        this->note = 0;
        this->velocity = 0;
        this->isNoteOn = false;
    }

    MIDIMessage(int channel, int note, int velocity, bool isNoteOn) {
        this->channel = channel;
        this->note = note;
        this->velocity = velocity;
        this->isNoteOn = isNoteOn;
    }

    String toString() const {
        return "Channel: " + String(channel) + ", Note: " + String(note) + ", Velocity: " + String(velocity) + ", isNoteOn: " + (isNoteOn ? "true" : "false");
    }
};

#endif //NETZ_MIDI_RECEIVER_MIDIMESSAGE_H
