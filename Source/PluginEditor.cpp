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
    setSize(1040, 700);
    title.setText("AudioEnergyAnalyzer EQ", juce::dontSendNotification);
    title.setFont(juce::Font(24.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(title);
    addAndMakeVisible(resetButton);
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
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(5.0f), 8.0f, 1.0f);

    auto bounds = getLocalBounds().reduced(16);
    bounds.removeFromTop(46);
    auto plot = bounds.removeFromTop(bounds.getHeight() - 112).reduced(0, 2);
    g.setColour(juce::Colour::fromRGB(13, 17, 29));
    g.fillRoundedRectangle(plot.toFloat(), 7.0f);

    const auto graph = plot.toFloat().reduced(42.0f, 18.0f);
    auto xForFrequency = [&graph](float hz)
    {
        const float t = std::log(hz / minFrequency) / std::log(maxFrequency / minFrequency);
        return graph.getX() + t * graph.getWidth();
    };
    auto yForGain = [&graph](float db)
    {
        return graph.getBottom() - (db - minGain) / (maxGain - minGain) * graph.getHeight();
    };

    g.setFont(11.0f);
    for (int db = -18; db <= 18; db += 6)
    {
        const float y = yForGain((float)db);
        g.setColour(db == 0 ? juce::Colours::white.withAlpha(0.35f) : juce::Colours::white.withAlpha(0.12f));
        g.drawHorizontalLine((int)y, graph.getX(), graph.getRight());
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.drawText(juce::String(db), (int)plot.getX() + 2, (int)y - 8, 32, 16, juce::Justification::centredRight);
    }

    const float ticks[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    for (float hz : ticks)
    {
        const float x = xForFrequency(hz);
        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawVerticalLine((int)x, graph.getY(), graph.getBottom());
        juce::String label = hz >= 1000.0f ? juce::String(hz / 1000.0f, hz >= 10000.0f ? 0 : 1) + "k" : juce::String((int)hz);
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawText(label, (int)x - 24, (int)graph.getBottom() + 3, 48, 15, juce::Justification::centred);
    }

    auto responseAt = [this](float frequency)
    {
        double total = 0.0;
        for (int i = 0; i < AudioEnergyAnalyzerEQAudioProcessor::bandCount; ++i)
        {
            const auto* raw = processor.parameters.getRawParameterValue("band" + juce::String(i));
            const double gain = raw != nullptr ? raw->load() : 0.0;
            const double ratio = frequency / AudioEnergyAnalyzerEQAudioProcessor::centresHz[(size_t)i];
            const double q = 1.0;
            const double w = (ratio - 1.0 / ratio) * q;
            total += gain / (1.0 + w * w);
        }
        return (float)juce::jlimit(-18.0, 18.0, total);
    };

    juce::Path curve;
    constexpr int samples = 600;
    for (int i = 0; i <= samples; ++i)
    {
        const float t = (float)i / (float)samples;
        const float hz = minFrequency * std::pow(maxFrequency / minFrequency, t);
        const float x = graph.getX() + t * graph.getWidth();
        const float y = yForGain(responseAt(hz));
        if (i == 0) curve.startNewSubPath(x, y); else curve.lineTo(x, y);
    }
    g.setColour(juce::Colour::fromRGB(40, 205, 255));
    g.strokePath(curve, juce::PathStrokeType(2.4f));

    for (int i = 0; i < AudioEnergyAnalyzerEQAudioProcessor::bandCount; ++i)
    {
        const float hz = AudioEnergyAnalyzerEQAudioProcessor::centresHz[(size_t)i];
        const auto* raw = processor.parameters.getRawParameterValue("band" + juce::String(i));
        const float db = raw != nullptr ? raw->load() : 0.0f;
        const float x = xForFrequency(hz);
        const float y = yForGain(db);
        g.setColour(juce::Colour::fromRGB(255, 184, 54));
        g.fillEllipse(x - 4.5f, y - 4.5f, 9.0f, 9.0f);
        g.setColour(juce::Colours::white);
        g.setFont(11.0f);
        g.drawText(juce::String(i + 1), (int)x - 10, (int)y - 20, 20, 14, juce::Justification::centred);
    }

    const int columnWidth = bounds.getWidth() / AudioEnergyAnalyzerEQAudioProcessor::bandCount;
    auto labelsArea = bounds.withY(plot.getBottom() + 8).withHeight(70);
    for (int i = 0; i < AudioEnergyAnalyzerEQAudioProcessor::bandCount; ++i)
    {
        auto col = labelsArea.withX(labelsArea.getX() + i * columnWidth)
                             .withWidth(i == AudioEnergyAnalyzerEQAudioProcessor::bandCount - 1
                                 ? labelsArea.getRight() - (labelsArea.getX() + i * columnWidth) : columnWidth);
        const auto* raw = processor.parameters.getRawParameterValue("band" + juce::String(i));
        const float db = raw != nullptr ? raw->load() : 0.0f;
        g.setFont(11.5f);
        g.setColour(juce::Colours::white);
        g.drawFittedText(bandNames[i], col, juce::Justification::centred, 1);
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.setColour(juce::Colours::yellow);
        g.drawText(juce::String(db, 2) + " dB", col.getX(), col.getY() + 27, col.getWidth(), 20, juce::Justification::centred);
    }
}

void AudioEnergyAnalyzerEQAudioProcessorEditor::resized()
{
    auto top = getLocalBounds().reduced(16).removeFromTop(36);
    title.setBounds(top.removeFromLeft(430));
    resetButton.setBounds(top.removeFromRight(100).reduced(0, 2));
}

void AudioEnergyAnalyzerEQAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    dragBand = -1;
    auto plot = getLocalBounds().reduced(16).withTrimmedTop(46);
    const auto graph = plot.removeFromTop(plot.getHeight() - 112).toFloat().reduced(42.0f, 18.0f);
    const float minX = graph.getX(), maxX = graph.getRight();
    const float minY = graph.getY(), maxY = graph.getBottom();
    float bestDistance = 18.0f;
    for (int i = 0; i < AudioEnergyAnalyzerEQAudioProcessor::bandCount; ++i)
    {
        const float hz = AudioEnergyAnalyzerEQAudioProcessor::centresHz[(size_t)i];
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
    auto plot = getLocalBounds().reduced(16).withTrimmedTop(46);
    const auto graph = plot.removeFromTop(plot.getHeight() - 112).toFloat().reduced(42.0f, 18.0f);
    const float db = juce::jlimit(minGain, maxGain, maxGain - (event.position.y - graph.getY()) / graph.getHeight() * (maxGain - minGain));
    if (auto* param = processor.parameters.getParameter("band" + juce::String(dragBand)))
        param->setValueNotifyingHost(param->convertTo0to1(db));
    repaint();
}
