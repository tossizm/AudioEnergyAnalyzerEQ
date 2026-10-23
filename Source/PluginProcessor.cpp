#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <array>
#include <cmath>
#include <regex>
#include <string>

const std::array<float, AudioEnergyAnalyzerEQAudioProcessor::bandCount> AudioEnergyAnalyzerEQAudioProcessor::centresHz
    = { 40.0f, 84.85f, 173.2f, 353.55f, 707.1f, 1414.2f, 3162.3f, 7071.1f, 12649.1f, 17888.5f };

AudioEnergyAnalyzerEQAudioProcessor::AudioEnergyAnalyzerEQAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    lastGainDb.fill(999.0f);
    bandQs.fill(1.0f);
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioEnergyAnalyzerEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < bandCount; ++i)
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "band" + juce::String(i), 1 },
            "Band " + juce::String(i + 1), juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f, "dB"));
    return layout;
}

void AudioEnergyAnalyzerEQAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, 2048, (juce::uint32) juce::jmax(1, getTotalNumOutputChannels()) };
    for (auto& band : filters)
        for (auto& filter : band)
            filter.prepare(spec);
    lastGainDb.fill(999.0f);
    updateFilters();
}
void AudioEnergyAnalyzerEQAudioProcessor::releaseResources() {}
bool AudioEnergyAnalyzerEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
        && out == layouts.getMainInputChannelSet();
}

void AudioEnergyAnalyzerEQAudioProcessor::updateFilters()
{
    for (int i = 0; i < bandCount; ++i)
    {
        const auto* raw = parameters.getRawParameterValue("band" + juce::String(i));
        const float gain = raw != nullptr ? raw->load() : 0.0f;
        if (std::abs(gain - lastGainDb[(size_t)i]) < 0.005f) continue;
        lastGainDb[(size_t)i] = gain;
        const auto frequency = (float) juce::jlimit(20.0, currentSampleRate * 0.45, (double)bandFrequencies[(size_t)i]);
        auto coeff = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate,
            frequency, bandQs[(size_t)i], juce::Decibels::decibelsToGain(gain));
        for (auto& filter : filters[(size_t)i]) filter.coefficients = coeff;
    }
}

void AudioEnergyAnalyzerEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int inputChannels = getTotalNumInputChannels();
    const int outputChannels = getTotalNumOutputChannels();
    for (int ch = inputChannels; ch < outputChannels; ++ch) buffer.clear(ch, 0, buffer.getNumSamples());
    updateFilters();
    for (int ch = 0; ch < juce::jmin(2, buffer.getNumChannels()); ++ch)
        for (int band = 0; band < bandCount; ++band)
        {
            juce::dsp::AudioBlock<float> block(buffer);
            auto oneChannel = block.getSingleChannelBlock((size_t)ch);
            juce::dsp::ProcessContextReplacing<float> context(oneChannel);
            filters[(size_t)band][(size_t)ch].process(context);
        }
}


bool AudioEnergyAnalyzerEQAudioProcessor::loadAnalysisTxt(const juce::File& file, juce::String& message)
{
    if (!file.existsAsFile())
    {
        message = "The selected TXT file does not exist.";
        return false;
    }

    const auto text = file.loadFileAsString();
    const std::string source = text.toStdString();
    const std::regex bellPattern(
        R"(Bell\\s*,\\s*([0-9]+(?:\\.[0-9]+)?)\\s*Hz\\s*,\\s*([-+]?[0-9]+(?:\\.[0-9]+)?)\\s*dB\\s*,\\s*Q\\s*([0-9]+(?:\\.[0-9]+)?))",
        std::regex_constants::icase);
    std::vector<std::array<float, 3>> parsedBands;
    for (std::sregex_iterator it(source.begin(), source.end(), bellPattern), end; it != end; ++it)
    {
        const float frequency = std::stof((*it)[1].str());
        const float gain = std::stof((*it)[2].str());
        const float q = std::stof((*it)[3].str());
        if (!std::isfinite(frequency) || !std::isfinite(gain) || !std::isfinite(q)
            || frequency < 20.0f || frequency > 20000.0f || q < 0.1f || q > 18.0f)
            continue;
        parsedBands.push_back({ frequency, juce::jlimit(-12.0f, 12.0f, gain), q });
        if (parsedBands.size() == (size_t)bandCount)
            break;
    }

    if (parsedBands.size() != (size_t)bandCount)
    {
        message = "Could not load EQ: expected 10 Bell entries formatted as 'Bell, 123.4 Hz, -2.0 dB, Q 1.00'. Found "
            + juce::String((int)parsedBands.size()) + " valid entries.";
        return false;
    }

    for (int i = 0; i < bandCount; ++i)
    {
        bandFrequencies[(size_t)i] = parsedBands[(size_t)i][0];
        bandQs[(size_t)i] = parsedBands[(size_t)i][2];
        if (auto* parameter = parameters.getParameter("band" + juce::String(i)))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(parsedBands[(size_t)i][1]));
    }
    referenceName = file.getFileName();
    lastGainDb.fill(999.0f);
    updateFilters();
    message = "Loaded 10 Bell EQ bands from " + file.getFileName() + ". Frequency, gain and Q are applied.";
    return true;
}

