#include "ABButton.h"
#include "../PluginProcessor.h"

ABButton::ABButton (RefBoxProcessor& p) : processor (p)
{
    startTimerHz (30);
}

void ABButton::timerCallback()
{
    bool abOn = processor.isABActive();
    if (abOn)
    {
        glowPhase += 0.06f;
        if (glowPhase > juce::MathConstants<float>::twoPi)
            glowPhase -= juce::MathConstants<float>::twoPi;
    }
    repaint();
}

void ABButton::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float sz = std::min (bounds.getWidth(), bounds.getHeight());
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float radius = sz * 0.45f;

    bool abOn = processor.isABActive();

    // Outer glow
    if (abOn)
    {
        float pulse = 0.5f + 0.5f * std::sin (glowPhase);
        float glowAlpha = 0.04f + 0.03f * pulse;
        for (int r = 3; r >= 1; --r)
        {
            float expand = radius + (float) r * 8.0f;
            g.setColour (RefBoxColours::abOn.withAlpha (glowAlpha));
            g.fillEllipse (cx - expand, cy - expand, expand * 2.0f, expand * 2.0f);
        }
    }
    else if (hovering)
    {
        g.setColour (juce::Colour (0xFF2A2A38).withAlpha (0.5f));
        g.fillEllipse (cx - radius - 6, cy - radius - 6,
                       (radius + 6) * 2.0f, (radius + 6) * 2.0f);
    }

    // Main circle - metallic gradient
    auto circleRect = juce::Rectangle<float> (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    juce::ColourGradient metalGrad (
        abOn ? juce::Colour (0xFF003850) : juce::Colour (0xFF2A2A32),
        cx, cy - radius,
        abOn ? juce::Colour (0xFF001820) : juce::Colour (0xFF141418),
        cx, cy + radius, false);
    g.setGradientFill (metalGrad);
    g.fillEllipse (circleRect);

    // Inner highlight ring (metallic sheen)
    auto innerHighlight = circleRect.reduced (3.0f);
    juce::ColourGradient sheenGrad (
        abOn ? RefBoxColours::abOn.withAlpha (0.15f) : juce::Colour (0xFF404050).withAlpha (0.15f),
        cx, cy - radius * 0.7f,
        juce::Colours::transparentBlack,
        cx, cy + radius * 0.3f, false);
    g.setGradientFill (sheenGrad);
    g.fillEllipse (innerHighlight);

    // Border ring
    g.setColour (abOn ? RefBoxColours::abOn.withAlpha (0.7f) : juce::Colour (0xFF3A3A44));
    g.drawEllipse (circleRect, abOn ? 2.5f : 1.5f);

    // Inner ring
    g.setColour (abOn ? RefBoxColours::abOn.withAlpha (0.2f) : juce::Colour (0xFF252530));
    g.drawEllipse (circleRect.reduced (4.0f), 1.0f);

    // "A/B" text
    float fontSize = radius * 0.7f;
    g.setFont (juce::FontOptions (fontSize, juce::Font::bold));
    g.setColour (abOn ? RefBoxColours::abOn : RefBoxColours::textSecondary);
    g.drawText ("A/B", circleRect, juce::Justification::centred);

    // Status text below
    float smallFont = std::max (8.0f, radius * 0.25f);
    g.setFont (juce::FontOptions (smallFont));
    g.setColour (abOn ? RefBoxColours::abOn.withAlpha (0.5f) : RefBoxColours::textDim);
    auto statusRect = circleRect;
    statusRect.translate (0.0f, radius * 0.45f);
    g.drawText (abOn ? "REF" : "MIX", statusRect, juce::Justification::centred);
}

void ABButton::mouseDown (const juce::MouseEvent& e)
{
    auto bounds = getLocalBounds().toFloat();
    float sz = std::min (bounds.getWidth(), bounds.getHeight());
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float radius = sz * 0.45f;

    float dx = e.position.x - cx;
    float dy = e.position.y - cy;

    if (dx * dx + dy * dy <= radius * radius * 1.2f)
        processor.setABActive (! processor.isABActive());
}
