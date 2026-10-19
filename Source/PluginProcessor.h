#pragma once
#include <JuceHeader.h>

class AudioEnergyAnalyzerEQAudioProcessor final : public juce::AudioProcessor
{
public:
    AudioEnergyAnalyzerEQAudioProcessor();
    ~AudioEnergyAnalyzerEQAudioProcessor() override = default;

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
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    bool loadReferenceJson(const juce::File& file, juce::String& message);
    juce::String getReferenceName() const { return referenceName; }
    static constexpr int bandCount = 10;
    static const std::array<float, bandCount> centresHz;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    std::array<std::array<Filter, 2>, bandCount> filters;
    std::array<float, bandCount> lastGainDb{};
    double currentSampleRate = 44100.0;
    juce::String referenceName { "Manual / default" };
    void updateFilters();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEnergyAnalyzerEQAudioProcessor)
};
