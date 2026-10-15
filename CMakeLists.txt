cmake_minimum_required(VERSION 3.24)

project(HarmonioSonora VERSION 0.2.0)

include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)

# Signalsmith Stretch (MIT licence): high-quality pitch shifting.
# It needs Signalsmith Linear (FFT routines); the version below is the one
# the Stretch repository itself pins.
FetchContent_Declare(signalsmith_linear
    GIT_REPOSITORY https://github.com/Signalsmith-Audio/linear.git
    GIT_TAG 0.6.4
    GIT_SHALLOW TRUE)

# SOURCE_SUBDIR points at a folder that does not exist, so only the files are
# downloaded and the library's own CMake file is not run (we use its header directly).
FetchContent_Declare(signalsmith_stretch
    GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-stretch.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR no_cmake_here)

FetchContent_MakeAvailable(JUCE signalsmith_linear signalsmith_stretch)

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

target_include_directories(HarmonioSonora PRIVATE ${signalsmith_stretch_SOURCE_DIR})

target_compile_definitions(HarmonioSonora PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    _USE_MATH_DEFINES)

target_link_libraries(HarmonioSonora
    PRIVATE
        juce::juce_audio_utils
        signalsmith-linear
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
