#include "SpectrumDisplay.h"
#include <cmath>

SpectrumDisplay::SpectrumDisplay (SpectralAnalyzer& mix, SpectralAnalyzer& ref,
                                  ReferenceCurveManager& curves, EQMatcher& eq,
                                  SystemSimulator& sys)
    : mixAnalyzer (mix), refAnalyzer (ref), curveManager (curves),
      eqMatcher (eq), systemSim (sys)
{
    peakHold.fill (-100.0f);
    peakHoldCounter.fill (0.0f);
    displayMagnitudes.fill (-100.0f);
    initDisplayWeighting();

    // Gain scaling buttons
    addAndMakeVisible (gainUpBtn);
    addAndMakeVisible (gainDownBtn);
    gainUpBtn.onClick = [this] {
        displayGainDb = juce::jlimit (-18.0f, 18.0f, displayGainDb + 3.0f);
        repaint();
    };
    gainDownBtn.onClick = [this] {
        displayGainDb = juce::jlimit (-18.0f, 18.0f, displayGainDb - 3.0f);
        repaint();
    };
    auto styleGainBtn = [] (juce::TextButton& btn) {
        btn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF1A1A22));
        btn.setColour (juce::TextButton::textColourOffId, RefBoxColours::textSecondary);
    };
    styleGainBtn (gainUpBtn);
    styleGainBtn (gainDownBtn);

    setMouseCursor (juce::MouseCursor::CrosshairCursor);
    startTimerHz (120);
}

void SpectrumDisplay::initDisplayWeighting()
{
    // 3.0 dB/octave slope compensation. Milder than SPAN's 4.5 dB/oct default,
    // which preserves more genre character and makes spectral imbalances
    // (lacking bass, harsh treble, muddy mids) easier to spot visually.
    // Pivot point at 1 kHz (0 dB).
    for (int i = 0; i < SpectralAnalyzer::numBands; ++i)
    {
        float freq = 20.0f * std::pow (1000.0f, (float) i / (float) SpectralAnalyzer::numBands);
        float octavesFrom1k = std::log2 (freq / 1000.0f);
        displayWeighting[(size_t) i] = 3.0f * octavesFrom1k;
    }
}

void SpectrumDisplay::setSpectrumMode (SpectrumMode newMode)
{
    spectrumMode = newMode;
    mixAnalyzer.setMode (newMode);
    refAnalyzer.setMode (newMode);
}

void SpectrumDisplay::timerCallback()
{
    auto& mags = mixAnalyzer.getBandMagnitudes();
    bool mixHasSignal = false;
    for (int i = 0; i < SpectralAnalyzer::numBands; ++i)
    {
        if (mags[(size_t) i] > -90.0f) { mixHasSignal = true; break; }
    }

    if (! mixHasSignal)
    {
        auto& refMags = refAnalyzer.getBandMagnitudes();
        bool refHasSignal = false;
        for (int i = 0; i < SpectralAnalyzer::numBands; ++i)
        {
            if (refMags[(size_t) i] > -90.0f) { refHasSignal = true; break; }
        }
        showRefSpectrum = refHasSignal;
    }
    else
    {
        showRefSpectrum = false;
    }

    auto& activeMags = showRefSpectrum ? refAnalyzer.getBandMagnitudes() : mags;

    // Compute target weighted values, then interpolate displayMagnitudes toward them
    // for butter-smooth 120fps motion between FFT updates (~10Hz)
    for (int i = 0; i < SpectralAnalyzer::numBands; ++i)
    {
        float target = activeMags[(size_t) i] + displayWeighting[(size_t) i] + displayGainDb;
        if (! displayMagsInitialized)
            displayMagnitudes[(size_t) i] = target;
        else
            displayMagnitudes[(size_t) i] += (target - displayMagnitudes[(size_t) i]) * displayLerp;
    }
    displayMagsInitialized = true;

    updatePeakHold (displayMagnitudes);

    // Kitten animation: organic wandering between ~100Hz and ~2kHz
    if (kittenModePtr != nullptr && *kittenModePtr)
    {
        kittenPhase += 0.007f;
        kittenBob += 0.06f;

        // Target-based horizontal movement with pauses
        if (kittenPauseFrames > 0.0f)
        {
            kittenPauseFrames -= 1.0f;
        }
        else if (std::abs (kittenCurrentOsc - kittenTargetOsc) < 0.015f)
        {
            // Reached target — pause for 0.5-2.5 seconds, then pick new destination
            kittenPauseFrames = 60.0f + 120.0f * (0.5f + 0.5f * std::sin (kittenPhase * 3.7f));
            kittenTargetOsc = 0.12f + 0.76f * (0.5f + 0.5f * std::sin (kittenPhase * 2.3f + 4.1f));
        }

        // Smooth glide toward target (slow, chill movement)
        kittenCurrentOsc += (kittenTargetOsc - kittenCurrentOsc) * 0.018f;
    }

    repaint();
}

