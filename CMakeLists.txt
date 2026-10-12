cmake_minimum_required(VERSION 3.22)
project(ClubAudioAnalyzerEQ VERSION 1.0.0)

if(NOT DEFINED JUCE_SOURCE_DIR)
  include(FetchContent)
  FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(juce)
else()
  add_subdirectory("${JUCE_SOURCE_DIR}" JUCE)
endif()

juce_add_plugin(ClubAudioAnalyzerEQ
  COMPANY_NAME "ClubAudioAnalyzer"
  BUNDLE_ID "jp.clubaudioanalyzer.eq"
  PLUGIN_MANUFACTURER_CODE ClAu
  PLUGIN_CODE CaEQ
  PRODUCT_NAME "ClubAudioAnalyzer EQ"
  DESCRIPTION "JSON-linked 10-band RBJ Bell EQ"
  FORMATS VST3
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Fx EQ)

target_sources(ClubAudioAnalyzerEQ PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginProcessor.h
  Source/PluginEditor.cpp
  Source/PluginEditor.h)

target_compile_features(ClubAudioAnalyzerEQ PRIVATE cxx_std_17)
target_compile_definitions(ClubAudioAnalyzerEQ PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(ClubAudioAnalyzerEQ PRIVATE
  juce::juce_audio_utils
  juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags
         juce::juce_recommended_lto_flags
         juce::juce_recommended_warning_flags)
