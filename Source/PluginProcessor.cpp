#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <array>
#include <cmath>

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
double ClubAudioAnalyzerEQAudioProcessor::bellResponse(double frequency,double centre,double q)
{
    if(frequency<=0||centre<=0) return 0.0;
    const double x=frequency/centre-centre/frequency;
    return 1.0/std::sqrt(1.0+q*q*x*x);
}
float ClubAudioAnalyzerEQAudioProcessor::getResponseDb(float frequency) const
{
    double total=0.0;
    for(int i=0;i<bandCount;++i) total+=getBandGain(i)*bellResponse(frequency,centresHz[(size_t)i],qValues[(size_t)i]);
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
    double a[bandCount][bandCount]{};
    double x[bandCount]{};
    for(int row=0;row<bandCount;++row)
        for(int col=0;col<bandCount;++col)
            a[row][col]=bellResponse(centresHz[(size_t)row],centresHz[(size_t)col],qValues[(size_t)col]);
    // Coordinate descent minimizes all band-centre residuals together, with a small
    // regularizer to avoid extreme boosts where filters overlap.
    for(int i=0;i<bandCount;++i) x[i]=0.0;
    for(int iteration=0;iteration<180;++iteration)
        for(int col=0;col<bandCount;++col)
        {
            double numerator=0.0,denominator=0.10;
            for(int row=0;row<bandCount;++row)
            {
                double residual=desiredDb[(size_t)row];
                for(int k=0;k<bandCount;++k) if(k!=col) residual-=a[row][k]*x[k];
                numerator+=a[row][col]*residual;
                denominator+=a[row][col]*a[row][col];
            }
            x[col]=juce::jlimit(-12.0,12.0,numerator/denominator);
        }
    for(int i=0;i<bandCount;++i)
        if(auto* p=parameters.getParameter("band"+juce::String(i)))
            p->setValueNotifyingHost(p->convertTo0to1((float)x[i]));
}
bool ClubAudioAnalyzerEQAudioProcessor::loadAnalysisTxt(const juce::File& file,juce::String& message)
{
    if(!file.existsAsFile()){message="TXT file does not exist.";return false;}
    const auto lines=juce::StringArray::fromLines(file.loadFileAsString());
    std::array<double,bandCount> desired{}; std::array<bool,bandCount> found{};
    const juce::StringArray labels={"20~60 Hz","60~120 Hz","120~250 Hz","250~500 Hz","500~1000 Hz","1000~2000 Hz","2000~5000 Hz","5000~10000 Hz","10000~16000 Hz","16000~20000 Hz"};
    for(const auto& line:lines)
    {
        if(!line.contains("|")) continue;
        auto cols=juce::StringArray::fromTokens(line,"|","");
        if(cols.size()<6) continue;
        const auto bandText=cols[0].trim().replace("–","~").replace("-","~");
        int index=-1;
        for(int i=0;i<bandCount;++i)
        {
            auto norm=labels[i].replace("–","~");
            if(bandText.containsIgnoreCase(norm)){index=i;break;}
        }
        if(index<0) continue;
        auto diffText=cols[5].trim().replace("dB","");
        const double diff=diffText.getDoubleValue();
        if(!std::isfinite(diff)) continue;
        desired[(size_t)index]=-diff;
        found[(size_t)index]=true;
    }
    int count=0; for(bool b:found) if(b) ++count;
    if(count==0){message="No band Diff values found. Expected the ClubAudioAnalyzer TXT table with Band and Diff columns.";return false;}
    // Missing bands remain at 0 dB correction; optimize the complete 10-band curve.
    optimizeGains(desired);
    referenceName=file.getFileName();
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
