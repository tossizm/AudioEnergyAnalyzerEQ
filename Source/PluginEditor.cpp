#include "PluginEditor.h"
#include <cmath>

ClubAudioAnalyzerEQAudioProcessorEditor::ClubAudioAnalyzerEQAudioProcessorEditor(ClubAudioAnalyzerEQAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1040, 700);
    setResizable(true, true);
    setResizeLimits(760, 560, 1920, 1200);

    title.setText("ClubAudioAnalyzer EQ", juce::dontSendNotification);
    title.setFont(juce::Font(24.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(title);

    txtFileLabel.setText("Analysis TXT: (not loaded)", juce::dontSendNotification);
    txtFileLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    txtFileLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
    addAndMakeVisible(txtFileLabel);

    referenceLabel.setText("Reference: " + processor.getReferenceName(), juce::dontSendNotification);
    referenceLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    referenceLabel.setColour(juce::Label::textColourId, juce::Colours::lime);
    addAndMakeVisible(referenceLabel);

    status.setText("Load an analysis TXT file.", juce::dontSendNotification);
    status.setFont(juce::Font(14.0f));
    status.setColour(juce::Label::textColourId, juce::Colours::lightblue);
    addAndMakeVisible(status);

    curveTitle.setText("Frequency Response", juce::dontSendNotification);
    curveTitle.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(curveTitle);

    addAndMakeVisible(loadTxtButton);
    loadTxtButton.onClick = [this] { chooseTxtFile(); };
    addAndMakeVisible(resetButton);
    resetButton.onClick = [this] {
        for (int i = 0; i < 10; ++i)
            if (auto* param = processor.parameters.getParameter("band" + juce::String(i)))
                param->setValueNotifyingHost(param->convertTo0to1(0.0f));
        updateBandText();
        repaint();
    };

    const juce::String names[] = {
        "20 ~ 60 Hz", "60 ~ 120 Hz", "120 ~ 250 Hz", "250 ~ 500 Hz",
        "500 ~ 1000 Hz", "1000 ~ 2000 Hz", "2000 ~ 5000 Hz",
        "5000 ~ 10000 Hz", "10000 ~ 16000 Hz", "16000 ~ 20000 Hz"
    };
    for (int i = 0; i < 10; ++i)
    {
        auto& bandLabel = labels[(size_t)i];
        bandLabel.setText(names[i], juce::dontSendNotification);
        bandLabel.setFont(juce::Font(13.0f, juce::Font::bold));
        bandLabel.setJustificationType(juce::Justification::centred);
        bandLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(bandLabel);

        auto& value = valueLabels[(size_t)i];
        value.setFont(juce::Font(14.0f, juce::Font::bold));
        value.setJustificationType(juce::Justification::centred);
        value.setColour(juce::Label::textColourId, juce::Colours::yellow);
        addAndMakeVisible(value);
    }
    updateBandText();
}

void ClubAudioAnalyzerEQAudioProcessorEditor::chooseTxtFile()
{
    chooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (!file.existsAsFile()) return;
            juce::String message;
            const bool ok = processor.loadAnalysisTxt(file, message);
            status.setText((ok ? "OK: " : "Error: ") + message, juce::dontSendNotification);
            if (ok)
            {
                txtFileLabel.setText(file.getFileName(), juce::dontSendNotification);
                referenceLabel.setText("Reference: " + processor.getReferenceName(), juce::dontSendNotification);
            }
            updateBandText();
            repaint();
        });
}

void ClubAudioAnalyzerEQAudioProcessorEditor::updateBandText()
{
    for (int i = 0; i < 10; ++i)
        valueLabels[(size_t)i].setText(juce::String(processor.getBandGain(i), 2) + " dB", juce::dontSendNotification);
}

