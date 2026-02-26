#include "EQMatcher.h"
#include <cmath>
#include <algorithm>

EQMatcher::EQMatcher()
{
    deltaCurve.fill (0.0f);
    correctiveCurve.fill (0.0f);
    smoothedDelta.fill (0.0f);
    overshootFlags.fill (false);
}

void EQMatcher::calculate (const std::array<float, SpectralAnalyzer::numBands>& mixSpectrum,
                            const ReferenceCurveData& reference)
{
    float totalDeltaSquared = 0.0f;

    for (int i = 0; i < SpectralAnalyzer::numBands; ++i)
    {
        // Delta = mix - reference average
        float delta = mixSpectrum[(size_t) i] - reference.averageMagnitudes[(size_t) i];

        // Smooth the delta
        smoothedDelta[(size_t) i] = smoothedDelta[(size_t) i] * smoothingFactor
                                     + delta * (1.0f - smoothingFactor);
        deltaCurve[(size_t) i] = smoothedDelta[(size_t) i];

        // Corrective curve is the inverse
        correctiveCurve[(size_t) i] = -deltaCurve[(size_t) i];

        // Overshoot: mix exceeds reference maximum
        overshootFlags[(size_t) i] =
            (mixSpectrum[(size_t) i] > reference.maxMagnitudes[(size_t) i] + overshootThreshold);

        totalDeltaSquared += delta * delta;
    }

    // Match score: lower delta = better match
    // Score of 100 means perfect match, 0 means very far off
    float rms = std::sqrt (totalDeltaSquared / (float) SpectralAnalyzer::numBands);
    matchScore = std::max (0.0f, 100.0f - rms * 3.0f);

    calculateRegionScores();
}

void EQMatcher::calculateRegionScores()
{
    // Calculate per-region match scores
    // Band mapping for 128 bands across 20Hz-20kHz (log-spaced):
    // Sub (20-60Hz): bands 0-14
    // Bass (60-250Hz): bands 15-35
    // Low Mid (250-500Hz): bands 36-48
    // Mid (500-2kHz): bands 49-72
    // Upper Mid (2k-4kHz): bands 73-84
    // Presence (4k-8kHz): bands 85-97
    // Air (8k-20kHz): bands 98-127

    auto scoreRegion = [this] (int startBand, int endBand) -> float
    {
        float sumSquared = 0.0f;
        int count = 0;
        for (int i = startBand; i <= endBand && i < SpectralAnalyzer::numBands; ++i)
        {
            sumSquared += deltaCurve[(size_t) i] * deltaCurve[(size_t) i];
            ++count;
        }
        if (count == 0) return 100.0f;
        float rms = std::sqrt (sumSquared / (float) count);
        return std::max (0.0f, 100.0f - rms * 3.0f);
    };

    regionScores.sub = scoreRegion (0, 14);
    regionScores.bass = scoreRegion (15, 35);
    regionScores.lowMid = scoreRegion (36, 48);
    regionScores.mid = scoreRegion (49, 72);
    regionScores.upperMid = scoreRegion (73, 84);
    regionScores.presence = scoreRegion (85, 97);
    regionScores.air = scoreRegion (98, 127);
}
