#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Components/RefBoxLookAndFeel.h"
#include "Components/SpectrumDisplay.h"
#include "Components/WaveformDisplay.h"
#include "Components/WaveformMicroscope.h"
#include "Components/ReferenceGrid.h"
#include "Components/SystemSimPanel.h"
#include "Components/MeterPanel.h"
#include "Components/TopBar.h"
#include "Components/ABButton.h"

class RefBoxEditor : public juce::AudioProcessorEditor
{
public:
    explicit RefBoxEditor (RefBoxProcessor&);
    ~RefBoxEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    RefBoxProcessor& refBoxProcessor;
    RefBoxLookAndFeel lookAndFeel;

    SpectrumDisplay spectrumDisplay;
    TopBar topBar;
    WaveformDisplay waveformDisplay;
    WaveformMicroscope waveformMicroscope;
    ReferenceGrid referenceGrid;
    SystemSimPanel systemSimPanel;
    MeterPanel meterPanel;
    ABButton abButton;

    static constexpr int defaultWidth = 1100;
    static constexpr int defaultHeight = 720;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RefBoxEditor)
};
