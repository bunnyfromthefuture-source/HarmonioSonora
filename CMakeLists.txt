cmake_minimum_required(VERSION 3.22)

project(HarmonioSonora VERSION 0.1.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(HarmonioSonora
    COMPANY_NAME "Harmonio"
    PLUGIN_MANUFACTURER_CODE Harm
    PLUGIN_CODE Hsno
    FORMATS VST3
    PRODUCT_NAME "Harmonio Sonora"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(HarmonioSonora PRIVATE
    PluginProcessor.cpp
    PluginProcessor.h)

target_compile_definitions(HarmonioSonora PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(HarmonioSonora
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
