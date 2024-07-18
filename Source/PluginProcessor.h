#pragma once

#include <JuceHeader.h>

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

struct ChainSettings
{
    float peakFreq1 { 0 }, peakGainInDecibels1 { 0 }, peakQuality1 {1.f};
    float peakFreq2 { 0 }, peakGainInDecibels2 { 0 }, peakQuality2 {1.f};
    float peakFreq3 { 0 }, peakGainInDecibels3 { 0 }, peakQuality3 {1.f};
    float lowCutFreq { 0 }, highCutFreq { 0 }, lowCutQuality {1.f}, highCutQuality {1.f};
    float lowShelfFreq { 0 }, lowShelfGainInDecibels { 0 }, lowShelfQuality {1.f};
    float highShelfFreq { 0 }, highShelfGainInDecibels { 0 }, highShelfQuality {1.f};
    Slope lowCutSlope { Slope::Slope_12 }, highCutSlope { Slope::Slope_12 };
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

using Filter = juce::dsp::IIR::Filter<float>;
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, Filter, Filter, Filter, Filter, CutFilter>;

enum ChainPositions
{
    LowCut,
    LowShelf,
    Peak1,
    Peak2,
    Peak3,
    HighShelf,
    HighCut
};
using Coefficients = Filter::CoefficientsPtr;

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate, int peakIndex);
Coefficients makeLowShelfFilter(const ChainSettings& chainSettings, double sampleRate);
Coefficients makeHighShelfFilter(const ChainSettings& chainSettings, double sampleRate);

void updateCoefficients(Coefficients& old, const Coefficients& replacements);

template<int Index, typename ChainType>
void update(ChainType& chain, const Coefficients& coefficients)
{
    auto& filter = chain.template get<Index>();
    updateCoefficients(filter.coefficients, coefficients);
    chain.template setBypassed<Index>(false);
}

template<typename ChainType>
void updateCutFilter(ChainType& chain, const Coefficients& coefficients, Slope slope)
{
    chain.template setBypassed<0>(true);
    chain.template setBypassed<1>(true);
    chain.template setBypassed<2>(true);
    chain.template setBypassed<3>(true);

    switch (slope)
    {
        case Slope_48:
            update<3>(chain, coefficients);
            [[fallthrough]];
        case Slope_36:
            update<2>(chain, coefficients);
            [[fallthrough]];
        case Slope_24:
            update<1>(chain, coefficients);
            [[fallthrough]];
        case Slope_12:
            update<0>(chain, coefficients);
            break;
    }
}

inline auto makeLowCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    auto coefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
        chainSettings.lowCutFreq, sampleRate, 2 * (chainSettings.lowCutSlope + 1));
    return coefficients[0];
}

inline auto makeHighCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    auto coefficients =  juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
        chainSettings.highCutFreq, sampleRate, 2 * (chainSettings.highCutSlope + 1));
    return coefficients[0];

}

class EQoonAudioProcessor  : public juce::AudioProcessor
{
public:
    EQoonAudioProcessor();
    ~EQoonAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};

private:
    MonoChain leftChain, rightChain;

    void updatePeakFilters(const ChainSettings& chainSettings);
    void updateLowShelfFilter(const ChainSettings& chainSettings);
    void updateHighShelfFilter(const ChainSettings& chainSettings);
    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);
    void updateFilters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQoonAudioProcessor)
};
