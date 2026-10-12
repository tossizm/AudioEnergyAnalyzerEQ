#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <array>
#include <cmath>\n#include <limits>

const std::array<float, ClubAudioAnalyzerEQAudioProcessor::bandCount> ClubAudioAnalyzerEQAudioProcessor::centresHz
    = { 40.0f, 84.85f, 173.2f, 353.55f, 707.1f, 1414.2f, 3162.3f, 7071.1f, 12649.1f, 17888.5f };
const std::array<float, ClubAudioAnalyzerEQAudioProcessor::bandCount> ClubAudioAnalyzerEQAudioProcessor::qValues
    = { 0.87f, 1.41f, 1.33f, 1.41f, 1.41f, 1.41f, 1.05f, 1.41f, 2.0f, 2.0f };

ClubAudioAnalyzerEQAudioProcessor::ClubAudioAnalyzerEQAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{ lastGainDb.fill(999.0f); }

juce::AudioProcessorValueTreeState::ParameterLayout ClubAudioAnalyzerEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i=0;i<bandCount;++i)
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"band"+juce::String(i),1},
            "Band "+juce::String(i+1),juce::NormalisableRange<float>(-12.0f,12.0f,0.01f),0.0f,"dB"));
    return layout;
}
void ClubAudioAnalyzerEQAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate=sampleRate;
    juce::dsp::ProcessSpec spec{sampleRate,2048,(juce::uint32)juce::jmax(1,getTotalNumOutputChannels())};
    for(auto& band:filters) for(auto& filter:band) filter.prepare(spec);
    lastGainDb.fill(999.0f); updateFilters();
}
void ClubAudioAnalyzerEQAudioProcessor::releaseResources() {}
bool ClubAudioAnalyzerEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out=layouts.getMainOutputChannelSet();
    return (out==juce::AudioChannelSet::mono()||out==juce::AudioChannelSet::stereo())&&out==layouts.getMainInputChannelSet();
}
float ClubAudioAnalyzerEQAudioProcessor::getBandGain(int index) const
{
    if(index<0||index>=bandCount) return 0.0f;
    const auto* p=parameters.getRawParameterValue("band"+juce::String(index));
    return p?p->load():0.0f;
}
// Evaluate the exact JUCE peak-filter transfer function in dB. This mirrors
// makePeakFilter() used by the audio path instead of the former bell-shaped
// approximation, so the plotted response describes the actual serial biquads.
static double peakFilterMagnitudeDb(double frequency, double centre, double q,
                                    double gainDb, double sampleRate)
{
    if (frequency <= 0.0 || sampleRate <= 0.0)
        return 0.0;

    const double safeCentre = juce::jlimit(1.0, sampleRate * 0.45, centre);
    auto coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, safeCentre, q,
        juce::Decibels::decibelsToGain((float)gainDb));
    const auto* c = coefficients->getRawCoefficients(); // b0, b1, b2, a0, a1, a2
    const double omega = juce::MathConstants<double>::twoPi * frequency / sampleRate;
    const double c1 = std::cos(omega), s1 = std::sin(omega);
    const double c2 = std::cos(2.0 * omega), s2 = std::sin(2.0 * omega);
    const double nr = c[0] + c[1] * c1 + c[2] * c2;
    const double ni = -c[1] * s1 - c[2] * s2;
    const double dr = c[3] + c[4] * c1 + c[5] * c2;
    const double di = -c[4] * s1 - c[5] * s2;
    const double numerator = nr * nr + ni * ni;
    const double denominator = dr * dr + di * di;
    if (numerator <= 0.0 || denominator <= 0.0)
        return -120.0;
    return 10.0 * std::log10(numerator / denominator);
}

