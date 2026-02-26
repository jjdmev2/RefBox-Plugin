#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/SpectralAnalyzer.h"
#include "../DSP/ReferenceCurve.h"
#include "../DSP/EQMatcher.h"
#include "../DSP/SystemSimulator.h"
#include "RefBoxLookAndFeel.h"

enum class VisualizerColorScheme
{
    Classic,     // Green/cyan to yellow to red (height-based)
    PioneerRGB,  // Red=bass, Green=mids, Blue=highs (frequency-based, CDJ style)
    Amber,       // Warm amber/gold gradient
    Purple,      // Purple/violet gradient
    Fire         // Yellow to orange to red
};

class SpectrumDisplay : public juce::Component, public juce::Timer
{
public:
    SpectrumDisplay (SpectralAnalyzer& mixAnalyzer,
                     SpectralAnalyzer& refAnalyzer,
                     ReferenceCurveManager& curveManager,
                     EQMatcher& eqMatcher,
                     SystemSimulator& systemSim);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

    void setShowReference (bool show) { showReference = show; }
    void setShowDelta (bool show) { showDelta = show; }
    void setShowSystemResponse (bool show) { showSystemResponse = show; }
    void setShowRefSpectrum (bool show) { showRefSpectrum = show; }

    void setSpectrumMode (SpectrumMode newMode);
    SpectrumMode getSpectrumMode() const { return spectrumMode; }

    void setColorScheme (VisualizerColorScheme scheme) { colorScheme = scheme; repaint(); }
    VisualizerColorScheme getColorScheme() const { return colorScheme; }

private:
    SpectralAnalyzer& mixAnalyzer;
    SpectralAnalyzer& refAnalyzer;
    ReferenceCurveManager& curveManager;
    EQMatcher& eqMatcher;
    SystemSimulator& systemSim;

    bool showReference = true;
    bool showDelta = false;
    bool showSystemResponse = false;
    bool showRefSpectrum = false;

    SpectrumMode spectrumMode = SpectrumMode::Average;
    VisualizerColorScheme colorScheme = VisualizerColorScheme::Classic;

    static constexpr float minDb = -70.0f;
    static constexpr float maxDb = 6.0f;
    static constexpr float dbRange = maxDb - minDb;

    // Peak hold
    std::array<float, SpectralAnalyzer::numBands> peakHold{};
    std::array<float, SpectralAnalyzer::numBands> peakHoldCounter{};
    static constexpr float peakFallRate = 0.35f;
    static constexpr float peakHoldTime = 25.0f;

    // Equal loudness weighting (applied to display only)
    std::array<float, SpectralAnalyzer::numBands> displayWeighting{};
    void initDisplayWeighting();

    // Display-level interpolation (smooths between FFT updates for butter-smooth 120fps)
    std::array<float, SpectralAnalyzer::numBands> displayMagnitudes{};
    bool displayMagsInitialized = false;
    static constexpr float displayLerp = 0.22f;  // per-frame interpolation at 120Hz

    // Reference curve vertical offset (shift+drag)
    float refCurveOffsetDb = 0.0f;
    float dragStartOffsetDb = 0.0f;
    float dragStartY = 0.0f;
    bool isDraggingCurve = false;

    // Manual display gain (vertical zoom for spectrum bars)
    float displayGainDb = 0.0f;
    juce::TextButton gainUpBtn { "+" };
    juce::TextButton gainDownBtn { "\xe2\x88\x92" };  // minus sign

    float dbToY (float db, float height) const;
    float bandToX (int band, float width) const;
    void updatePeakHold (const std::array<float, SpectralAnalyzer::numBands>& magnitudes);
    juce::Colour getBarColour (float normalizedY, int band = 0) const;

    void drawGrid (juce::Graphics& g, juce::Rectangle<float> area);
    void drawBars (juce::Graphics& g, juce::Rectangle<float> area,
                   const std::array<float, SpectralAnalyzer::numBands>& magnitudes);
    void drawPeakHolders (juce::Graphics& g, juce::Rectangle<float> area);
    void drawReferenceCurve (juce::Graphics& g, juce::Rectangle<float> area,
                             const ReferenceCurveData& curve);
    void drawDelta (juce::Graphics& g, juce::Rectangle<float> area);
    void drawSystemResponse (juce::Graphics& g, juce::Rectangle<float> area);
    void drawLabels (juce::Graphics& g, juce::Rectangle<float> plotArea);

    // Hover frequency display
    juce::Point<float> hoverPos { -1.0f, -1.0f };
    bool mouseInside = false;

    // Kitten easter egg
    bool* kittenModePtr = nullptr;
    float kittenPhase = 0.0f;
    float kittenBob = 0.0f;
    float kittenCurrentOsc = 0.5f;   // current horizontal position (0..1 in band range)
    float kittenTargetOsc = 0.5f;    // target horizontal position
    float kittenPauseFrames = 0.0f;  // frames to stay at current position
    float kittenDisplayY = -1.0f;    // smoothed vertical position (-1 = uninitialized)
    void drawPixelKitten (juce::Graphics& g, float x, float waveTopY, float scale) const;

public:
    void setKittenModePtr (bool* ptr) { kittenModePtr = ptr; }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumDisplay)
};
