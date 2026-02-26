#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/LUFSMeter.h"
#include "../DSP/EQMatcher.h"
#include "RefBoxLookAndFeel.h"

class MeterPanel : public juce::Component, public juce::Timer
{
public:
    MeterPanel (LUFSMeter& mixMeter, LUFSMeter& refMeter, EQMatcher& eqMatcher);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    LUFSMeter& mixMeter;
    LUFSMeter& refMeter;
    EQMatcher& eqMatcher;

    float mixMomentary = -100.0f, mixShortTerm = -100.0f, mixIntegrated = -100.0f, mixTruePeak = -100.0f;
    float refMomentary = -100.0f, refShortTerm = -100.0f, refIntegrated = -100.0f;
    float matchScore = 0.0f;

    void drawVerticalMeter (juce::Graphics& g, juce::Rectangle<float> area,
                            float value, float minVal, float maxVal,
                            const juce::String& label, juce::Colour colour);

    void drawLufsValue (juce::Graphics& g, juce::Rectangle<float> area,
                        const juce::String& label, float value, juce::Colour colour);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeterPanel)
};