void SpectrumDisplay::updatePeakHold (const std::array<float, SpectralAnalyzer::numBands>& magnitudes)
{
    for (int i = 0; i < SpectralAnalyzer::numBands; ++i)
    {
        float mag = magnitudes[(size_t) i];
        if (mag >= peakHold[(size_t) i])
        {
            peakHold[(size_t) i] = mag;
            peakHoldCounter[(size_t) i] = peakHoldTime;
        }
        else
        {
            if (peakHoldCounter[(size_t) i] > 0.0f)
                peakHoldCounter[(size_t) i] -= 1.0f;
            else
                peakHold[(size_t) i] = std::max (-100.0f, peakHold[(size_t) i] - peakFallRate);
        }
    }
}

void SpectrumDisplay::resized()
{
    int btnW = 22;
    int btnH = 18;
    int rightPad = 6;
    int topPad = 18;
    gainUpBtn.setBounds (getWidth() - btnW - rightPad, topPad, btnW, btnH);
    gainDownBtn.setBounds (getWidth() - btnW - rightPad, topPad + btnH + 2, btnW, btnH);
}

float SpectrumDisplay::dbToY (float db, float height) const
{
    float normalized = (db - minDb) / dbRange;
    return height * (1.0f - juce::jlimit (0.0f, 1.0f, normalized));
}

float SpectrumDisplay::bandToX (int band, float width) const
{
    float t = (float) band / (float) SpectralAnalyzer::numBands;
    return std::pow (t, 0.82f) * width;  // stretch bass region for better sub-bass accuracy
}

