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
    juce::Label title, txtFileLabel, referenceLabel, status, curveTitle;
    juce::TextButton loadTxtButton{"Load Analysis TXT"}, resetButton{"Reset EQ"};
    std::array<juce::Label,ClubAudioAnalyzerEQAudioProcessor::bandCount> labels, valueLabels;
    juce::FileChooser chooser{"Select a ClubAudioAnalyzer analysis TXT"};
    void chooseTxtFile();
    void updateBandText();
    void drawResponse(juce::Graphics&,juce::Rectangle<float>);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClubAudioAnalyzerEQAudioProcessorEditor)
};