void ClubAudioAnalyzerEQAudioProcessorEditor::drawResponse(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour(juce::Colour::fromRGB(13, 17, 29));
    g.fillRoundedRectangle(r, 5.0f);
    auto plot = r.reduced(42.0f, 18.0f);
    const float minDb = -18.0f, maxDb = 18.0f;
    auto yFor = [&](float db) { return juce::jmap(juce::jlimit(minDb, maxDb, db), maxDb, minDb, plot.getY(), plot.getBottom()); };
    auto xFor = [&](float hz) { return juce::jmap(std::log10(juce::jlimit(20.0f, 20000.0f, hz)), std::log10(20.0f), std::log10(20000.0f), plot.getX(), plot.getRight()); };
    g.setFont(11.0f);
    for (int db = -18; db <= 18; db += 6)
    {
        const float y = yFor((float)db);
        g.setColour(db == 0 ? juce::Colours::grey.withAlpha(0.8f) : juce::Colours::white.withAlpha(0.12f));
        g.drawHorizontalLine((int)y, plot.getX(), plot.getRight());
        g.setColour(juce::Colours::lightgrey);
        g.drawText(juce::String(db), (int)r.getX() + 4, (int)y - 7, 32, 14, juce::Justification::centredRight);
    }
    const int hzTicks[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    for (int hz : hzTicks)
    {
        const float x = xFor((float)hz);
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.drawVerticalLine((int)x, plot.getY(), plot.getBottom());
        g.setColour(juce::Colours::lightgrey);
        g.drawText(hz >= 1000 ? juce::String(hz / 1000) + "k" : juce::String(hz),
                   (int)x - 20, (int)plot.getBottom() + 4, 40, 14, juce::Justification::centred);
    }
    juce::Path path;
    for (int i = 0; i <= 420; ++i)
    {
        const float hz = (float)(20.0 * std::pow(1000.0, (double)i / 420.0));
        const float x = xFor(hz), y = yFor(processor.getResponseDb(hz));
        if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
    }
    g.setColour(juce::Colour::fromRGB(72, 205, 255));
    g.strokePath(path, juce::PathStrokeType(2.5f));
    for (int i = 0; i < 10; ++i)
    {
        const float x = xFor(ClubAudioAnalyzerEQAudioProcessor::centresHz[(size_t)i]);
        const float y = yFor(processor.getResponseDb(ClubAudioAnalyzerEQAudioProcessor::centresHz[(size_t)i]));
        g.setColour(juce::Colour::fromRGB(255, 191, 71));
        g.fillEllipse(x - 4.5f, y - 4.5f, 9.0f, 9.0f);
        g.setColour(juce::Colours::white);
        g.drawText(juce::String(i + 1), (int)x - 8, (int)y - 19, 16, 14, juce::Justification::centred);
    }
}

void ClubAudioAnalyzerEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(1, 0, 102));
    g.setColour(juce::Colours::white.withAlpha(0.22f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(5.0f), 8.0f, 1.0f);
    drawResponse(g, juce::Rectangle<float>(16.0f, 154.0f, (float)getWidth() - 32.0f, (float)getHeight() * 0.46f));
}

void ClubAudioAnalyzerEQAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(16);
    title.setBounds(area.removeFromTop(34));
    loadTxtButton.setBounds(area.removeFromTop(30).removeFromRight(190).reduced(2, 0));
    resetButton.setBounds(getWidth() - 118, 16, 96, 30);

    txtFileLabel.setBounds(16, 48, getWidth() - 32, 25);
    referenceLabel.setBounds(16, 74, getWidth() - 32, 25);
    status.setBounds(16, 102, getWidth() - 32, 22);
    curveTitle.setBounds(18, 128, 300, 20);

    const int graphBottom = (int)(154.0f + (float)getHeight() * 0.46f);
    const int bandTop = graphBottom + 12;
    const int bandHeight = juce::jmax(76, getHeight() - bandTop - 16);
    const int colW = (getWidth() - 32) / 10;
    for (int i = 0; i < 10; ++i)
    {
        const int x = 16 + i * colW;
        const int w = (i == 9) ? getWidth() - 16 - x : colW;
        labels[(size_t)i].setBounds(x + 1, bandTop, w - 2, 34);
        valueLabels[(size_t)i].setBounds(x + 1, bandTop + 34, w - 2, juce::jmin(28, bandHeight - 34));
    }
}