juce::Colour SpectrumDisplay::getBarColour (float normalizedY, int band) const
{
    switch (colorScheme)
    {
        case VisualizerColorScheme::PioneerRGB:
        {
            // Pioneer CDJ-style: color by frequency band
            // Red = bass (0-31, ~20-250Hz), Green = mids (32-95, ~250Hz-5kHz), Blue = highs (96-127, ~5k-20kHz)
            float pos = (float) band / (float) SpectralAnalyzer::numBands;
            float brightness = 0.55f + normalizedY * 0.45f;

            if (pos < 0.12f)
            {
                // Sub bass: dark red
                return juce::Colour::fromFloatRGBA (0.55f * brightness, 0.0f, 0.02f * brightness, 1.0f);
            }
            else if (pos < 0.25f)
            {
                // Bass: red
                float t = (pos - 0.12f) / 0.13f;
                return juce::Colour::fromFloatRGBA (
                    (0.55f + t * 0.45f) * brightness, t * 0.1f * brightness, 0.0f, 1.0f);
            }
            else if (pos < 0.35f)
            {
                // Low mid transition: red -> green
                float t = (pos - 0.25f) / 0.10f;
                return juce::Colour::fromFloatRGBA (
                    (1.0f - t * 0.6f) * brightness, (0.1f + t * 0.7f) * brightness, 0.0f, 1.0f);
            }
            else if (pos < 0.55f)
            {
                // Mids: green with yellow tint
                float t = (pos - 0.35f) / 0.20f;
                return juce::Colour::fromFloatRGBA (
                    (0.4f - t * 0.35f) * brightness, (0.8f + t * 0.2f) * brightness, 0.0f, 1.0f);
            }
            else if (pos < 0.72f)
            {
                // Upper mid: green
                float t = (pos - 0.55f) / 0.17f;
                return juce::Colour::fromFloatRGBA (
                    0.05f * brightness, (1.0f - t * 0.15f) * brightness, t * 0.3f * brightness, 1.0f);
            }
            else if (pos < 0.82f)
            {
                // Transition: green -> blue
                float t = (pos - 0.72f) / 0.10f;
                return juce::Colour::fromFloatRGBA (
                    0.0f, (0.85f - t * 0.7f) * brightness, (0.3f + t * 0.7f) * brightness, 1.0f);
            }
            else
            {
                // Highs: blue to light blue/cyan
                float t = (pos - 0.82f) / 0.18f;
                return juce::Colour::fromFloatRGBA (
                    t * 0.2f * brightness, (0.15f + t * 0.45f) * brightness, brightness, 1.0f);
            }
        }

        case VisualizerColorScheme::Amber:
        {
            // Warm amber/gold gradient by height
            float t = normalizedY;
            return juce::Colour::fromFloatRGBA (
                0.6f + t * 0.4f, 0.3f + t * 0.35f, t * 0.08f, 1.0f);
        }

        case VisualizerColorScheme::Purple:
        {
            // Purple/violet gradient by height
            float t = normalizedY;
            return juce::Colour::fromFloatRGBA (
                0.3f + t * 0.5f, 0.05f + t * 0.15f, 0.5f + t * 0.35f, 1.0f);
        }

        case VisualizerColorScheme::Fire:
        {
            // Yellow base to orange to deep red by height
            if (normalizedY < 0.5f)
            {
                float t = normalizedY / 0.5f;
                return juce::Colour::fromFloatRGBA (
                    0.9f + t * 0.1f, 0.7f - t * 0.2f, 0.0f, 1.0f);
            }
            else
            {
                float t = (normalizedY - 0.5f) / 0.5f;
                return juce::Colour::fromFloatRGBA (
                    1.0f, 0.5f - t * 0.4f, t * 0.05f, 1.0f);
            }
        }

        case VisualizerColorScheme::Classic:
        default:
        {
            // Original green/cyan to yellow to red
            if (normalizedY < 0.45f)
            {
                float t = normalizedY / 0.45f;
                return juce::Colour::fromRGB (
                    (uint8_t) (t * 180), (uint8_t) (160 + t * 60), (uint8_t) (60 * (1.0f - t)));
            }
            else if (normalizedY < 0.78f)
            {
                float t = (normalizedY - 0.45f) / 0.33f;
                return juce::Colour::fromRGB (
                    (uint8_t) (180 + t * 75), (uint8_t) (220 - t * 80), (uint8_t) 0);
            }
            else
            {
                float t = (normalizedY - 0.78f) / 0.22f;
                return juce::Colour::fromRGB (255, (uint8_t) (140 * (1.0f - t)), (uint8_t) (40 * (1.0f - t)));
            }
        }
    }
}

// --- Mouse: shift+drag to move reference curve ---

void SpectrumDisplay::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isShiftDown() && showReference && curveManager.getActiveCurve() != nullptr)
    {
        isDraggingCurve = true;
        dragStartY = e.position.y;
        dragStartOffsetDb = refCurveOffsetDb;
    }
}

void SpectrumDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (isDraggingCurve)
    {
        float plotH = (float) getHeight() * 0.9f;
        float deltaPixels = dragStartY - e.position.y;
        float deltaDbs = (deltaPixels / plotH) * dbRange;
        refCurveOffsetDb = juce::jlimit (-30.0f, 30.0f, dragStartOffsetDb + deltaDbs);
        repaint();
    }
}

void SpectrumDisplay::mouseUp (const juce::MouseEvent&)
{
    isDraggingCurve = false;
}

