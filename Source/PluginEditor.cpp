#include "PluginEditor.h"

ClubAudioAnalyzerEQAudioProcessorEditor::ClubAudioAnalyzerEQAudioProcessorEditor(ClubAudioAnalyzerEQAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    titleLabel.setText("ClubAudioAnalyzer EQ", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);
    for (auto* label : { &eqLabel, &referenceLabel, &statusLabel, &ratioLabel })
    {
        label->setJustificationType(juce::Justification::topLeft);
        label->setColour(juce::Label::textColourId, juce::Colours::white);
        label->setMinimumHorizontalScale(0.7f);
        addAndMakeVisible(label);
    }
    loadEqButton.onClick = [this] { chooseEqJson(); };
    loadReferenceButton.onClick = [this] { chooseReferenceJson(); };
    loadEqButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff72c6ef));
    loadEqButton.setColour(juce::TextButton::textColourOffId, juce::Colours::blue);
    loadReferenceButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff72c6ef));
    loadReferenceButton.setColour(juce::TextButton::textColourOffId, juce::Colours::blue);
    addAndMakeVisible(loadEqButton);
    addAndMakeVisible(loadReferenceButton);
    setSize(660, 360);
    refreshLabels();
}

void ClubAudioAnalyzerEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff010066));
    g.setColour(juce::Colours::white.withAlpha(0.25f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 8.0f, 1.0f);
}

void ClubAudioAnalyzerEQAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    titleLabel.setBounds(area.removeFromTop(36));
    area.removeFromTop(8);
    loadEqButton.setBounds(area.removeFromTop(34).removeFromLeft(280));
    area.removeFromTop(8);
    eqLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);
    loadReferenceButton.setBounds(area.removeFromTop(34).removeFromLeft(280));
    area.removeFromTop(8);
    referenceLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);
    statusLabel.setBounds(area.removeFromTop(40));
    ratioLabel.setBounds(area);
}

void ClubAudioAnalyzerEQAudioProcessorEditor::chooseEqJson()
{
    chooser = std::make_unique<juce::FileChooser>("EQ設定JSONを選択", juce::File{}, "*.json");
    juce::Component::SafePointer<ClubAudioAnalyzerEQAudioProcessorEditor> safeThis(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis](const juce::FileChooser& fc)
        {
            if (safeThis == nullptr) return;
            const auto file = fc.getResult();
            if (file == juce::File{}) return;
            juce::String error;
            if (! safeThis->processor.loadEqSettings(file, error))
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "EQ JSON読込エラー", error);
            safeThis->refreshLabels();
        });
}

void ClubAudioAnalyzerEQAudioProcessorEditor::chooseReferenceJson()
{
    chooser = std::make_unique<juce::FileChooser>("比率リファレンスJSONを選択", juce::File{}, "*.json");
    juce::Component::SafePointer<ClubAudioAnalyzerEQAudioProcessorEditor> safeThis(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis](const juce::FileChooser& fc)
        {
            if (safeThis == nullptr) return;
            const auto file = fc.getResult();
            if (file == juce::File{}) return;
            juce::String error;
            if (! safeThis->processor.loadRatioReference(file, error))
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "比率リファレンス読込エラー", error);
            safeThis->refreshLabels();
        });
}

void ClubAudioAnalyzerEQAudioProcessorEditor::refreshLabels()
{
    eqLabel.setText("EQ設定: " + processor.getEqFileName(), juce::dontSendNotification);
    referenceLabel.setText("比率リファレンス: " + processor.getReferenceFileName(), juce::dontSendNotification);
    statusLabel.setText(processor.getStatusText(), juce::dontSendNotification);
    ratioLabel.setText(processor.getRatioSummary(), juce::dontSendNotification);
    repaint();
}
