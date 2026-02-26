#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace RefBoxColours
{
    // Backgrounds - slightly warmer dark tones
    const juce::Colour bg           (0xFF0D0D10);
    const juce::Colour panelBg      (0xFF151519);
    const juce::Colour panelBorder  (0xFF222228);
    const juce::Colour slotBg       (0xFF19191D);
    const juce::Colour slotActive   (0xFF1E1E2A);

    // Accents
    const juce::Colour accent       (0xFF00B4D8);  // cyan-blue
    const juce::Colour accentDim    (0xFF006880);
    const juce::Colour reference    (0xFFFFAA00);  // warm amber
    const juce::Colour referenceDim (0xFF886600);

    // Semantic
    const juce::Colour good         (0xFF00CC66);
    const juce::Colour warning      (0xFFFFAA00);
    const juce::Colour danger       (0xFFFF3344);
    const juce::Colour abOn         (0xFF00D4FF);
    const juce::Colour abOff        (0xFF2A2A34);

    // Text
    const juce::Colour textPrimary  (0xFFE0E0E4);
    const juce::Colour textSecondary(0xFF808088);
    const juce::Colour textDim      (0xFF505058);

    // Meters
    const juce::Colour meterGreen   (0xFF00CC66);
    const juce::Colour meterYellow  (0xFFDDAA00);
    const juce::Colour meterRed     (0xFFFF3344);
    const juce::Colour meterBg      (0xFF0E0E12);
}

class RefBoxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RefBoxLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, RefBoxColours::bg);
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF1A1A22));
        setColour (juce::TextButton::textColourOffId, RefBoxColours::textPrimary);
        setColour (juce::TextButton::textColourOnId, RefBoxColours::accent);
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF1A1A22));
        setColour (juce::ComboBox::textColourId, RefBoxColours::textPrimary);
        setColour (juce::ComboBox::outlineColourId, RefBoxColours::panelBorder);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xFF1A1A20));
        setColour (juce::PopupMenu::textColourId, RefBoxColours::textPrimary);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, RefBoxColours::accent.withAlpha (0.15f));
        setColour (juce::ListBox::backgroundColourId, RefBoxColours::panelBg);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        float cornerSize = 4.0f;

        auto baseColour = backgroundColour;
        if (shouldDrawButtonAsDown)
            baseColour = baseColour.brighter (0.12f);
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter (0.06f);

        // Metallic gradient on buttons
        juce::ColourGradient btnGrad (
            baseColour.brighter (0.04f), bounds.getX(), bounds.getY(),
            baseColour.darker (0.02f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (btnGrad);
        g.fillRoundedRectangle (bounds, cornerSize);

        // Top highlight line
        g.setColour (juce::Colours::white.withAlpha (0.03f));
        g.fillRect (bounds.getX() + 1.0f, bounds.getY() + 1.0f, bounds.getWidth() - 2.0f, 1.0f);

        g.setColour (RefBoxColours::panelBorder);
        g.drawRoundedRectangle (bounds, cornerSize, 1.0f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool, bool) override
    {
        auto font = juce::FontOptions (button.getHeight() * 0.42f);
        g.setFont (font);
        g.setColour (button.findColour (button.getToggleState()
                                            ? juce::TextButton::textColourOnId
                                            : juce::TextButton::textColourOffId));

        g.drawText (button.getButtonText(), button.getLocalBounds(),
                    juce::Justification::centred, false);
    }
};
