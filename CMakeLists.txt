cmake_minimum_required(VERSION 3.15)

project(NetzMIDIReceiver VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(JUCE)

# Define the plugin's sources
set(SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/MessageReceiver.h
        # Add other source files here
)

# Define the plugin's headers
set(HEADERS
        Source/PluginProcessor.h
        Source/PluginEditor.h
        Source/MessageReceiver.h
        # Add other header files here
)

# Define the VST3 UID - you should change this to a unique value
set(PLUGIN_ID "netz-midi-receiver")

juce_add_plugin(NetzMIDIReceiver
        COMPANY_NAME "graf"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT TRUE
        IS_MIDI_EFFECT TRUE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
        PLUGIN_MANUFACTURER_CODE Juce
        PLUGIN_CODE Demo
        FORMATS VST3 AU Standalone # Add other formats if needed
        PRODUCT_NAME "Netz MIDI Receiver")

juce_generate_juce_header(NetzMIDIReceiver)

target_sources(NetzMIDIReceiver PRIVATE ${SOURCES} ${HEADERS})

target_compile_definitions(NetzMIDIReceiver
        PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(NetzMIDIReceiver PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra)

# Add other necessary configurations, flags, etc.
