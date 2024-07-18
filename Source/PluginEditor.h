#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                                        juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
    }
};

struct ResponseCurveComponent : juce::Component,
                                juce::AudioProcessorParameter::Listener,
                                juce::Timer
{
    ResponseCurveComponent(EQoonAudioProcessor&);
    ~ResponseCurveComponent();

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
    void timerCallback() override;
    void paint(juce::Graphics& g) override;

private:
    EQoonAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged{false};
    MonoChain monoChain;
};

class EQoonAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    EQoonAudioProcessorEditor(EQoonAudioProcessor&);
    ~EQoonAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    EQoonAudioProcessor& audioProcessor;
    CustomRotarySlider lowCutFreqSlider, lowCutSlopeSlider, lowCutQualitySlider;
    CustomRotarySlider lowShelfFreqSlider, lowShelfGainSlider, lowShelfQualitySlider;
    CustomRotarySlider peakFreq1Slider, peakGain1Slider, peakQuality1Slider;
    CustomRotarySlider peakFreq2Slider, peakGain2Slider, peakQuality2Slider;
    CustomRotarySlider peakFreq3Slider, peakGain3Slider, peakQuality3Slider;
    CustomRotarySlider highShelfFreqSlider, highShelfGainSlider, highShelfQualitySlider;
    CustomRotarySlider highCutFreqSlider, highCutSlopeSlider, highCutQualitySlider;
    ResponseCurveComponent responseCurveComponent;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    Attachment lowCutFreqSliderAttachment, lowCutSlopeSliderAttachment, lowCutQualitySliderAttachment;
    Attachment lowShelfFreqSliderAttachment, lowShelfGainSliderAttachment, lowShelfQualitySliderAttachment;
    Attachment peakFreqSlider1Attachment, peakGainSlider1Attachment, peakQualitySlider1Attachment;
    Attachment peakFreqSlider2Attachment, peakGainSlider2Attachment, peakQualitySlider2Attachment;
    Attachment peakFreqSlider3Attachment, peakGainSlider3Attachment, peakQualitySlider3Attachment;
    Attachment highShelfFreqSliderAttachment, highShelfGainSliderAttachment, highShelfQualitySliderAttachment;
    Attachment highCutFreqSliderAttachment, highCutSlopeSliderAttachment, highCutQualitySliderAttachment;

    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQoonAudioProcessorEditor)
};