void SpectrumDisplay::mouseMove (const juce::MouseEvent& e)
{
    hoverPos = e.position;
    mouseInside = true;
    repaint();
}

void SpectrumDisplay::mouseExit (const juce::MouseEvent&)
{
    mouseInside = false;
    repaint();
}

void SpectrumDisplay::drawPixelKitten (juce::Graphics& g, float x, float waveTopY, float scale) const
{
    // Chunky pixel cat loaf (16 wide x 12 tall)
    // Round orange chonk with stripes, no surfboard
    // 0=transparent 1=black(outline/features) 2=orange 3=dark_orange(stripes) 4=pink(nose) 5=cream(cheeks)
    static constexpr int spriteW = 16;
    static constexpr int spriteH = 12;
    static constexpr uint8_t sprite[spriteH][spriteW] = {
        { 0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0 },  // ear tips
        { 0,0,1,2,2,1,0,0,0,0,1,2,2,1,0,0 },  // ears
        { 0,1,2,2,2,2,1,1,1,1,2,2,2,2,1,0 },  // top of head
        { 1,2,3,2,3,2,3,2,2,3,2,3,2,3,2,1 },  // stripes on back
        { 1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1 },  // head/body
        { 1,2,2,1,1,2,2,2,2,2,2,1,1,2,2,1 },  // eyes
        { 1,2,2,2,2,2,2,4,2,2,2,2,2,2,2,1 },  // nose
        { 1,2,5,2,2,1,2,2,2,1,2,2,2,5,2,1 },  // mouth + cheeks
        { 1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1 },  // big round body
        { 1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1 },  // big round body
        { 0,1,2,1,1,2,2,2,2,2,2,1,1,2,1,0 },  // tiny paws
        { 0,0,1,0,0,1,1,1,1,1,1,0,0,1,0,0 },  // bottom
    };

    static const juce::Colour colors[] = {
        juce::Colour (0x00000000),  // 0: transparent
        juce::Colour (0xFF1A1A1A),  // 1: black outline
        juce::Colour (0xFFE8932A),  // 2: orange fur
        juce::Colour (0xFFCC7820),  // 3: dark orange stripes
        juce::Colour (0xFFFF6688),  // 4: pink nose
        juce::Colour (0xFFFFC870),  // 5: cream cheeks
    };

    float px = scale;
    float originX = x - (spriteW * px * 0.5f);
    float originY = waveTopY - (spriteH * px);

    for (int row = 0; row < spriteH; ++row)
    {
        for (int col = 0; col < spriteW; ++col)
        {
            uint8_t c = sprite[row][col];
            if (c == 0) continue;

            g.setColour (colors[c]);
            g.fillRect (originX + col * px, originY + row * px, px, px);
        }
    }
}

// --- Paint ---

