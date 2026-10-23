#include "PluginEditor.h"
#include <cmath>

namespace
{
constexpr float minFrequency = 20.0f;
constexpr float maxFrequency = 20000.0f;
constexpr float minGain = -18.0f;
constexpr float maxGain = 18.0f;
constexpr float defaultWidth = 660.0f;
constexpr float defaultHeight = 444.0f;
const char* bandNames[] = { "20 ~ 60 Hz", "60 ~ 120 Hz", "120 ~ 250 Hz", "250 ~ 500 Hz",
    "500 ~ 1000 Hz", "1000 ~ 2000 Hz", "2000 ~ 5000 Hz", "5000 ~ 10000 Hz",
    "10000 ~ 16000 Hz", "16000 ~ 20000 Hz" };
}

AudioEnergyAnalyzerEQAudioProcessorEditor::AudioEnergyAnalyzerEQAudioProcessorEditor(AudioEnergyAnalyzerEQAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(660, 444);
    setResizable(true, true);
    setResizeLimits(660, 444, 1800, 1200);
    loadButton.setLookAndFeel(&responsiveLookAndFeel);
    resetButton.setLookAndFeel(&responsiveLookAndFeel);

    loadButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(144, 238, 60));
    loadButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(144, 238, 60));
    loadButton.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(0, 0, 255));
    loadButton.setColour(juce::TextButton::textColourOnId, juce::Colour::fromRGB(0, 0, 255));
    resetButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(144, 238, 60));
    resetButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(144, 238, 60));
    resetButton.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(0, 0, 255));
    resetButton.setColour(juce::TextButton::textColourOnId, juce::Colour::fromRGB(0, 0, 255));

    title.setText("AudioEnergyAnalyzer EQ", juce::dontSendNotification);
    title.setFont(juce::Font(18.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(title);

    addAndMakeVisible(loadButton);
    addAndMakeVisible(resetButton);
    fileStatus.setText("", juce::dontSendNotification);
    fileStatus.setFont(juce::Font(11.25f));
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
                    fileStatus.setText(loaded ? file.getFileName() : juce::String{}, juce::dontSendNotification);
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
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2.25f), 4.5f, 0.75f);

    auto bounds = getLocalBounds().reduced(6);
    bounds.removeFromTop(46);
    auto plot = bounds.removeFromTop(juce::jmax(67, bounds.getHeight() - 58)).reduced(0, 0);
    g.setColour(juce::Colour::fromRGB(13, 17, 29));
    g.fillRoundedRectangle(plot.toFloat(), 2.5f);

    const auto graph = plot.toFloat().reduced(22.5f, 9.0f);
    auto xForFrequency = [&graph](float hz)
    {
        const float t = std::log(hz / minFrequency) / std::log(maxFrequency / minFrequency);
        return graph.getX() + t * graph.getWidth();
    };
    auto yForGain = [&graph](float db)
    {
        return graph.getBottom() - (db - minGain) / (maxGain - minGain) * graph.getHeight();
    };

    g.setFont(10.125f * (getWidth() / defaultWidth));
    for (int db = -18; db <= 18; db += 6)
    {
        const float y = yForGain((float)db);
        g.setColour(db == 0 ? juce::Colours::white.withAlpha(0.35f) : juce::Colours::white.withAlpha(0.12f));
        g.drawHorizontalLine((int)y, graph.getX(), graph.getRight());
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.drawText(juce::String(db), (int)plot.getX(), (int)y - 6, 28, 13, juce::Justification::centredRight);
    }

    const float ticks[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    for (float hz : ticks)
    {
        const float x = xForFrequency(hz);
        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawVerticalLine((int)x, graph.getY(), graph.getBottom());
        juce::String label = hz >= 1000.0f ? juce::String(hz / 1000.0f, hz >= 10000.0f ? 0 : 1) + "k" : juce::String((int)hz);
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawText(label, (int)x - 13, (int)graph.getBottom(), 40, 12, juce::Justification::centred);
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
    g.strokePath(curve, juce::PathStrokeType(2.7f));

    for (int i = 0; i < AudioEnergyAnalyzerEQAudioProcessor::bandCount; ++i)
    {
        const float hz = processor.getBandFrequency(i);
        const auto* raw = processor.parameters.getRawParameterValue("band" + juce::String(i));
        const float db = raw != nullptr ? raw->load() : 0.0f;
        const float x = xForFrequency(hz);
        const float y = yForGain(db);
        g.setColour(juce::Colour::fromRGB(255, 184, 54));
        g.fillEllipse(x - 5.25f, y - 5.25f, 10.5f, 10.5f);
    }

    const int columnWidth = bounds.getWidth() / AudioEnergyAnalyzerEQAudioProcessor::bandCount;
    auto labelsArea = bounds.withY(plot.getBottom() + 3).withHeight(39);
    for (int i = 0; i < AudioEnergyAnalyzerEQAudioProcessor::bandCount; ++i)
    {
        auto col = labelsArea.withX(labelsArea.getX() + i * columnWidth)
                             .withWidth(i == AudioEnergyAnalyzerEQAudioProcessor::bandCount - 1
                                 ? labelsArea.getRight() - (labelsArea.getX() + i * columnWidth) : columnWidth);
        const auto* raw = processor.parameters.getRawParameterValue("band" + juce::String(i));
        const float db = raw != nullptr ? raw->load() : 0.0f;
        g.setFont(6.40875f * (getWidth() / defaultWidth));
        g.setColour(juce::Colours::white);
        g.drawFittedText(juce::String(processor.getBandFrequency(i), 1) + " Hz", col.withY(labelsArea.getY()).withHeight(15), juce::Justification::centred, 1);
        g.setFont(juce::Font(7.5375f * (getWidth() / defaultWidth), juce::Font::bold));
        g.setColour(juce::Colours::yellow);
        g.drawText(juce::String(db, 1) + " dB", col.getX(), labelsArea.getY() + 18, col.getWidth(), 13, juce::Justification::centred);
        g.drawText("Q " + juce::String(processor.getBandQ(i), 2), col.getX(), labelsArea.getY() + 33, col.getWidth(), 13, juce::Justification::centred);
    }
}

void AudioEnergyAnalyzerEQAudioProcessorEditor::resized()
{
    const float textScale = juce::jmin(getWidth() / defaultWidth, getHeight() / defaultHeight);
    title.setFont(juce::Font(18.0f * textScale, juce::Font::bold));
    fileStatus.setFont(juce::Font(11.25f * textScale));
    auto area = getLocalBounds().reduced(9);
    const int topHeight = juce::roundToInt(43.5f * textScale);
    const int resetWidth = juce::roundToInt(162.0f * textScale);
    const int loadWidth = juce::roundToInt(256.5f * textScale);
    auto top = area.removeFromTop(topHeight);
    resetButton.setBounds(top.removeFromRight(resetWidth).reduced(0, 0));
    loadButton.setBounds(top.removeFromRight(loadWidth).reduced(1, 0));
    title.setBounds(top);
    fileStatus.setBounds(area.removeFromTop(30 * textScale));
}

void AudioEnergyAnalyzerEQAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    dragBand = -1;
    auto plot = getLocalBounds().reduced(6).withTrimmedTop(46);
    plot = plot.withHeight(juce::jmax(67, plot.getHeight() - 58));
    const auto graph = plot.toFloat().reduced(22.5f, 9.0f);
    const float minX = graph.getX(), maxX = graph.getRight();
    const float maxY = graph.getBottom();
    float bestDistance = 10.5f;
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
    auto plot = getLocalBounds().reduced(6).withTrimmedTop(46);
    plot = plot.withHeight(juce::jmax(67, plot.getHeight() - 58));
    const auto graph = plot.toFloat().reduced(22.5f, 9.0f);
    const float db = juce::jlimit(minGain, maxGain, maxGain - (event.position.y - graph.getY()) / graph.getHeight() * (maxGain - minGain));
    if (auto* param = processor.parameters.getParameter("band" + juce::String(dragBand)))
        param->setValueNotifyingHost(param->convertTo0to1(db));
    repaint();
}