double ClubAudioAnalyzerEQAudioProcessor::bellResponse(double frequency,double centre,double q)
{
    if(frequency<=0||centre<=0) return 0.0;
    const double x=frequency/centre-centre/frequency;
    return 1.0/std::sqrt(1.0+q*q*x*x);
}
float ClubAudioAnalyzerEQAudioProcessor::getResponseDb(float frequency) const
{
    if (currentSampleRate <= 0.0 || frequency <= 0.0f)
        return 0.0f;

    // The ten filters run in series, therefore their dB magnitudes add.
    double total = 0.0;
    const double safeFrequency = juce::jlimit(1.0, currentSampleRate * 0.499,
                                               (double)frequency);
    for (int i = 0; i < bandCount; ++i)
        total += peakFilterMagnitudeDb(safeFrequency, centresHz[(size_t)i],
                                       qValues[(size_t)i], getBandGain(i),
                                       currentSampleRate);
    return (float)total;
}
void ClubAudioAnalyzerEQAudioProcessor::updateFilters()
{
    for(int i=0;i<bandCount;++i)
    {
        const float gain=getBandGain(i);
        if(std::abs(gain-lastGainDb[(size_t)i])<0.005f) continue;
        lastGainDb[(size_t)i]=gain;
        const float frequency=(float)juce::jlimit(20.0,currentSampleRate*0.45,(double)centresHz[(size_t)i]);
        auto coeff=juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate,frequency,qValues[(size_t)i],juce::Decibels::decibelsToGain(gain));
        for(auto& filter:filters[(size_t)i]) filter.coefficients=coeff;
    }
}
void ClubAudioAnalyzerEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for(int ch=getTotalNumInputChannels();ch<getTotalNumOutputChannels();++ch) buffer.clear(ch,0,buffer.getNumSamples());
    updateFilters();
    for(int ch=0;ch<juce::jmin(2,buffer.getNumChannels());++ch)
        for(int band=0;band<bandCount;++band)
        {
            juce::dsp::AudioBlock<float> block(buffer);
            auto oneChannel=block.getSingleChannelBlock((size_t)ch);
            juce::dsp::ProcessContextReplacing<float> context(oneChannel);
            filters[(size_t)band][(size_t)ch].process(context);
        }
}
void ClubAudioAnalyzerEQAudioProcessor::optimizeGains(const std::array<double,bandCount>& desiredDb)
{
    // Non-linear coordinate descent using the same JUCE peak-filter transfer
    // function as the audio path and response graph. The objective is the sum
    // of squared errors at all ten band centres plus a mild gain regularizer.
    std::array<double, bandCount> gains{};
    const double sampleRate = juce::jmax(22050.0, currentSampleRate);
    constexpr double gainLimit = 12.0;
    constexpr double gainStep = 0.25;
    constexpr double regularization = 0.025;
    constexpr int iterations = 24;

    auto responseAt = [&](int row, const std::array<double, bandCount>& candidate)
    {
        double response = 0.0;
        for (int band = 0; band < bandCount; ++band)
            response += peakFilterMagnitudeDb(centresHz[(size_t)row],
                centresHz[(size_t)band], qValues[(size_t)band],
                candidate[(size_t)band], sampleRate);
        return response;
    };

    for (int iteration = 0; iteration < iterations; ++iteration)
    {
        double largestChange = 0.0;
        for (int col = 0; col < bandCount; ++col)
        {
            double bestCost = std::numeric_limits<double>::infinity();
            double bestGain = gains[(size_t)col];
            for (int step = -48; step <= 48; ++step)
            {
                const double trialGain = step * gainStep;
                auto trial = gains;
                trial[(size_t)col] = trialGain;
                double cost = regularization * trialGain * trialGain;
                for (int row = 0; row < bandCount; ++row)
                {
                    const double error = responseAt(row, trial) - desiredDb[(size_t)row];
                    cost += error * error;
                }
                if (cost < bestCost)
                {
                    bestCost = cost;
                    bestGain = trialGain;
                }
            }
            largestChange = juce::jmax(largestChange,
                std::abs(bestGain - gains[(size_t)col]));
            gains[(size_t)col] = bestGain;
        }
        if (largestChange < gainStep)
            break;
    }

    for (int i = 0; i < bandCount; ++i)
        if (auto* p = parameters.getParameter("band" + juce::String(i)))
            p->setValueNotifyingHost(p->convertTo0to1((float)gains[(size_t)i]));
}
bool ClubAudioAnalyzerEQAudioProcessor::loadAnalysisTxt(const juce::File& file,juce::String& message)
{
    if(!file.existsAsFile()){message="TXT file does not exist.";return false;}
    const auto fileText = file.loadFileAsString();
    const auto lines=juce::StringArray::fromLines(fileText);
    // The analyzer TXT may include a reference name in its metadata. Accept common
    // English labels and preserve the text after the separator for display.
    for (const auto& rawLine : lines)
    {
        const auto line = rawLine.trim();
        const auto lower = line.toLowerCase();
        int separator = line.indexOfChar(':');
        if (separator < 0) separator = line.indexOfChar((juce_wchar)0xFF1A);
        if (separator > 0 && (lower.startsWith("reference") || lower.startsWith("リファレンスデータ")))
        {
            auto candidate = line.substring(separator + 1).trim();
            if (candidate.isNotEmpty()) { referenceName = candidate; break; }
        }
    }
    std::array<double,bandCount> desired{}; std::array<bool,bandCount> found{};
    const double lows[bandCount]={20,60,120,250,500,1000,2000,5000,10000,16000};
    const double highs[bandCount]={60,120,250,500,1000,2000,5000,10000,16000,20000};
    bool headerSeen=false;
    for(const auto& line:lines)
    {
        if(!line.contains("|")) continue;
        auto cols=juce::StringArray::fromTokens(line,"|","");
        if(cols.size()<6) continue;
        // Require the actual analyzer table header before interpreting column 5 as Diff.
        if(cols[0].trim().equalsIgnoreCase("Band"))
        {
            headerSeen=false;
            for(const auto& col:cols) if(col.trim().equalsIgnoreCase("Diff")) headerSeen=true;
            continue;
        }
        if(!headerSeen) continue;
        const auto bandText=cols[0].trim().replace(" ","").replace("–","~").replace("—","~").replace("-","~");
        int index=-1;
        for(int i=0;i<bandCount;++i)
        {
            const auto lo=juce::String((int)lows[i]), hi=juce::String((int)highs[i]);
            const auto compact=lo+"~"+hi+"Hz";
            const auto compactNoHz=lo+"~"+hi;
            if(bandText.equalsIgnoreCase(compact)||bandText.equalsIgnoreCase(compactNoHz)){index=i;break;}
        }
        if(index<0) continue;
        const auto diffText=cols[5].trim().replace("dB","").replace("%","").trim();
        const double diff=diffText.getDoubleValue();
        if(!std::isfinite(diff)) continue;
        // Analyzer convention: Diff = Sample - Target, so the first-pass correction is -Diff.
        desired[(size_t)index]=-diff;
        found[(size_t)index]=true;
    }
    int count=0; for(bool b:found) if(b) ++count;
    if(count==0)
    {
        message=headerSeen
            ? "No band rows parsed. Expected rows like '20 ~ 60 Hz | ... | +11.22 dB | ...'."
            : "Analyzer table header not found. Expected 'Band | Pre % | Correlation | Post % | EQ | Diff | Sample | Target'.";
        return false;
    }
    optimizeGains(desired);
    if (referenceName.isEmpty()) referenceName = "Not specified in TXT";
    message="Loaded "+juce::String(count)+"/10 band Diff values; jointly optimized the 10 EQ filters.";
    return true;
}
bool ClubAudioAnalyzerEQAudioProcessor::loadReferenceJson(const juce::File& file,juce::String& message)
{
    juce::var root; const auto parsed=juce::JSON::parse(file.loadFileAsString(),root);
    if(parsed.failed()){message="JSON parse error: "+parsed.getErrorMessage();return false;}
    auto* obj=root.getDynamicObject();
    if(obj==nullptr){message="JSON root must be an object.";return false;}
    std::array<double,bandCount> desired{}; int loaded=0;
    auto bands=obj->getProperty("bands");
    if(bands.isArray())
    {
        for(const auto& v:*bands.getArray())
        {
            auto* b=v.getDynamicObject(); if(!b) continue;
            int index=-1; bool oneBased=false;
            if(b->hasProperty("index")) index=(int)b->getProperty("index");
            else if(b->hasProperty("band")){index=(int)b->getProperty("band");oneBased=true;}
            else if(b->hasProperty("min_hz"))
            {
                const double lo=(double)b->getProperty("min_hz"),hi=(double)b->getProperty("max_hz");
                const double lows[]={20,60,120,250,500,1000,2000,5000,10000,16000};
                const double highs[]={60,120,250,500,1000,2000,5000,10000,16000,20000};
                for(int i=0;i<bandCount;++i) if(std::abs(lo-lows[i])<1.0&&std::abs(hi-highs[i])<1.0){index=i;break;}
            }
            if(oneBased&&index>=1&&index<=bandCount)--index;
            if(index<0||index>=bandCount) continue;
            double val=0.0;
            if(b->hasProperty("target_db")) val=(double)b->getProperty("target_db");
            else if(b->hasProperty("gain_db")) val=(double)b->getProperty("gain_db");
            else
            {
                double pct=0.0;
                if(b->hasProperty("energy_percent"))pct=(double)b->getProperty("energy_percent");
                else if(b->hasProperty("target_percent"))pct=(double)b->getProperty("target_percent");
                else if(b->hasProperty("ratio"))pct=(double)b->getProperty("ratio");
                if(pct>0.0) val=10.0*std::log10(pct/10.0);
            }
            if(!std::isfinite(val)) continue;
            desired[(size_t)index]=val; ++loaded;
        }
    }
    // Also accept a top-level target_db array (ClubAudioAnalyzer export schema).
    if(loaded==0)
    {
        auto values=obj->getProperty("target_db");
        if(values.isArray()&&values.getArray()->size()>=bandCount)
            for(int i=0;i<bandCount;++i){const double v=(double)(*values.getArray())[i];if(std::isfinite(v)){desired[(size_t)i]=v;++loaded;}}
    }
    if(loaded==0){message="No usable bands found. Expected 'bands' or a top-level 10-item target_db array.";return false;}
    optimizeGains(desired);
    referenceName=file.getFileName();
    message="Loaded "+juce::String(loaded)+" reference targets and jointly optimized all 10 filters. Gain limit: +/-12 dB.";
    return true;
}
void ClubAudioAnalyzerEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{auto state=parameters.copyState();std::unique_ptr<juce::XmlElement> xml(state.createXml());copyXmlToBinary(*xml,destData);}
void ClubAudioAnalyzerEQAudioProcessor::setStateInformation(const void* data,int sizeInBytes)
{std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data,sizeInBytes));if(xml&&xml->hasTagName(parameters.state.getType()))parameters.replaceState(juce::ValueTree::fromXml(*xml));}
juce::AudioProcessorEditor* ClubAudioAnalyzerEQAudioProcessor::createEditor(){return new ClubAudioAnalyzerEQAudioProcessorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new ClubAudioAnalyzerEQAudioProcessor();}
