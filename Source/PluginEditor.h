#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class AudioEnergyAnalyzerEQAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AudioEnergyAnalyzerEQAudioProcessorEditor(AudioEnergyAnalyzerEQAudioProcessor&);
    ~AudioEnergyAnalyzerEQAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    AudioEnergyAnalyzerEQAudioProcessor& processor;
    juce::Label title, status;
    juce::TextButton loadButton { "Load Reference JSON" }, resetButton { "Reset EQ" };
    std::array<juce::Slider, AudioEnergyAnalyzerEQAudioProcessor::bandCount> sliders;
    std::array<juce::Label, AudioEnergyAnalyzerEQAudioProcessor::bandCount> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, AudioEnergyAnalyzerEQAudioProcessor::bandCount> attachments;
    juce::FileChooser chooser { "Select AudioEnergyAnalyzer reference JSON" };
    void chooseJson();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEnergyAnalyzerEQAudioProcessorEditor)
};
