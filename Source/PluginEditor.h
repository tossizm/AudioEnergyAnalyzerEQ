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
    juce::Label title,status,curveTitle;
    juce::TextButton loadJsonButton{"Load Reference JSON"},loadTxtButton{"Load Analysis TXT"},resetButton{"Reset EQ"};
    std::array<juce::Slider,ClubAudioAnalyzerEQAudioProcessor::bandCount> sliders;
    std::array<juce::Label,ClubAudioAnalyzerEQAudioProcessor::bandCount> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,ClubAudioAnalyzerEQAudioProcessor::bandCount> attachments;
    juce::FileChooser chooser{"Select a ClubAudioAnalyzer file"};
    enum class LoadMode { json, txt };
    void chooseFile(LoadMode);
    void drawResponse(juce::Graphics&,juce::Rectangle<float>);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClubAudioAnalyzerEQAudioProcessorEditor)
};
