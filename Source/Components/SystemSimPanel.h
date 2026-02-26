#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/SystemSimulator.h"
#include "RefBoxLookAndFeel.h"

class SystemSimPanel : public juce::Component, public juce::Timer
{
public:
    SystemSimPanel (SystemSimulator& simulator);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    SystemSimulator& simulator;

    struct SimButton : public juce::Component
    {
        SimButton (SystemSimPanel& owner, int sysIndex, const juce::String& name);
        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent&) override;

        SystemSimPanel& owner;
        int systemIndex;
        juce::String systemName;
        bool active = false;

        void drawIcon (juce::Graphics& g, juce::Rectangle<float> area);
    };

    std::vector<std::unique_ptr<SimButton>> simButtons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SystemSimPanel)
};
