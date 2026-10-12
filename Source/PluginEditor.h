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
    void chooseEqJson();
    void chooseReferenceJson();
    void refreshLabels();
    ClubAudioAnalyzerEQAudioProcessor& processor;
    juce::TextButton loadEqButton { "EQ設定JSONを読み込む" };
    juce::TextButton loadReferenceButton { "比率リファレンスJSONを読み込む" };
    juce::Label titleLabel, eqLabel, referenceLabel, statusLabel, ratioLabel;
    std::unique_ptr<juce::FileChooser> chooser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClubAudioAnalyzerEQAudioProcessorEditor)
};
