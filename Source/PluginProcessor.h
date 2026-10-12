#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

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
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool loadEqSettings(const juce::File& file, juce::String& error);
    bool loadRatioReference(const juce::File& file, juce::String& error);
    juce::String getEqFileName() const;
    juce::String getReferenceFileName() const;
    juce::String getStatusText() const;
    juce::String getRatioSummary() const;

private:
    struct BandSetting
    {
        double frequencyHz = 1000.0;
        double q = 0.707;
        double gainDb = 0.0;
        int minHz = 0;
        int maxHz = 0;
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        std::array<double, 2> z1 { 0.0, 0.0 }, z2 { 0.0, 0.0 };
    };

    void updateCoefficients();
    void resetFilterState();
    static bool parseRatioArray(const juce::var& value, std::array<double, 10>& out);

    mutable juce::CriticalSection stateLock;
    std::array<BandSetting, 10> bands;
    std::array<double, 10> targetRatios {};
    bool hasTargetRatios = false;
    juce::String eqFileName = "未読込";
    juce::String referenceFileName = "未読込";
    juce::String statusText = "EQ設定JSONと比率リファレンスJSONを読み込んでください。";
    double currentSampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClubAudioAnalyzerEQAudioProcessor)
};
