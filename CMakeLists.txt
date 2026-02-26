cmake_minimum_required(VERSION 3.22)

project(RefBox VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../JUCE JUCE)

# AAX SDK
juce_set_aax_sdk_path(${CMAKE_CURRENT_SOURCE_DIR}/../aax-sdk-2-9-0)

juce_add_plugin(RefBox
    COMPANY_NAME "RefBoxAudio"
    BUNDLE_ID "com.refboxaudio.refbox"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Rfbx
    PLUGIN_CODE Rfb1
    FORMATS AU VST3 AAX Standalone
    AAX_CATEGORY AAX_ePlugInCategory_None
    PRODUCT_NAME "RefBox"
)

target_sources(RefBox
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/DSP/SpectralAnalyzer.cpp
        Source/DSP/SpectralAnalyzer.h
        Source/DSP/LUFSMeter.cpp
        Source/DSP/LUFSMeter.h
        Source/DSP/ReferenceCurve.cpp
        Source/DSP/ReferenceCurve.h
        Source/DSP/SystemSimulator.cpp
        Source/DSP/SystemSimulator.h
        Source/DSP/EQMatcher.cpp
        Source/DSP/EQMatcher.h
        Source/Components/SpectrumDisplay.cpp
        Source/Components/SpectrumDisplay.h
        Source/Components/ReferencePanel.cpp
        Source/Components/ReferencePanel.h
        Source/Components/ReferenceGrid.cpp
        Source/Components/ReferenceGrid.h
        Source/Components/RefBoxLookAndFeel.h
        Source/Components/SystemSimPanel.cpp
        Source/Components/SystemSimPanel.h
        Source/Components/MeterPanel.cpp
        Source/Components/MeterPanel.h
        Source/Components/TopBar.cpp
        Source/Components/TopBar.h
        Source/Components/WaveformDisplay.cpp
        Source/Components/WaveformDisplay.h
        Source/Components/WaveformMicroscope.cpp
        Source/Components/WaveformMicroscope.h
        Source/Components/ABButton.cpp
        Source/Components/ABButton.h
)

target_compile_definitions(RefBox
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(RefBox
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_audio_devices
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
