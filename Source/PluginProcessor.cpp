#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

ClubAudioAnalyzerEQAudioProcessor::ClubAudioAnalyzerEQAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    constexpr std::array<double, 10> centres { 34.641, 84.853, 173.205, 353.553, 707.107,
                                               1414.214, 2828.427, 5656.854, 11313.708, 17888.544 };
    constexpr std::array<double, 10> qs { 0.866025, 1.414214, 0.999999, 1.414214, 1.414214,
                                          1.414214, 1.414214, 1.414214, 1.414214, 2.0 };
    for (size_t i = 0; i < bands.size(); ++i)
    {
        bands[i].frequencyHz = centres[i];
        bands[i].q = qs[i];
    }
}

void ClubAudioAnalyzerEQAudioProcessor::prepareToPlay(double sampleRate, int)
{
    const juce::ScopedLock lock(stateLock);
    currentSampleRate = sampleRate;
    updateCoefficients();
    resetFilterState();
}

void ClubAudioAnalyzerEQAudioProcessor::releaseResources() {}

bool ClubAudioAnalyzerEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void ClubAudioAnalyzerEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const juce::ScopedLock lock(stateLock);
    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int n = 0; n < samples; ++n)
        {
            double y = static_cast<double>(data[n]);
            for (auto& band : bands)
            {
                const double out = band.b0 * y + band.z1[static_cast<size_t>(ch)];
                band.z1[static_cast<size_t>(ch)] = band.b1 * y - band.a1 * out + band.z2[static_cast<size_t>(ch)];
                band.z2[static_cast<size_t>(ch)] = band.b2 * y - band.a2 * out;
                y = out;
            }
            data[n] = static_cast<float>(y);
        }
    }
    for (int ch = channels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, samples);
}

juce::AudioProcessorEditor* ClubAudioAnalyzerEQAudioProcessor::createEditor()
{
    return new ClubAudioAnalyzerEQAudioProcessorEditor(*this);
}

void ClubAudioAnalyzerEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    const juce::ScopedLock lock(stateLock);
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty("eq_file", eqFileName);
    root->setProperty("reference_file", referenceFileName);
    juce::Array<juce::var> gains;
    juce::Array<juce::var> savedBands;
    for (const auto& band : bands)
    {
        gains.add(band.gainDb);
        juce::DynamicObject::Ptr item = new juce::DynamicObject();
        item->setProperty("frequency_hz", band.frequencyHz);
        item->setProperty("q", band.q);
        item->setProperty("gain_db", band.gainDb);
        item->setProperty("min_hz", band.minHz);
        item->setProperty("max_hz", band.maxHz);
        savedBands.add(juce::var(item));
    }
    root->setProperty("gain_db", gains);
    root->setProperty("eq_bands", savedBands);
    juce::Array<juce::var> ratios;
    for (double value : targetRatios) ratios.add(value);
    root->setProperty("target_ratio_pct", ratios);
    root->setProperty("has_target_ratios", hasTargetRatios);
    const auto json = juce::JSON::toString(juce::var(root), true);
    destData.replaceAll(json.toRawUTF8(), json.getNumBytesAsUTF8());
}

void ClubAudioAnalyzerEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    const auto parsed = juce::JSON::parse(juce::String::fromUTF8(static_cast<const char*>(data), sizeInBytes));
    if (! parsed.isObject()) return;
    const juce::ScopedLock lock(stateLock);
    const auto obj = parsed.getDynamicObject();
    if (obj == nullptr) return;
    eqFileName = obj->getProperty("eq_file").toString();
    referenceFileName = obj->getProperty("reference_file").toString();
    const auto savedBands = obj->getProperty("eq_bands");
    if (savedBands.isArray() && savedBands.size() == 10)
    {
        for (int i = 0; i < 10; ++i)
        {
            const auto item = savedBands[i];
            if (! item.isObject()) continue;
            const auto entry = item.getDynamicObject();
            auto& band = bands[static_cast<size_t>(i)];
            band.frequencyHz = static_cast<double>(entry->getProperty("frequency_hz"));
            band.q = static_cast<double>(entry->getProperty("q"));
            band.gainDb = static_cast<double>(entry->getProperty("gain_db"));
            band.minHz = static_cast<int>(entry->getProperty("min_hz"));
            band.maxHz = static_cast<int>(entry->getProperty("max_hz"));
        }
    }
    else
    {
        const auto gains = obj->getProperty("gain_db");
        if (gains.isArray() && gains.size() == 10)
            for (int i = 0; i < 10; ++i) bands[static_cast<size_t>(i)].gainDb = static_cast<double>(gains[i]);
    }
    std::array<double, 10> ratios {};
    if (parseRatioArray(obj->getProperty("target_ratio_pct"), ratios))
    {
        targetRatios = ratios;
        hasTargetRatios = static_cast<bool>(obj->getProperty("has_target_ratios"));
    }
    updateCoefficients();
    resetFilterState();
}

bool ClubAudioAnalyzerEQAudioProcessor::parseRatioArray(const juce::var& value, std::array<double, 10>& out)
{
    if (! value.isArray() || value.size() != 10) return false;
    double sum = 0.0;
    for (int i = 0; i < 10; ++i)
    {
        const double v = static_cast<double>(value[i]);
        if (! std::isfinite(v) || v <= 0.0) return false;
        out[static_cast<size_t>(i)] = v;
        sum += v;
    }
    return std::abs(sum - 100.0) <= 0.1;
}

