cmake_minimum_required(VERSION 3.22)
project(Amolizer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Downloads JUCE automatically on first configure
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Amolizer
    COMPANY_NAME "Amolizer Audio"
    PLUGIN_MANUFACTURER_CODE Amla
    PLUGIN_CODE Amlz
    FORMATS VST3 Standalone
    PRODUCT_NAME "Amolizer"
    BUNDLE_ID com.amolizer.amolizer
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Fx Dynamics EQ
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_add_binary_data(AmolizerAssets SOURCES
    Resources/background.jpg
    Resources/tongue.png
)

target_sources(Amolizer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(Amolizer PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Amolizer
    PRIVATE
        AmolizerAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
