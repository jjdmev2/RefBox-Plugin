#pragma once
#include "SpectralAnalyzer.h"
#include "ReferenceCurve.h"
#include <array>

// Computes the EQ delta between the current mix and a reference curve
// Shows overshoot (where mix exceeds reference) and deficit
class EQMatcher
{
public:
    EQMatcher();

    // Calculate the delta between current mix spectrum and reference
    void calculate (const std::array<float, SpectralAnalyzer::numBands>& mixSpectrum,
                    const ReferenceCurveData& reference);

    // Get the delta curve (positive = mix is louder, negative = mix is quieter)
    const std::array<float, SpectralAnalyzer::numBands>& getDelta() const { return deltaCurve; }

    // Get overshoot flags (true where mix exceeds reference max)
    const std::array<bool, SpectralAnalyzer::numBands>& getOvershoot() const { return overshootFlags; }

    // Get the corrective EQ curve (negate of delta - what you'd need to apply)
    const std::array<float, SpectralAnalyzer::numBands>& getCorrectiveCurve() const { return correctiveCurve; }

    // Smoothing for the delta display
    void setSmoothing (float factor) { smoothingFactor = factor; }

    // Threshold for overshoot detection (dB above max curve)
    void setOvershootThreshold (float thresholdDb) { overshootThreshold = thresholdDb; }

    // Overall match score (0-100, how well the mix matches the reference)
    float getMatchScore() const { return matchScore; }

    // Per-region scores
    struct RegionScore
    {
        float sub = 0.0f;      // 20-60Hz
        float bass = 0.0f;     // 60-250Hz
        float lowMid = 0.0f;   // 250-500Hz
        float mid = 0.0f;      // 500-2kHz
        float upperMid = 0.0f; // 2k-4kHz
        float presence = 0.0f; // 4k-8kHz
        float air = 0.0f;      // 8k-20kHz
    };

    RegionScore getRegionScores() const { return regionScores; }

private:
    std::array<float, SpectralAnalyzer::numBands> deltaCurve{};
    std::array<float, SpectralAnalyzer::numBands> correctiveCurve{};
    std::array<float, SpectralAnalyzer::numBands> smoothedDelta{};
    std::array<bool, SpectralAnalyzer::numBands> overshootFlags{};

    float smoothingFactor = 0.85f;
    float overshootThreshold = 0.0f; // dB above max
    float matchScore = 0.0f;
    RegionScore regionScores;

    void calculateRegionScores();
};
