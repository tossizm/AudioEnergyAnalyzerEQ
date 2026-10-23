#include "PluginEditor.h"
#include <cmath>

namespace
{
constexpr float minFrequency = 20.0f;
constexpr float maxFrequency = 20000.0f;
constexpr float minGain = -18.0f;
constexpr float maxGain = 18.0f;
const char* bandNames[] = { "20 ~ 60 Hz", "60 ~ 120 Hz", "120 ~ 250 Hz", "250 ~ 500 Hz",
    "500 ~ 1000 Hz", "1000 ~ 2000 Hz", "2000 ~ 5000 Hz", "5000 ~ 10000 Hz",
    "10000 ~ 16000 Hz", "16000 ~ 20000 Hz" };
}

AudioEnergyAnalyzerEQAudioProcessorEditor::AudioEnergyAnalyzerEQAudioProcessorEditor(AudioEnergyAnalyzerEQAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(780, 525);
    setResizable(true, true);
    setResizeLimits(780, 525, 2000, 1400);

    title.setText("AudioEnergyAnalyzer EQ", juce::dontSendNotification);
    title.setFont(juce::Font(24.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(title);

    addAndMakeVisible(loadButton);
    addAndMakeVisible(resetButton);
    fileStatus.setText("Analysis TXT: (not loaded)", juce::dontSendNotification);
    fileStatus.setFont(juce::Font(15.0f));
    fileStatus.setColour(juce::Label::textColourId, juce::Colours::yellow);
    addAndMakeVisible(fileStatus);

    loadButton.onClick = [this]
    {
        chooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                const auto file = fc.getResult();
                if (file.existsAsFile())
                {
                    juce::String message;
                    const bool loaded = processor.loadAnalysisTxt(file, message);
                    fileStatus.setText(message, juce::dontSendNotification);
                    fileStatus.setColour(juce::Label::textColourId, loaded ? juce::Colours::yellow : juce::Colours::red);
                    repaint();
                }
            });
    };
    resetButton.onClick = [this]
    {
        for (int i = 0; i < AudioEnergyAnalyzerEQAudioProcessor::bandCount; ++i)
            if (auto* param = processor.parameters.getParameter("band" + juce::String(i)))
                param->setValueNotifyingHost(param->convertTo0to1(0.0f));
        repaint();
    };
}

void AudioEnergyAnalyzerEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(1, 0, 102));
    g.setColour(juce::Colours::white.withAlpha(0.25f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(3.0f), 6.0f, 1.0f);

    auto bounds = getLocalBounds().reduced(8);
    bounds.removeFromTop(62);
    auto plot = bounds.removeFromTop(juce::jmax(90, bounds.getHeight() - 78)).reduced(0, 1);
    g.setColour(juce::Colour::fromRGB(13, 17, 29));
    g.fillRoundedRectangle(plot.toFloat(), 5.0f);

    const auto graph = plot.toFloat().reduced(30.0f, 12.0f);
    auto xForFrequency = [&graph](float hz)
    {
        const float t = std::log(hz / minFrequency) / std::log(maxFrequency / minFrequency);
        return graph.getX() + t * graph.getWidth();
    };
    auto yForGain = [&graph](float db)
    {
        return graph.getBottom() - (db - minGain) / (maxGain - minGain) * graph.getHeight();
    };

    g.setFont(13.5f);
    for (int db = -18; db <= 18; db += 6)
    {
        const float y = yForGain((float)db);
        g.setColour(db == 0 ? juce::Colours::white.withAlpha(0.35f) : juce::Colours::white.withAlpha(0.12f));
        g.drawHorizontalLine((int)y, graph.getX(), graph.getRight());
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.drawText(juce::String(db), (int)plot.getX(), (int)y - 9, 38, 18, juce::Justification::centredRight);
    }

    const float ticks[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    for (float hz : ticks)
    {
        const float x = xForFrequency(hz);
        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawVerticalLine((int)x, graph.getY(), graph.getBottom());
        juce::String label = hz >= 1000.0f ? juce::String(hz / 1000.0f, hz >= 10000.0f ? 0 : 1) + "k" : juce::String((int)hz);
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawText(label, (int)x - 18, (int)graph.getBottom() + 1, 54, 17, juce::Justification::centred);
    }

    auto responseAt = [this](float frequency)
    {
        double total = 0.0;
        for (int i = 0; i < AudioEnergyAnalyzerEQAudioProcessor::bandCount; ++i)
        {
            const auto* raw = processor.parameters.getRawParameterValue("band" + juce::String(i));
            const double gain = raw != nullptr ? raw->load() : 0.0;
            const double ratio = frequency / processor.getBandFrequency(i);
            const double q = processor.getBandQ(i);
            const double w = (ratio - 1.0 / ratio) * q;
            total += gain / (1.0 + w * w);
        }
        return (float)juce::jlimit(-18.0, 18.0, total);
    };

    juce::Path curve;
    constexpr int samples = 500;
    for (int i = 0; i <= samples; ++i)
    {
        const float t = (float)i / (float)samples;
        const float hz = minFrequency * std::pow(maxFrequency / minFrequency, t);
        const float x = graph.getX() + t * graph.getWidth();
        const float y = yForGain(responseAt(hz));
        if (i == 0) curve.startNewSubPath(x, y); else curve.lineTo(x, y);
    }
    g.setColour(juce::Colour::fromRGB(40, 205, 255));
    g.strokePath(curve, juce::PathStrokeType(1.8f));

    for (int i = 0; i < AudioEnergyAnalyzerEQAudioProcessor::bandCount; ++i)
    {
        const float hz = processor.getBandFrequency(i);
        const auto* raw = processor.parameters.getRawParameterValue("band" + juce::String(i));
        const float db = raw != nullptr ? raw->load() : 0.0f;
        const float x = xForFrequency(hz);
        const float y = yForGain(db);
        g.setColour(juce::Colour::fromRGB(255, 184, 54));
        g.fillEllipse(x - 7.0f, y - 7.0f, 14.0f, 14.0f);
    }

    const int columnWidth = bounds.getWidth() / AudioEnergyAnalyzerEQAudioProcessor::bandCount;
    auto labelsArea = bounds.withY(plot.getBottom() + 4).withHeight(52);
    for (int i = 0; i < AudioEnergyAnalyzerEQAudioProcessor::bandCount; ++i)
    {
        auto col = labelsArea.withX(labelsArea.getX() + i * columnWidth)
                             .withWidth(i == AudioEnergyAnalyzerEQAudioProcessor::bandCount - 1
                                 ? labelsArea.getRight() - (labelsArea.getX() + i * columnWidth) : columnWidth);
        const auto* raw = processor.parameters.getRawParameterValue("band" + juce::String(i));
        const float db = raw != nullptr ? raw->load() : 0.0f;
        g.setFont(12.75f);
        g.setColour(juce::Colours::white);
        g.drawFittedText(juce::String(processor.getBandFrequency(i), 1) + " Hz", col.withY(labelsArea.getY()).withHeight(20), juce::Justification::centred, 1);
        g.setFont(juce::Font(15.0f, juce::Font::bold));
        g.setColour(juce::Colours::yellow);
        g.drawText(juce::String(db, 1) + " dB", col.getX(), labelsArea.getY() + 24, col.getWidth(), 18, juce::Justification::centred);
        g.drawText("Q " + juce::String(processor.getBandQ(i), 2), col.getX(), labelsArea.getY() + 44, col.getWidth(), 18, juce::Justification::centred);
    }
}

void AudioEnergyAnalyzerEQAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(8);
    auto top = area.removeFromTop(24);
    title.setBounds(top.removeFromLeft(area.getWidth() / 2));
    resetButton.setBounds(top.removeFromRight(114).reduced(0, 1));
    loadButton.setBounds(top.removeFromRight(183).reduced(2, 1));
    fileStatus.setBounds(area.removeFromTop(18));
}

void AudioEnergyAnalyzerEQAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    dragBand = -1;
    auto plot = getLocalBounds().reduced(8).withTrimmedTop(62);
    plot = plot.withHeight(juce::jmax(90, plot.getHeight() - 78));
    const auto graph = plot.toFloat().reduced(30.0f, 12.0f);
    const float minX = graph.getX(), maxX = graph.getRight();
    const float maxY = graph.getBottom();
    float bestDistance = 14.0f;
    for (int i = 0; i < AudioEnergyAnalyzerEQAudioProcessor::bandCount; ++i)
    {
        const float hz = processor.getBandFrequency(i);
        const float x = minX + std::log(hz / minFrequency) / std::log(maxFrequency / minFrequency) * graph.getWidth();
        const auto* raw = processor.parameters.getRawParameterValue("band" + juce::String(i));
        const float db = raw != nullptr ? raw->load() : 0.0f;
        const float y = maxY - (db - minGain) / (maxGain - minGain) * graph.getHeight();
        const float distance = std::hypot(event.position.x - x, event.position.y - y);
        if (distance < bestDistance) { bestDistance = distance; dragBand = i; }
    }
}

void AudioEnergyAnalyzerEQAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (dragBand < 0) return;
    auto plot = getLocalBounds().reduced(8).withTrimmedTop(62);
    plot = plot.withHeight(juce::jmax(90, plot.getHeight() - 78));
    const auto graph = plot.toFloat().reduced(30.0f, 12.0f);
    const float db = juce::jlimit(minGain, maxGain, maxGain - (event.position.y - graph.getY()) / graph.getHeight() * (maxGain - minGain));
    if (auto* param = processor.parameters.getParameter("band" + juce::String(dragBand)))
        param->setValueNotifyingHost(param->convertTo0to1(db));
    repaint();
}
