cmake_minimum_required(VERSION 3.22)
project(AudioEnergyAnalyzerEQ VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.12
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(AudioEnergyAnalyzerEQ
  COMPANY_NAME "Tossizm"
  BUNDLE_ID "jp.audioenergyanalyzer.eq"
  PLUGIN_MANUFACTURER_CODE ClAu
  PLUGIN_CODE CaEq
  PRODUCT_NAME "AudioEnergyAnalyzerEQ"
  DESCRIPTION "10-band reference-driven EQ for AudioEnergyAnalyzer"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  FORMATS VST3
  VST3_CATEGORIES Fx EQ)

juce_generate_juce_header(AudioEnergyAnalyzerEQ)
target_sources(AudioEnergyAnalyzerEQ PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginProcessor.h
  Source/PluginEditor.cpp
  Source/PluginEditor.h)
target_compile_features(AudioEnergyAnalyzerEQ PRIVATE cxx_std_17)
target_compile_definitions(AudioEnergyAnalyzerEQ PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(AudioEnergyAnalyzerEQ PRIVATE
  juce::juce_audio_utils
  juce::juce_dsp
  juce::juce_gui_extra
  PUBLIC juce::juce_recommended_config_flags
         juce::juce_recommended_lto_flags
         juce::juce_recommended_warning_flags)