bool AudioEnergyAnalyzerEQAudioProcessor::loadReferenceJson(const juce::File& file, juce::String& message)
{
    juce::var root;
    const auto parsed = juce::JSON::parse(file.loadFileAsString(), root);
    if (parsed.failed()) { message = "JSON parse error: " + parsed.getErrorMessage(); return false; }
    auto* obj = root.getDynamicObject();
    if (obj == nullptr) { message = "JSON root must be an object."; return false; }
    juce::var bands = obj->getProperty("bands");
    if (!bands.isArray() || bands.getArray()->isEmpty()) { message = "JSON must contain a non-empty 'bands' array."; return false; }
    int loaded = 0;
    for (const auto& bandVar : *bands.getArray())
    {
        auto* band = bandVar.getDynamicObject();
        if (band == nullptr) continue;
        int index = -1;
        bool oneBased = false;
        if (band->hasProperty("index")) index = (int)band->getProperty("index");
        else if (band->hasProperty("band")) { index = (int)band->getProperty("band"); oneBased = true; }
        else if (band->hasProperty("min_hz"))
        {
            const double lo = (double)band->getProperty("min_hz");
            const double hi = (double)band->getProperty("max_hz");
            for (int i = 0; i < bandCount; ++i)
            {
                const double expectedLo[] = {20,60,120,250,500,1000,2000,5000,10000,16000};
                const double expectedHi[] = {60,120,250,500,1000,2000,5000,10000,16000,20000};
                if (std::abs(lo - expectedLo[i]) < 1.0 && std::abs(hi - expectedHi[i]) < 1.0) { index = i; break; }
            }
        }
        if (oneBased && index >= 1 && index <= bandCount) --index;
        if (index < 0 || index >= bandCount) continue;
        double targetDb = 0.0;
        if (band->hasProperty("target_db")) targetDb = (double)band->getProperty("target_db");
        else if (band->hasProperty("gain_db")) targetDb = (double)band->getProperty("gain_db");
        else
        {
            double pct = 0.0;
            if (band->hasProperty("energy_percent")) pct = (double)band->getProperty("energy_percent");
            else if (band->hasProperty("target_percent")) pct = (double)band->getProperty("target_percent");
            else if (band->hasProperty("ratio")) pct = (double)band->getProperty("ratio");
            if (pct > 0.0) targetDb = 10.0 * std::log10(pct / 10.0);
        }
        if (!std::isfinite(targetDb)) continue;
        if (auto* param = parameters.getParameter("band" + juce::String(index)))
            param->setValueNotifyingHost(param->convertTo0to1((float)juce::jlimit(-12.0, 12.0, targetDb)));
        ++loaded;
    }
    if (loaded == 0) { message = "No usable bands found. Expected index/band and target_db or energy_percent/target_percent/ratio."; return false; }
    referenceName = file.getFileName();
    message = "Loaded " + juce::String(loaded) + " bands from " + file.getFileName() + ". Percentages are converted to approximate relative dB versus a 10% reference and clamped to +/-12 dB.";
    return true;
}

void AudioEnergyAnalyzerEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
void AudioEnergyAnalyzerEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* AudioEnergyAnalyzerEQAudioProcessor::createEditor() { return new AudioEnergyAnalyzerEQAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AudioEnergyAnalyzerEQAudioProcessor(); }
