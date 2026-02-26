#include "SpectralAnalyzer.h"
#include <cmath>

SpectralAnalyzer::SpectralAnalyzer()
    : forwardFFT (fftOrder),
      window (fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    rawMagnitudes.resize (fftSize / 2, 0.0f);
    smoothedMagnitudes.fill (-100.0f);
    bandMagnitudes.fill (-100.0f);
}

void SpectralAnalyzer::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    fifoIndex = 0;
    fftData.fill (0.0f);
    fifoBuffer.fill (0.0f);
    smoothedMagnitudes.fill (-100.0f);
    bandMagnitudes.fill (-100.0f);
}

void SpectralAnalyzer::pushSamples (const float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        fifoBuffer[(size_t) fifoIndex] = data[i];
        ++fifoIndex;

        if (fifoIndex >= fftSize)
        {
            std::copy (fifoBuffer.begin(), fifoBuffer.begin() + fftSize, fftData.begin());
            fftDataReady.store (true);
            fifoIndex = 0;
        }
    }
}

void SpectralAnalyzer::processFFT()
{
    if (! fftDataReady.load())
        return;

    fftDataReady.store (false);

    window.multiplyWithWindowingTable (fftData.data(), fftSize);
    forwardFFT.performFrequencyOnlyForwardTransform (fftData.data());

    const int numBins = fftSize / 2;
    rawMagnitudes.resize (numBins);

    for (int i = 0; i < numBins; ++i)
        rawMagnitudes[(size_t) i] = fftData[(size_t) i];

    calculateBandMagnitudes();
}

void SpectralAnalyzer::calculateBandMagnitudes()
{
    // Log-spaced bands from 20Hz to 20kHz
    const float minFreq = 20.0f;
    const float maxFreq = 20000.0f;
    const float logMin = std::log10 (minFreq);
    const float logMax = std::log10 (maxFreq);
    const int numBins = fftSize / 2;
    const float binWidth = (float) currentSampleRate / (float) fftSize;

    for (int band = 0; band < numBands; ++band)
    {
        float bandStart = std::pow (10.0f, logMin + (logMax - logMin) * (float) band / (float) numBands);
        float bandEnd = std::pow (10.0f, logMin + (logMax - logMin) * (float) (band + 1) / (float) numBands);

        int binStart = std::max (1, (int) (bandStart / binWidth));
        int binEnd = std::min (numBins - 1, (int) (bandEnd / binWidth));

        float magnitude = 0.0f;
        int count = 0;

        if (mode == SpectrumMode::Peak)
        {
            // Peak: max bin value in band
            for (int bin = binStart; bin <= binEnd; ++bin)
            {
                magnitude = std::max (magnitude, rawMagnitudes[(size_t) bin]);
                ++count;
            }
        }
        else
        {
            // Fast/Average/Smooth: mean of bin magnitudes
            float sum = 0.0f;
            for (int bin = binStart; bin <= binEnd; ++bin)
            {
                sum += rawMagnitudes[(size_t) bin];
                ++count;
            }
            magnitude = (count > 0) ? (sum / (float) count) : 0.0f;
        }

        // Convert to dB
        float db = (magnitude > 0.0f)
            ? 20.0f * std::log10 (magnitude / (float) fftSize)
            : -100.0f;

        // Mode-dependent smoothing/ballistics
        switch (mode)
        {
            case SpectrumMode::Fast:
                // Low smoothing - responsive, shows transients
                smoothedMagnitudes[(size_t) band] = smoothedMagnitudes[(size_t) band] * 0.35f
                                                    + db * 0.65f;
                break;

            case SpectrumMode::Smooth:
                // High smoothing - stable, long-term tonal balance
                smoothedMagnitudes[(size_t) band] = smoothedMagnitudes[(size_t) band] * 0.93f
                                                    + db * 0.07f;
                break;

            case SpectrumMode::Peak:
                // Fast attack (instant), slow release (like SPAN RT Max)
                if (db > smoothedMagnitudes[(size_t) band])
                    smoothedMagnitudes[(size_t) band] = db;
                else
                    smoothedMagnitudes[(size_t) band] = smoothedMagnitudes[(size_t) band] * 0.97f
                                                        + db * 0.03f;
                break;

            case SpectrumMode::Average:
            default:
                // Standard exponential average (like SPAN/Pro-Q default)
                smoothedMagnitudes[(size_t) band] = smoothedMagnitudes[(size_t) band] * 0.75f
                                                    + db * 0.25f;
                break;
        }

        bandMagnitudes[(size_t) band] = smoothedMagnitudes[(size_t) band];
    }
}

std::array<float, SpectralAnalyzer::numBands>& SpectralAnalyzer::getBandMagnitudes()
{
    return bandMagnitudes;
}

const std::vector<float>& SpectralAnalyzer::getRawMagnitudes() const
{
    return rawMagnitudes;
}

float SpectralAnalyzer::getBandFrequency (int bandIndex) const
{
    const float minFreq = 20.0f;
    const float maxFreq = 20000.0f;
    const float logMin = std::log10 (minFreq);
    const float logMax = std::log10 (maxFreq);

    return std::pow (10.0f, logMin + (logMax - logMin) * ((float) bandIndex + 0.5f) / (float) numBands);
}

int SpectralAnalyzer::frequencyToBand (float frequency) const
{
    const float minFreq = 20.0f;
    const float maxFreq = 20000.0f;
    const float logMin = std::log10 (minFreq);
    const float logMax = std::log10 (maxFreq);

    float logFreq = std::log10 (std::clamp (frequency, minFreq, maxFreq));
    return std::clamp ((int) ((logFreq - logMin) / (logMax - logMin) * (float) numBands), 0, numBands - 1);
}
