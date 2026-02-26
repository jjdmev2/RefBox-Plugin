#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "RefBoxLookAndFeel.h"

class RefBoxProcessor;

class ABButton : public juce::Component, public juce::Timer
{
public:
    ABButton (RefBoxProcessor& processor);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseEnter (const juce::MouseEvent&) override { hovering = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hovering = false; repaint(); }
    void timerCallback() override;

private:
    RefBoxProcessor& processor;
    bool hovering = false;
    float glowPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ABButton)
};
