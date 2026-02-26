#pragma once
#include <juce_core/juce_core.h>
#include "SpectralAnalyzer.h"
#include <array>
#include <vector>
#include <string>

// Stores a spectral reference curve (average + max per band)
struct ReferenceCurveData
{
    std::string name;
    std::array<float, SpectralAnalyzer::numBands> averageMagnitudes{};
    std::array<float, SpectralAnalyzer::numBands> maxMagnitudes{};
    int numFramesAnalyzed = 0;

    // Running sum for building curves from multiple tracks
    std::array<double, SpectralAnalyzer::numBands> runningSum{};
};

class ReferenceCurveManager
{
public:
    ReferenceCurveManager();

    // Capture a spectral frame into the current curve being built
    void captureFrame (const std::array<float, SpectralAnalyzer::numBands>& magnitudes);

    // Start building a new reference curve
    void startCapture (const std::string& name);

    // Finalize the current capture
    ReferenceCurveData finishCapture();

    // Analyze a whole audio file and add to the current curve
    void analyzeFile (const juce::File& audioFile, double sampleRate);

    // Analyze all WAV/AIFF/MP3 files in a folder to build a genre curve
    ReferenceCurveData analyzeFolder (const juce::File& folder, double sampleRate);

    // Built-in genre presets
    static ReferenceCurveData getGenrePreset (const std::string& genre);
    static std::vector<std::string> getAvailableGenres();

    // Save / load curves
    void saveCurve (const ReferenceCurveData& curve, const juce::File& file);
    ReferenceCurveData loadCurve (const juce::File& file);

    // Current loaded curves for comparison
    void addCurve (const ReferenceCurveData& curve);
    void removeCurve (int index);
    const std::vector<ReferenceCurveData>& getCurves() const { return loadedCurves; }

    // Active curve for comparison
    void setActiveCurveIndex (int index) { activeCurveIndex = index; }
    int getActiveCurveIndex() const { return activeCurveIndex; }
    const ReferenceCurveData* getActiveCurve() const;

private:
    ReferenceCurveData currentCapture;
    bool isCapturing = false;

    std::vector<ReferenceCurveData> loadedCurves;
    int activeCurveIndex = -1;

    // Built-in genre curve data (generated from analysis of typical tracks)
    static void initGenrePresets();
    static std::vector<ReferenceCurveData> genrePresets;
    static bool presetsInitialized;
};
