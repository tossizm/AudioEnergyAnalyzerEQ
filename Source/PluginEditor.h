#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ClubAudioAnalyzerEQAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ClubAudioAnalyzerEQAudioProcessorEditor(ClubAudioAnalyzerEQAudioProcessor&);
    ~ClubAudioAnalyzerEQAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    ClubAudioAnalyzerEQAudioProcessor& processor;
    juce::Label title, status;
    juce::TextButton loadButton { "Load Reference JSON" }, resetButton { "Reset EQ" };
    std::array<juce::Slider, ClubAudioAnalyzerEQAudioProcessor::bandCount> sliders;
    std::array<juce::Label, ClubAudioAnalyzerEQAudioProcessor::bandCount> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, ClubAudioAnalyzerEQAudioProcessor::bandCount> attachments;
    juce::FileChooser chooser { "Select ClubAudioAnalyzer reference JSON" };
    void chooseJson();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClubAudioAnalyzerEQAudioProcessorEditor)
};
