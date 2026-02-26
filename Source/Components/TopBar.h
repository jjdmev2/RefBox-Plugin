#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "RefBoxLookAndFeel.h"

class RefBoxProcessor;
class SpectrumDisplay;

class TopBar : public juce::Component, public juce::Timer
{
public:
    TopBar (RefBoxProcessor& processor, SpectrumDisplay& spectrumDisplay);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown (const juce::MouseEvent& e) override;

private:
    RefBoxProcessor& processor;
    SpectrumDisplay& spectrumDisplay;

    // Display toggles
    juce::TextButton refCurveBtn { "Ref Curve" };
    juce::TextButton deltaBtn { "Delta" };
    juce::TextButton sysCurveBtn { "Sys Curve" };
    juce::TextButton lufsMatchBtn { "LUFS" };
    juce::TextButton playRefBtn { "PLAY" };
    juce::TextButton resetBtn { "RST" };

    // Spectrum mode selector
    juce::ComboBox spectrumModeSelector;

    // Genre preset
    juce::ComboBox genreSelector;

    // Color scheme
    juce::ComboBox colorSchemeSelector;

    bool showRef = true, showDelta = false, showSys = false;

    void styleToggle (juce::TextButton& btn, bool active);

    // Easter egg
    int logoClickCount = 0;
    juce::uint32 lastLogoClickTime = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopBar)
};
