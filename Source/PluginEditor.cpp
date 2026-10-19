#include "PluginEditor.h"

AudioEnergyAnalyzerEQAudioProcessorEditor::AudioEnergyAnalyzerEQAudioProcessorEditor(AudioEnergyAnalyzerEQAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1000, 420);
    title.setText("AudioEnergyAnalyzer EQ", juce::dontSendNotification);
    title.setFont(juce::Font(24.0f, juce::Font::bold)); title.setColour(juce::Label::textColourId, juce::Colours::white); addAndMakeVisible(title);
    status.setText("Reference: " + processor.getReferenceName(), juce::dontSendNotification);
    status.setColour(juce::Label::textColourId, juce::Colours::lightblue); addAndMakeVisible(status);
    addAndMakeVisible(loadButton); loadButton.onClick = [this] { chooseJson(); };
    addAndMakeVisible(resetButton); resetButton.onClick = [this] { for (int i=0;i<10;++i) if (auto* param=processor.parameters.getParameter("band"+juce::String(i))) param->setValueNotifyingHost(param->convertTo0to1(0.0f)); };
    const juce::String names[] = {"20–60 Hz", "60–120 Hz", "120–250 Hz", "250–500 Hz", "500–1000 Hz", "1000–2000 Hz", "2000–5000 Hz", "5000–10000 Hz", "10000–16000 Hz", "16000–20000 Hz"};
    for (int i=0;i<10;++i)
    {
        labels[(size_t)i].setText(names[i], juce::dontSendNotification);
        labels[(size_t)i].setJustificationType(juce::Justification::centred); labels[(size_t)i].setColour(juce::Label::textColourId, juce::Colours::white); addAndMakeVisible(labels[(size_t)i]);
        auto& s=sliders[(size_t)i]; s.setSliderStyle(juce::Slider::LinearVertical); s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 24); s.setRange(-12.0,12.0,0.01); s.setTextValueSuffix(" dB"); addAndMakeVisible(s);
        attachments[(size_t)i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,"band"+juce::String(i),s);
    }
}
void AudioEnergyAnalyzerEQAudioProcessorEditor::chooseJson()
{
    chooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) { const auto file=fc.getResult(); if (file.existsAsFile()) { juce::String msg; const bool ok=processor.loadReferenceJson(file,msg); status.setText((ok?"OK: ":"Error: ")+msg,juce::dontSendNotification); } });
}
void AudioEnergyAnalyzerEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(1,0,102));
    g.setColour(juce::Colours::white.withAlpha(0.25f)); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(5.0f), 8.0f, 1.0f);
}
void AudioEnergyAnalyzerEQAudioProcessorEditor::resized()
{
    auto area=getLocalBounds().reduced(16); auto top=area.removeFromTop(36); title.setBounds(top.removeFromLeft(340)); resetButton.setBounds(top.removeFromRight(100)); loadButton.setBounds(top.removeFromRight(190).reduced(4,0));
    status.setBounds(area.removeFromTop(30));
    const int colW=area.getWidth()/10;
    for(int i=0;i<10;++i){ auto col=area.removeFromLeft(i==9?area.getWidth():colW); labels[(size_t)i].setBounds(col.removeFromTop(45)); sliders[(size_t)i].setBounds(col.reduced(4,0)); }
}
