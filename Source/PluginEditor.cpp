#include "PluginProcessor.h"
#include "PluginEditor.h"

ResponseCurveComponent::ResponseCurveComponent(EQoonAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }
    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        auto chainSettings = getChainSettings(audioProcessor.apvts);

        auto peakCoefficients1 = makePeakFilter(chainSettings, audioProcessor.getSampleRate(), 1);
        auto peakCoefficients2 = makePeakFilter(chainSettings, audioProcessor.getSampleRate(), 2);
        auto peakCoefficients3 = makePeakFilter(chainSettings, audioProcessor.getSampleRate(), 3);

        updateCoefficients(monoChain.get<ChainPositions::Peak1>().coefficients, peakCoefficients1);
        updateCoefficients(monoChain.get<ChainPositions::Peak2>().coefficients, peakCoefficients2);
        updateCoefficients(monoChain.get<ChainPositions::Peak3>().coefficients, peakCoefficients3);

        auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
        auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
        
        updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
        updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
        
        auto lowShelfCoefficients = makeLowShelfFilter(chainSettings, audioProcessor.getSampleRate());
        auto highShelfCoefficients = makeHighShelfFilter(chainSettings, audioProcessor.getSampleRate());


        updateCoefficients(monoChain.get<ChainPositions::LowShelf>().coefficients, lowShelfCoefficients);
        updateCoefficients(monoChain.get<ChainPositions::HighShelf>().coefficients, highShelfCoefficients);

        repaint();
    }
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    using namespace juce;
    g.fillAll(Colours::black);

    auto responseArea = getLocalBounds();
    auto w = responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& lowShelf = monoChain.get<ChainPositions::LowShelf>();
    auto& peak1 = monoChain.get<ChainPositions::Peak1>();
    auto& peak2 = monoChain.get<ChainPositions::Peak2>();
    auto& peak3 = monoChain.get<ChainPositions::Peak3>();
    auto& highShelf = monoChain.get<ChainPositions::HighShelf>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();
    std::vector<double> mags;
    mags.resize(w);

    for (int i = 0; i < w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

        if (!monoChain.isBypassed<ChainPositions::Peak1>())
            mag *= peak1.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!monoChain.isBypassed<ChainPositions::Peak2>())
            mag *= peak2.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!monoChain.isBypassed<ChainPositions::Peak3>())
            mag *= peak3.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!monoChain.isBypassed<ChainPositions::LowShelf>())
            mag *= lowShelf.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!monoChain.isBypassed<ChainPositions::HighShelf>())
            mag *= highShelf.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!lowcut.isBypassed<0>())
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<1>())
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<2>())
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<3>())
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!highcut.isBypassed<0>())
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<1>())
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<2>())
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<3>())
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        mags[i] = Decibels::gainToDecibels(mag);
    }

    Path responseCurve;
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    for (size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }

    g.setColour(Colours::aqua);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

EQoonAudioProcessorEditor::EQoonAudioProcessorEditor(EQoonAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      responseCurveComponent(audioProcessor),
        lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
        lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
        lowCutQualitySliderAttachment(audioProcessor.apvts, "LowCut Quality", lowCutQualitySlider),
        lowShelfFreqSliderAttachment(audioProcessor.apvts, "LowShelf Freq", lowShelfFreqSlider),
        lowShelfGainSliderAttachment(audioProcessor.apvts, "LowShelf Gain", lowShelfGainSlider),
        lowShelfQualitySliderAttachment(audioProcessor.apvts, "LowShelf Quality", lowShelfQualitySlider),
        peakFreqSlider1Attachment(audioProcessor.apvts, "Peak1 Freq", peakFreq1Slider),
        peakGainSlider1Attachment(audioProcessor.apvts, "Peak1 Gain", peakGain1Slider),
        peakQualitySlider1Attachment(audioProcessor.apvts, "Peak1 Quality", peakQuality1Slider),
        peakFreqSlider2Attachment(audioProcessor.apvts, "Peak2 Freq", peakFreq2Slider),
        peakGainSlider2Attachment(audioProcessor.apvts, "Peak2 Gain", peakGain2Slider),
        peakQualitySlider2Attachment(audioProcessor.apvts, "Peak2 Quality", peakQuality2Slider),
        peakFreqSlider3Attachment(audioProcessor.apvts, "Peak3 Freq", peakFreq3Slider),
        peakGainSlider3Attachment(audioProcessor.apvts, "Peak3 Gain", peakGain3Slider),
        peakQualitySlider3Attachment(audioProcessor.apvts, "Peak3 Quality", peakQuality3Slider),
        highShelfFreqSliderAttachment(audioProcessor.apvts, "HighShelf Freq", highShelfFreqSlider),
        highShelfGainSliderAttachment(audioProcessor.apvts, "HighShelf Gain", highShelfGainSlider),
        highShelfQualitySliderAttachment(audioProcessor.apvts, "HighShelf Quality", highShelfQualitySlider),
        highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
        highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),
          highCutQualitySliderAttachment(audioProcessor.apvts, "HighCut Quality", highCutQualitySlider)
{
    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
    setSize(1000, 600);
}

