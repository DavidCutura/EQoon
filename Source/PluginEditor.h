/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

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

//==============================================================================
/**
*/
class EQoonAudioProcessorEditor  : public juce::AudioProcessorEditor,
juce::AudioProcessorParameter::Listener,
juce::Timer
{
public:
    EQoonAudioProcessorEditor (EQoonAudioProcessor&);
    ~EQoonAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIdnex, bool gestureIsStarting) override { } 
    
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    EQoonAudioProcessor& audioProcessor;
    
    juce::Atomic<bool> parametersChanged { false };
    
    CustomRotarySlider peakFreqSlider,
    peakGainSlider,
    peakQualitySlider,
    lowCutFreqSlider,
    highCutFreqSlider,
    lowCutSlopeSlider,
    highCutSlopeSlider;
    
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachement = APVTS::SliderAttachment;
    
    Attachement peakFreqSliderAttachement,
                peakGainSliderAttachement,
                peakQualitySliderAttachement,
                lowCutFreqSliderAttachement,
                highCutFreqSliderAttachement,
                lowCutSlopeSliderAttachement,
                highCutSlopeSliderAttachement;
     
    
    std::vector<juce::Component*> getComps();
    
    MonoChain monoChain;
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQoonAudioProcessorEditor)
};
