#pragma once
#include <JuceHeader.h>
#include <array>

class ClubAudioAnalyzerEQAudioProcessor final : public juce::AudioProcessor
{
public:
    ClubAudioAnalyzerEQAudioProcessor();
    ~ClubAudioAnalyzerEQAudioProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    bool loadReferenceJson(const juce::File&, juce::String&);
    bool loadAnalysisTxt(const juce::File&, juce::String&);
    juce::String getReferenceName() const { return referenceName; }
    static constexpr int bandCount = 10;
    static const std::array<float, bandCount> centresHz;
    static const std::array<float, bandCount> qValues;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    float getBandGain(int index) const;
    float getResponseDb(float frequency) const;

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    std::array<std::array<Filter, 2>, bandCount> filters;
    std::array<float, bandCount> lastGainDb{};
    double currentSampleRate = 44100.0;
    juce::String referenceName { "Manual / default" };
    void updateFilters();
    void optimizeGains(const std::array<double, bandCount>& desiredDb);
    static double bellResponse(double frequency, double centre, double q);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClubAudioAnalyzerEQAudioProcessor)
};