EQoonAudioProcessorEditor::~EQoonAudioProcessorEditor()
{
}

void EQoonAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void EQoonAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.5);
    responseCurveComponent.setBounds(responseArea);
    
    auto lowCutArea = bounds.removeFromTop(bounds.getHeight() * 0.143);
    auto lowShelfArea = bounds.removeFromTop(bounds.getHeight() * 0.167);
    auto peak1Area = bounds.removeFromTop(bounds.getHeight() * 0.2);
    auto peak2Area = bounds.removeFromTop(bounds.getHeight() * 0.25);
    auto peak3Area = bounds.removeFromTop(bounds.getHeight() * 0.33);
    auto highShelfArea = bounds.removeFromTop(bounds.getHeight() * 0.5);
    auto highCutArea = bounds;
    
    lowCutFreqSlider.setBounds(lowCutArea.removeFromLeft(lowCutArea.getWidth() * 0.33));
    lowCutSlopeSlider.setBounds(lowCutArea.removeFromLeft(lowCutArea.getWidth() * 0.5));
    lowCutQualitySlider.setBounds(lowCutArea);
    
    lowShelfFreqSlider.setBounds(lowShelfArea.removeFromLeft(lowShelfArea.getWidth() * 0.33));
    lowShelfGainSlider.setBounds(lowShelfArea.removeFromLeft(lowShelfArea.getWidth() * 0.5));
    lowShelfQualitySlider.setBounds(lowShelfArea);
    
    peakFreq1Slider.setBounds(peak1Area.removeFromLeft(peak1Area.getWidth() * 0.33));
    peakGain1Slider.setBounds(peak1Area.removeFromLeft(peak1Area.getWidth() * 0.5));
    peakQuality1Slider.setBounds(peak1Area);
    
    peakFreq2Slider.setBounds(peak2Area.removeFromLeft(peak2Area.getWidth() * 0.33));
    peakGain2Slider.setBounds(peak2Area.removeFromLeft(peak2Area.getWidth() * 0.5));
    peakQuality2Slider.setBounds(peak2Area);
    
    peakFreq3Slider.setBounds(peak3Area.removeFromLeft(peak3Area.getWidth() * 0.33));
    peakGain3Slider.setBounds(peak3Area.removeFromLeft(peak3Area.getWidth() * 0.5));
    peakQuality3Slider.setBounds(peak3Area);
    
    highShelfFreqSlider.setBounds(highShelfArea.removeFromLeft(highShelfArea.getWidth() * 0.33));
    highShelfGainSlider.setBounds(highShelfArea.removeFromLeft(highShelfArea.getWidth() * 0.5));
    highShelfQualitySlider.setBounds(highShelfArea);
    
    highCutFreqSlider.setBounds(highCutArea.removeFromLeft(highCutArea.getWidth() * 0.33));
    highCutSlopeSlider.setBounds(highCutArea.removeFromLeft(highCutArea.getWidth() * 0.5));
    highCutQualitySlider.setBounds(highCutArea);
}


std::vector<juce::Component*> EQoonAudioProcessorEditor::getComps()
{
    return {
        &peakFreq1Slider, &peakGain1Slider, &peakQuality1Slider,
        &peakFreq2Slider, &peakGain2Slider, &peakQuality2Slider,
        &peakFreq3Slider, &peakGain3Slider, &peakQuality3Slider,
        &lowCutFreqSlider, &highCutFreqSlider, &lowCutSlopeSlider, &highCutSlopeSlider,
        &lowShelfFreqSlider, &lowShelfGainSlider, &lowShelfQualitySlider,
        &highShelfFreqSlider, &highShelfGainSlider, &highShelfQualitySlider,
        &lowCutQualitySlider, &highCutQualitySlider,
        &responseCurveComponent
    };
}