void SpectrumDisplay::paint (juce::Graphics& g)
{
    if (isDraggingCurve && ! juce::ModifierKeys::currentModifiers.isShiftDown())
        isDraggingCurve = false;

    auto bounds = getLocalBounds().toFloat();

    // Metallic dark gradient
    juce::ColourGradient bgGrad (
        juce::Colour (0xFF161620), bounds.getX(), bounds.getY(),
        juce::Colour (0xFF0C0C10), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (bgGrad);
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (RefBoxColours::panelBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    // Top highlight
    g.setColour (juce::Colour (0xFF252535).withAlpha (0.3f));
    g.fillRect (bounds.getX() + 1.0f, bounds.getY() + 1.0f, bounds.getWidth() - 2.0f, 1.0f);

    float labelMarginL = getWidth() * 0.035f;
    float labelMarginR = 8.0f;
    float labelMarginT = 12.0f;
    float labelMarginB = getHeight() * 0.05f;

    auto plotArea = bounds.reduced (labelMarginL, labelMarginT);
    plotArea.removeFromBottom (labelMarginB);
    plotArea.removeFromRight (labelMarginR);

    drawGrid (g, plotArea);

    if (showSystemResponse && systemSim.getEnabled())
        drawSystemResponse (g, plotArea);

    if (showReference)
    {
        if (auto* curve = curveManager.getActiveCurve())
            drawReferenceCurve (g, plotArea, *curve);
    }

    drawBars (g, plotArea, displayMagnitudes);
    drawPeakHolders (g, plotArea);

    if (showDelta && curveManager.getActiveCurve() != nullptr)
        drawDelta (g, plotArea);

    drawLabels (g, plotArea);

    // Mode + Source indicator (top-right)
    g.setFont (juce::FontOptions (10.0f));

    const char* modeLabel = "AVG";
    switch (spectrumMode)
    {
        case SpectrumMode::Fast:    modeLabel = "FAST"; break;
        case SpectrumMode::Average: modeLabel = "AVG";  break;
        case SpectrumMode::Smooth:  modeLabel = "SLOW"; break;
        case SpectrumMode::Peak:    modeLabel = "PEAK"; break;
    }
    g.setColour (RefBoxColours::textDim);
    g.drawText (modeLabel,
                (int) (bounds.getRight() - 86), (int) (bounds.getY() + 3), 36, 14,
                juce::Justification::centredRight);

    g.setColour (showRefSpectrum ? RefBoxColours::reference : RefBoxColours::accent);
    g.drawText (showRefSpectrum ? "REF" : "MIX",
                (int) (bounds.getRight() - 46), (int) (bounds.getY() + 3), 40, 14,
                juce::Justification::centredRight);

    // Curve offset indicator
    if (std::abs (refCurveOffsetDb) > 0.1f)
    {
        g.setColour (RefBoxColours::reference.withAlpha (0.6f));
        g.setFont (juce::FontOptions (10.0f));
        g.drawText (juce::String::formatted ("Curve: %+.1f dB", refCurveOffsetDb),
                    (int) (bounds.getX() + 6), (int) (bounds.getY() + 3), 120, 14,
                    juce::Justification::centredLeft);
    }

    // Display gain indicator (next to +/- buttons)
    if (std::abs (displayGainDb) > 0.1f)
    {
        g.setColour (RefBoxColours::textSecondary);
        g.setFont (juce::FontOptions (9.0f));
        g.drawText (juce::String::formatted ("%+.0f dB", displayGainDb),
                    (int) (bounds.getRight() - 52), (int) (bounds.getY() + 56), 44, 12,
                    juce::Justification::centredRight);
    }

    // Hover frequency display
    if (mouseInside && hoverPos.x >= plotArea.getX() && hoverPos.x <= plotArea.getRight()
        && hoverPos.y >= plotArea.getY() && hoverPos.y <= plotArea.getBottom())
    {
        // Vertical crosshair line
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawVerticalLine ((int) hoverPos.x, plotArea.getY(), plotArea.getBottom());

        // Convert x position to frequency (inverse of bandToX power curve)
        float relX = (hoverPos.x - plotArea.getX()) / plotArea.getWidth();
        relX = juce::jlimit (0.0f, 1.0f, relX);
        float t = std::pow (relX, 1.0f / 0.82f);  // inverse of pow(t, 0.82)
        int band = (int) (t * (float) SpectralAnalyzer::numBands);
        band = juce::jlimit (0, SpectralAnalyzer::numBands - 1, band);
        float freq = mixAnalyzer.getBandFrequency (band);

        // Also compute dB at hover position
        float hoverDb = maxDb - ((hoverPos.y - plotArea.getY()) / plotArea.getHeight()) * dbRange;

        // Format frequency label
        juce::String freqStr;
        if (freq >= 1000.0f)
            freqStr = juce::String (freq / 1000.0f, 1) + " kHz";
        else
            freqStr = juce::String ((int) freq) + " Hz";

        juce::String dbStr = juce::String (hoverDb, 1) + " dB";
        juce::String label = freqStr + "  |  " + dbStr;

        // Draw label near cursor (offset to avoid overlap)
        g.setFont (juce::FontOptions (11.0f));
        int labelW = 140;
        int labelH = 18;
        float lx = hoverPos.x + 12.0f;
        float ly = hoverPos.y - 22.0f;

        // Keep label inside bounds
        if (lx + labelW > plotArea.getRight())
            lx = hoverPos.x - labelW - 8.0f;
        if (ly < plotArea.getY())
            ly = hoverPos.y + 8.0f;

        g.setColour (juce::Colour (0xDD101018));
        g.fillRoundedRectangle (lx - 4.0f, ly - 1.0f, (float) labelW + 8.0f, (float) labelH + 2.0f, 4.0f);
        g.setColour (RefBoxColours::accent);
        g.drawText (label, (int) lx, (int) ly, labelW, labelH, juce::Justification::centredLeft);
    }

    // Pixel kitten easter egg — wanders the spectral valley (~100Hz-2kHz)
    if (kittenModePtr != nullptr && *kittenModePtr)
    {
        // Map kittenCurrentOsc (0..1) to band range for 100Hz and 2kHz
        SpectralAnalyzer tmp;
        int lowBand = tmp.frequencyToBand (100.0f);
        int highBand = tmp.frequencyToBand (2000.0f);
        float bandPos = (float) lowBand + kittenCurrentOsc * (float) (highBand - lowBand);
        int kittenBand = juce::jlimit (0, SpectralAnalyzer::numBands - 1, (int) bandPos);

        // Use fractional band position for smooth X movement (matching bandToX power curve)
        float normalizedBand = bandPos / (float) SpectralAnalyzer::numBands;
        float kittenX = plotArea.getX() + std::pow (normalizedBand, 0.82f) * plotArea.getWidth();

        // Find the bar height — but smooth heavily so cat rides a gentle wave, not spikes
        float barDb = displayMagnitudes[(size_t) kittenBand];
        float targetY = plotArea.getY() + dbToY (barDb, plotArea.getHeight());

        if (kittenDisplayY < 0.0f)
            kittenDisplayY = targetY;  // initialize on first frame
        else
            kittenDisplayY += (targetY - kittenDisplayY) * 0.035f;  // very smooth vertical tracking

        // Gentle bobbing on top of the smoothed position
        float bob = std::sin (kittenBob) * 1.5f;
        float kittenY = kittenDisplayY + bob;

        // Scale proportional to plot area
        float kittenScale = std::max (2.0f, std::min (3.5f, plotArea.getHeight() / 80.0f));

        drawPixelKitten (g, kittenX, kittenY, kittenScale);
    }
}

void SpectrumDisplay::drawGrid (juce::Graphics& g, juce::Rectangle<float> area)
{
    for (float db = -60.0f; db <= 0.0f; db += 10.0f)
    {
        float y = area.getY() + dbToY (db, area.getHeight());
        g.setColour (db == 0.0f ? juce::Colour (0xFF2A2A30) : juce::Colour (0xFF131316));
        g.drawHorizontalLine ((int) y, area.getX(), area.getRight());
    }

    float freqs[] = { 20, 30, 40, 50, 80, 100, 200, 500, 1000, 2000, 5000, 10000 };
    g.setColour (juce::Colour (0xFF131316));
    SpectralAnalyzer tempAnalyzer;
    for (float freq : freqs)
    {
        int band = tempAnalyzer.frequencyToBand (freq);
        float x = area.getX() + bandToX (band, area.getWidth());
        g.drawVerticalLine ((int) x, area.getY(), area.getBottom());
    }
}

void SpectrumDisplay::drawBars (juce::Graphics& g, juce::Rectangle<float> area,
                                 const std::array<float, SpectralAnalyzer::numBands>& magnitudes)
{
    const float totalW = area.getWidth();
    const float barW = totalW / (float) SpectralAnalyzer::numBands;
    const float gap = std::max (1.0f, barW * 0.12f);
    const float actualW = barW - gap;
    const float segH = std::max (2.0f, area.getHeight() / 42.0f);
    const float segGap = 1.0f;

    const bool* overshoot = nullptr;
    if (showReference && curveManager.getActiveCurve() != nullptr)
        overshoot = eqMatcher.getOvershoot().data();

    for (int i = 0; i < SpectralAnalyzer::numBands; ++i)
    {
        float x = area.getX() + bandToX (i, totalW) + gap * 0.5f;
        float barTop = area.getY() + dbToY (magnitudes[(size_t) i], area.getHeight());
        float barBottom = area.getBottom();

        if (barTop >= barBottom) continue;

        bool isOver = (overshoot != nullptr && overshoot[i]);
        float y = barBottom;

        while (y - segH > barTop)
        {
            y -= (segH + segGap);
            float normY = 1.0f - (y - area.getY()) / area.getHeight();
            juce::Colour c = isOver ? RefBoxColours::danger : getBarColour (normY, i);
            g.setColour (c);
            g.fillRect (x, y, actualW, segH);
        }

        float rem = y - barTop;
        if (rem > 0.0f)
        {
            float normY = 1.0f - (barTop - area.getY()) / area.getHeight();
            g.setColour (isOver ? RefBoxColours::danger : getBarColour (normY, i));
            g.fillRect (x, barTop, actualW, rem);
        }
    }
}

void SpectrumDisplay::drawPeakHolders (juce::Graphics& g, juce::Rectangle<float> area)
{
    const float totalW = area.getWidth();
    const float barW = totalW / (float) SpectralAnalyzer::numBands;
    const float gap = std::max (1.0f, barW * 0.12f);
    const float actualW = barW - gap;

    for (int i = 0; i < SpectralAnalyzer::numBands; ++i)
    {
        float db = peakHold[(size_t) i];
        if (db <= minDb) continue;

        float x = area.getX() + bandToX (i, totalW) + gap * 0.5f;
        float y = area.getY() + dbToY (db, area.getHeight());
        float normY = 1.0f - (y - area.getY()) / area.getHeight();

        g.setColour (getBarColour (normY, i).withAlpha (0.85f));
        g.fillRect (x, y, actualW, 2.0f);
    }
}

void SpectrumDisplay::drawReferenceCurve (juce::Graphics& g, juce::Rectangle<float> area,
                                           const ReferenceCurveData& curve)
{
    const int nb = SpectralAnalyzer::numBands;
    const float barW = area.getWidth() / (float) nb;

    // Smooth the curve data with a moving average (radius 3)
    auto smoothValues = [] (const std::array<float, SpectralAnalyzer::numBands>& src,
                            const std::array<float, SpectralAnalyzer::numBands>& weight,
                            float offset) -> std::array<float, SpectralAnalyzer::numBands>
    {
        std::array<float, SpectralAnalyzer::numBands> result;
        const int radius = 3;
        for (int i = 0; i < SpectralAnalyzer::numBands; ++i)
        {
            float sum = 0.0f;
            int count = 0;
            for (int j = i - radius; j <= i + radius; ++j)
            {
                if (j >= 0 && j < SpectralAnalyzer::numBands)
                {
                    sum += src[(size_t) j] + weight[(size_t) j] + offset;
                    count++;
                }
            }
            result[(size_t) i] = sum / (float) count;
        }
        return result;
    };

    auto smoothAvg = smoothValues (curve.averageMagnitudes, displayWeighting, refCurveOffsetDb);
    auto smoothMax = smoothValues (curve.maxMagnitudes, displayWeighting, refCurveOffsetDb);

    juce::Path avgPath, maxPath;
    juce::Path fillPath;
    fillPath.startNewSubPath (area.getX(), area.getBottom());

    for (int i = 0; i < nb; ++i)
    {
        float x = area.getX() + bandToX (i, area.getWidth()) + barW * 0.5f;
        float yAvg = area.getY() + dbToY (smoothAvg[(size_t) i], area.getHeight());
        float yMax = area.getY() + dbToY (smoothMax[(size_t) i], area.getHeight());

        if (i == 0) { avgPath.startNewSubPath (x, yAvg); maxPath.startNewSubPath (x, yMax); }
        else { avgPath.lineTo (x, yAvg); maxPath.lineTo (x, yMax); }
        fillPath.lineTo (x, yMax);
    }

    fillPath.lineTo (area.getRight(), area.getBottom());
    fillPath.closeSubPath();

    // Round the corners for smoother curves
    juce::Path smoothAvgPath = avgPath.createPathWithRoundedCorners (8.0f);
    juce::Path smoothMaxPath = maxPath.createPathWithRoundedCorners (8.0f);

    g.setColour (RefBoxColours::reference.withAlpha (0.05f));
    g.fillPath (fillPath);
    g.setColour (RefBoxColours::reference.withAlpha (0.18f));
    g.strokePath (smoothMaxPath, juce::PathStrokeType (1.0f));
    g.setColour (RefBoxColours::reference.withAlpha (0.65f));
    g.strokePath (smoothAvgPath, juce::PathStrokeType (2.0f));
}

void SpectrumDisplay::drawDelta (juce::Graphics& g, juce::Rectangle<float> area)
{
    auto& delta = eqMatcher.getDelta();
    const float barW = area.getWidth() / (float) SpectralAnalyzer::numBands;
    float centerY = area.getY() + area.getHeight() * 0.5f;

    for (int i = 0; i < SpectralAnalyzer::numBands; ++i)
    {
        float x = area.getX() + bandToX (i, area.getWidth());
        float d = delta[(size_t) i];
        float h = std::min (std::abs (d) * (area.getHeight() / dbRange), area.getHeight() * 0.4f);

        g.setColour ((d > 0 ? RefBoxColours::danger : RefBoxColours::good).withAlpha (0.3f));
        g.fillRect (x + 0.5f, d > 0 ? centerY - h : centerY, barW - 1.0f, h);
    }

    g.setColour (RefBoxColours::textDim);
    g.drawHorizontalLine ((int) centerY, area.getX(), area.getRight());
}

void SpectrumDisplay::drawSystemResponse (juce::Graphics& g, juce::Rectangle<float> area)
{
    auto response = systemSim.getResponseCurve();
    juce::Path path;
    float centerDb = (maxDb + minDb) * 0.5f;

    for (int i = 0; i < 128; ++i)
    {
        float x = area.getX() + ((float) i / 128.0f) * area.getWidth();
        float y = area.getY() + dbToY (centerDb + response[(size_t) i], area.getHeight());
        if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
    }

    g.setColour (juce::Colour (0xFFBB44FF).withAlpha (0.4f));
    g.strokePath (path, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
}

void SpectrumDisplay::drawLabels (juce::Graphics& g, juce::Rectangle<float> plotArea)
{
    g.setColour (RefBoxColours::textDim);
    g.setFont (juce::FontOptions (std::max (8.0f, getHeight() * 0.022f)));

    struct FL { float f; const char* t; };
    FL labels[] = { {20,"20"}, {30,"30"}, {40,"40"}, {50,"50"}, {80,"80"}, {100,"100"}, {200,"200"},
                    {500,"500"}, {1000,"1k"}, {2000,"2k"}, {5000,"5k"}, {10000,"10k"}, {20000,"20k"} };

    SpectralAnalyzer tmp;
    float labelY = plotArea.getBottom() + 3.0f;
    for (auto& l : labels)
    {
        int band = tmp.frequencyToBand (l.f);
        float x = plotArea.getX() + bandToX (band, plotArea.getWidth());
        g.drawText (l.t, (int)(x - 16), (int)labelY, 32, 14, juce::Justification::centred);
    }

    for (float db = -60.0f; db <= 0.0f; db += 10.0f)
    {
        float y = plotArea.getY() + dbToY (db, plotArea.getHeight());
        g.drawText (juce::String ((int) db), 2, (int)(y - 6), (int)(plotArea.getX() - 4), 12,
                    juce::Justification::right);
    }
}
