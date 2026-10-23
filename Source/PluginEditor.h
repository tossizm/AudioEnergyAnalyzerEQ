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
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
private:
    AudioEnergyAnalyzerEQAudioProcessor& processor;
    juce::Label title;
    juce::TextButton loadButton { "Load Analysis TXT" };
    juce::TextButton resetButton { "Reset EQ" };
    juce::Label fileStatus;
    juce::FileChooser chooser { "Select an analysis TXT file", juce::File{}, "*.txt" };
    int dragBand = -1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEnergyAnalyzerEQAudioProcessorEditor)
};
