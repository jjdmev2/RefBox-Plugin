#include "MeterPanel.h"
#include <cmath>

MeterPanel::MeterPanel (LUFSMeter& mix, LUFSMeter& ref, EQMatcher& eq)
    : mixMeter (mix), refMeter (ref), eqMatcher (eq)
{
    startTimerHz (15);
}

void MeterPanel::timerCallback()
{
    mixMomentary = mixMeter.getMomentaryLUFS();
    mixShortTerm = mixMeter.getShortTermLUFS();
    mixIntegrated = mixMeter.getIntegratedLUFS();
    mixTruePeak = mixMeter.getTruePeak();
    refMomentary = refMeter.getMomentaryLUFS();
    refShortTerm = refMeter.getShortTermLUFS();
    refIntegrated = refMeter.getIntegratedLUFS();
    matchScore = eqMatcher.getMatchScore();
    repaint();
}

void MeterPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (RefBoxColours::panelBg);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (RefBoxColours::panelBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    auto area = bounds.reduced (6.0f);

    // Title
    g.setColour (RefBoxColours::textSecondary);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("LOUDNESS", area.removeFromTop (16), juce::Justification::centred);

    area.removeFromTop (4);

    // Split into MIX (left) and REF (right) columns
    float halfW = area.getWidth() * 0.5f;
    auto mixArea = area.removeFromLeft (halfW).reduced (2, 0);
    auto refArea = area.reduced (2, 0);

    // Column headers
    g.setFont (juce::FontOptions (9.0f));
    g.setColour (RefBoxColours::accent);
    g.drawText ("MIX", mixArea.removeFromTop (14), juce::Justification::centred);
    g.setColour (RefBoxColours::reference);
    g.drawText ("REF", refArea.removeFromTop (14), juce::Justification::centred);

    mixArea.removeFromTop (2);
    refArea.removeFromTop (2);

    // Vertical meters - take the main space
    float meterH = std::min (mixArea.getHeight() * 0.55f, 280.0f);
    auto mixMeterArea = mixArea.removeFromTop (meterH);
    auto refMeterArea = refArea.removeFromTop (meterH);

    // Draw the two main vertical meters side by side
    drawVerticalMeter (g, mixMeterArea.reduced (4, 0), mixMomentary, -60.0f, 0.0f, "M", RefBoxColours::accent);
    drawVerticalMeter (g, refMeterArea.reduced (4, 0), refMomentary, -60.0f, 0.0f, "M", RefBoxColours::reference);

    mixArea.removeFromTop (4);
    refArea.removeFromTop (4);

    // LUFS readouts below meters
    float readoutH = 16.0f;
    drawLufsValue (g, mixArea.removeFromTop (readoutH), "M", mixMomentary, RefBoxColours::accent);
    drawLufsValue (g, refArea.removeFromTop (readoutH), "M", refMomentary, RefBoxColours::reference);
    drawLufsValue (g, mixArea.removeFromTop (readoutH), "S", mixShortTerm, RefBoxColours::accent);
    drawLufsValue (g, refArea.removeFromTop (readoutH), "S", refShortTerm, RefBoxColours::reference);
    drawLufsValue (g, mixArea.removeFromTop (readoutH), "I", mixIntegrated, RefBoxColours::accent);
    drawLufsValue (g, refArea.removeFromTop (readoutH), "I", refIntegrated, RefBoxColours::reference);

    // True peak
    float tpDb = (mixTruePeak > 0.0f) ? 20.0f * std::log10 (mixTruePeak) : -100.0f;
    juce::Colour tpCol = (tpDb > -1.0f) ? RefBoxColours::danger
                        : (tpDb > -3.0f) ? RefBoxColours::warning
                        : RefBoxColours::good;
    drawLufsValue (g, mixArea.removeFromTop (readoutH), "TP", tpDb, tpCol);

    mixArea.removeFromTop (6);
    refArea.removeFromTop (6 + readoutH);

    // Delta
    if (mixIntegrated > -100.0f && refIntegrated > -100.0f)
    {
        float delta = mixIntegrated - refIntegrated;
        juce::Colour deltaCol = (std::abs (delta) < 1.0f) ? RefBoxColours::good
                              : (std::abs (delta) < 3.0f) ? RefBoxColours::warning
                              : RefBoxColours::danger;

        auto fullWidth = bounds.reduced (6).removeFromBottom (bounds.getHeight() * 0.25f);
        fullWidth = fullWidth.removeFromTop (18);

        g.setColour (deltaCol);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (juce::String::formatted ("%+.1f LUFS", delta),
                    fullWidth, juce::Justification::centred);
    }

    // Match score at bottom
    auto scoreArea = bounds.reduced (6).removeFromBottom (bounds.getHeight() * 0.15f);
    g.setColour (RefBoxColours::textDim);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("MATCH", scoreArea.removeFromTop (12), juce::Justification::centred);

    juce::Colour scoreCol = (matchScore >= 80) ? RefBoxColours::good
                           : (matchScore >= 50) ? RefBoxColours::warning
                           : RefBoxColours::danger;
    g.setColour (scoreCol);
    g.setFont (juce::FontOptions (18.0f));
    g.drawText (juce::String ((int) matchScore), scoreArea, juce::Justification::centred);
}

void MeterPanel::resized() {}

void MeterPanel::drawVerticalMeter (juce::Graphics& g, juce::Rectangle<float> area,
                                     float value, float minVal, float maxVal,
                                     const juce::String& /*label*/, juce::Colour colour)
{
    // Background
    g.setColour (RefBoxColours::meterBg);
    g.fillRoundedRectangle (area, 3.0f);

    float normalized = juce::jlimit (0.0f, 1.0f, (value - minVal) / (maxVal - minVal));

    if (normalized > 0.001f)
    {
        float segH = std::max (2.0f, area.getHeight() / 40.0f);
        float segGap = 1.0f;
        float fillHeight = normalized * area.getHeight();
        float fillTop = area.getBottom() - fillHeight;
        float inset = 1.5f;

        // Draw segments from bottom up to the fill level
        for (float y = area.getBottom() - segH - segGap;
             y >= fillTop && y >= area.getY();
             y -= (segH + segGap))
        {
            // normPos: 0 = bottom, 1 = top
            float normPos = 1.0f - (y - area.getY()) / area.getHeight();

            juce::Colour segCol;
            if (normPos > 0.92f)
                segCol = RefBoxColours::meterRed;
            else if (normPos > 0.75f)
                segCol = RefBoxColours::meterYellow;
            else
                segCol = colour.interpolatedWith (RefBoxColours::meterGreen, 0.4f);

            g.setColour (segCol);
            g.fillRect (area.getX() + inset, y, area.getWidth() - inset * 2.0f, segH);
        }
    }

    // Border
    g.setColour (RefBoxColours::panelBorder);
    g.drawRoundedRectangle (area, 3.0f, 1.0f);
}

void MeterPanel::drawLufsValue (juce::Graphics& g, juce::Rectangle<float> area,
                                 const juce::String& label, float value, juce::Colour colour)
{
    auto lblArea = area.removeFromLeft (16);
    g.setColour (RefBoxColours::textDim);
    g.setFont (juce::FontOptions (8.0f));
    g.drawText (label, lblArea, juce::Justification::centred);

    g.setColour (colour);
    g.setFont (juce::FontOptions (10.0f));
    if (value > -100.0f)
        g.drawText (juce::String::formatted ("%.1f", value), area, juce::Justification::centredRight);
    else
        g.drawText ("-inf", area, juce::Justification::centredRight);
}