bool ClubAudioAnalyzerEQAudioProcessor::loadEqSettings(const juce::File& file, juce::String& error)
{
    const auto parsed = juce::JSON::parse(file);
    if (! parsed.isObject()) { error = "EQ設定JSONを解析できません。"; return false; }
    const auto obj = parsed.getDynamicObject();
    const auto list = obj->getProperty("eq_bands");
    if (! list.isArray() || list.size() != 10)
    { error = "eq_bands は10帯域必要です。ClubAudioAnalyzerのVST3用EQ設定JSONを選択してください。"; return false; }

    std::array<BandSetting, 10> incoming = bands;
    for (int i = 0; i < 10; ++i)
    {
        const auto item = list[i];
        if (! item.isObject()) { error = "eq_bands の形式が不正です。"; return false; }
        const auto entry = item.getDynamicObject();
        const double frequency = entry->getProperty("frequency_hz");
        const double q = entry->getProperty("q");
        const double gain = entry->getProperty("gain_db");
        const auto filterType = entry->getProperty("filter_type").toString();
        if (! std::isfinite(frequency) || ! std::isfinite(q) || ! std::isfinite(gain)
            || frequency < 10.0 || q <= 0.0 || std::abs(gain) > 24.0
            || (! filterType.isEmpty() && ! filterType.equalsIgnoreCase("Bell")))
        { error = "EQ設定に範囲外または非対応の値があります。"; return false; }
        incoming[static_cast<size_t>(i)].frequencyHz = frequency;
        incoming[static_cast<size_t>(i)].q = q;
        incoming[static_cast<size_t>(i)].gainDb = gain;
        incoming[static_cast<size_t>(i)].minHz = static_cast<int>(entry->getProperty("min_hz"));
        incoming[static_cast<size_t>(i)].maxHz = static_cast<int>(entry->getProperty("max_hz"));
    }
    const juce::ScopedLock lock(stateLock);
    bands = incoming;
    eqFileName = file.getFileName();
    const auto ratios = obj->getProperty("target_ratio_pct");
    std::array<double, 10> parsedRatios {};
    if (parseRatioArray(ratios, parsedRatios)) { targetRatios = parsedRatios; hasTargetRatios = true; }
    updateCoefficients();
    resetFilterState();
    statusText = "EQ設定を読み込みました。";
    error.clear();
    return true;
}

bool ClubAudioAnalyzerEQAudioProcessor::loadRatioReference(const juce::File& file, juce::String& error)
{
    const auto parsed = juce::JSON::parse(file);
    if (! parsed.isObject()) { error = "比率リファレンスJSONを解析できません。"; return false; }
    const auto obj = parsed.getDynamicObject();
    std::array<double, 10> ratios {};
    if (! parseRatioArray(obj->getProperty("target_ratio_pct"), ratios))
    {
        const auto db = obj->getProperty("target_db");
        if (! db.isArray() || db.size() != 10) { error = "target_ratio_pct または10要素の target_db が必要です。"; return false; }
        double sum = 0.0;
        for (int i = 0; i < 10; ++i)
        {
            const double value = static_cast<double>(db[i]);
            if (! std::isfinite(value)) { error = "target_db に不正な値があります。"; return false; }
            ratios[static_cast<size_t>(i)] = std::pow(10.0, value / 10.0);
            sum += ratios[static_cast<size_t>(i)];
        }
        if (! std::isfinite(sum) || sum <= 0.0) { error = "target_db から比率を計算できません。"; return false; }
        for (auto& value : ratios) value = value / sum * 100.0;
    }
    const juce::ScopedLock lock(stateLock);
    targetRatios = ratios;
    hasTargetRatios = true;
    referenceFileName = file.getFileName();
    statusText = "比率リファレンスを読み込みました。EQ設定JSONは音声処理用に別途必要です。";
    error.clear();
    return true;
}

juce::String ClubAudioAnalyzerEQAudioProcessor::getEqFileName() const { const juce::ScopedLock lock(stateLock); return eqFileName; }
juce::String ClubAudioAnalyzerEQAudioProcessor::getReferenceFileName() const { const juce::ScopedLock lock(stateLock); return referenceFileName; }
juce::String ClubAudioAnalyzerEQAudioProcessor::getStatusText() const { const juce::ScopedLock lock(stateLock); return statusText; }
juce::String ClubAudioAnalyzerEQAudioProcessor::getRatioSummary() const
{
    const juce::ScopedLock lock(stateLock);
    if (! hasTargetRatios) return "目標比率：未読込";
    juce::String result = "目標比率 %（10帯域）: ";
    for (size_t i = 0; i < targetRatios.size(); ++i)
    {
        if (i != 0) result += " / ";
        result += juce::String(targetRatios[i], 2);
    }
    return result;
}

void ClubAudioAnalyzerEQAudioProcessor::updateCoefficients()
{
    const double sr = juce::jmax(1000.0, currentSampleRate);
    for (auto& band : bands)
    {
        const double frequency = juce::jlimit(10.0, sr * 0.49, band.frequencyHz);
        const double q = juce::jmax(0.05, band.q);
        const double A = std::pow(10.0, band.gainDb / 40.0);
        const double w0 = juce::MathConstants<double>::twoPi * frequency / sr;
        const double alpha = std::sin(w0) / (2.0 * q);
        const double cw = std::cos(w0);
        const double a0 = 1.0 + alpha / A;
        band.b0 = (1.0 + alpha * A) / a0;
        band.b1 = (-2.0 * cw) / a0;
        band.b2 = (1.0 - alpha * A) / a0;
        band.a1 = (-2.0 * cw) / a0;
        band.a2 = (1.0 - alpha / A) / a0;
    }
}

void ClubAudioAnalyzerEQAudioProcessor::resetFilterState()
{
    for (auto& band : bands) { band.z1 = { 0.0, 0.0 }; band.z2 = { 0.0, 0.0 }; }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ClubAudioAnalyzerEQAudioProcessor();
}
