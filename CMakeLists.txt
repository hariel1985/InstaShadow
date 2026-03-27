cmake_minimum_required(VERSION 3.22)
project(InstaShadow VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../JUCE ${CMAKE_CURRENT_BINARY_DIR}/JUCE)

juce_add_plugin(InstaShadow
    COMPANY_NAME "InstaShadow"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Inst
    PLUGIN_CODE Ishd
    FORMATS VST3 AU LV2
    LV2URI "https://github.com/hariel1985/InstaShadow"
    PRODUCT_NAME "InstaShadow"
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(InstaShadow)

juce_add_binary_data(InstaShadowData SOURCES
    Resources/Rajdhani-Regular.ttf
    Resources/Rajdhani-Medium.ttf
    Resources/Rajdhani-Bold.ttf
)

target_sources(InstaShadow
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/LookAndFeel.cpp
        Source/CompressorEngine.cpp
        Source/OpticalCell.cpp
        Source/VCACompressor.cpp
        Source/TransformerSaturation.cpp
        Source/OpticalPanel.cpp
        Source/DiscretePanel.cpp
        Source/TransformerPanel.cpp
        Source/OutputPanel.cpp
)

target_compile_definitions(InstaShadow
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(InstaShadow
    PRIVATE
        InstaShadowData
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
