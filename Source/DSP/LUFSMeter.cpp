#include "LUFSMeter.h"
#include <cmath>

LUFSMeter::LUFSMeter()
    : oversampling (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR)
{
}

void LUFSMeter::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate = sampleRate;
    currentNumChannels = numChannels;

    // 400ms momentary window
    momentaryBlockSize = (int) (sampleRate * 0.4);
    momentaryBuffer.resize ((size_t) momentaryBlockSize, 0.0f);
    momentaryWritePos = 0;

    // 3s short-term window
    shortTermBlockSize = (int) (sampleRate * 3.0);
    shortTermBuffer.resize ((size_t) shortTermBlockSize, 0.0f);
    shortTermWritePos = 0;

    // K-weighting filters
    kWeightChannels.resize ((size_t) numChannels);
    updateKWeightingFilters();

    // True peak oversampling
    oversampling.initProcessing ((size_t) samplesPerBlock);

    // Reset integrated
    gatingBlocks.clear();
    integratedSum = 0.0;
    integratedCount = 0;

    reset();
}

void LUFSMeter::updateKWeightingFilters()
{
    // ITU-R BS.1770-4 K-weighting
    // Stage 1: Pre-filter (high shelf +4dB at high frequencies)
    auto preCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        currentSampleRate, 1500.0, 0.7071f, 1.5849f); // +4dB

    // Stage 2: Revised Low-Band weighting (high-pass ~60Hz)
    auto rlbCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (
        currentSampleRate, 38.0, 0.5);

    for (auto& ch : kWeightChannels)
    {
        ch.preFilter.coefficients = preCoeffs;
        ch.preFilter.reset();
        ch.rlbFilter.coefficients = rlbCoeffs;
        ch.rlbFilter.reset();
    }
}

void LUFSMeter::processBlock (const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numCh = std::min (buffer.getNumChannels(), currentNumChannels);

    // Apply K-weighting and sum channels
    juce::AudioBuffer<float> kWeighted (numCh, numSamples);
    kWeighted.makeCopyOf (buffer);

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = kWeighted.getWritePointer (ch);

        for (int i = 0; i < numSamples; ++i)
        {
            data[i] = kWeightChannels[(size_t) ch].preFilter.processSample (data[i]);
            data[i] = kWeightChannels[(size_t) ch].rlbFilter.processSample (data[i]);
        }
    }

    // Calculate mean square per sample (summed channels)
    for (int i = 0; i < numSamples; ++i)
    {
        float sumSquared = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            float s = kWeighted.getSample (ch, i);
            sumSquared += s * s;
        }

        // BS.1770: sum channel powers (Gi=1.0 for L,R), do NOT average
        float meanSquare = sumSquared;

        // Write to momentary ring buffer
        momentaryBuffer[(size_t) momentaryWritePos] = meanSquare;
        momentaryWritePos = (momentaryWritePos + 1) % momentaryBlockSize;

        // Write to short-term ring buffer
        shortTermBuffer[(size_t) shortTermWritePos] = meanSquare;
        shortTermWritePos = (shortTermWritePos + 1) % shortTermBlockSize;
    }

    // Calculate momentary LUFS (400ms)
    {
        double sum = 0.0;
        for (int i = 0; i < momentaryBlockSize; ++i)
            sum += (double) momentaryBuffer[(size_t) i];

        double meanPower = sum / (double) momentaryBlockSize;
        float lufs = (meanPower > 0.0) ? (float) (-0.691 + 10.0 * std::log10 (meanPower)) : -100.0f;
        momentaryLUFS.store (lufs);
    }

    // Calculate short-term LUFS (3s)
    {
        double sum = 0.0;
        for (int i = 0; i < shortTermBlockSize; ++i)
            sum += (double) shortTermBuffer[(size_t) i];

        double meanPower = sum / (double) shortTermBlockSize;
        float lufs = (meanPower > 0.0) ? (float) (-0.691 + 10.0 * std::log10 (meanPower)) : -100.0f;
        shortTermLUFS.store (lufs);
    }

    // Update integrated LUFS with gating
    updateIntegrated (momentaryLUFS.load());

    // True peak detection
    {
        float peak = truePeak.load();
        for (int ch = 0; ch < numCh; ++ch)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                float absSample = std::abs (buffer.getSample (ch, i));
                if (absSample > peak)
                    peak = absSample;
            }
        }
        truePeak.store (peak);
    }
}

void LUFSMeter::updateIntegrated (float blockLoudness)
{
    if (blockLoudness <= -100.0f)
        return;

    gatingBlocks.push_back (blockLoudness);

    // Absolute gate: -70 LUFS
    double absGateSum = 0.0;
    int absGateCount = 0;

    for (float bl : gatingBlocks)
    {
        if (bl > -70.0f)
        {
            absGateSum += std::pow (10.0, (double) bl / 10.0);
            ++absGateCount;
        }
    }

    if (absGateCount == 0)
    {
        integratedLUFS.store (-100.0f);
        return;
    }

    double absGateMean = absGateSum / (double) absGateCount;
    float relativeThreshold = (float) (10.0 * std::log10 (absGateMean)) - 10.0f;

    // Relative gate: -10 dB below absolute gated mean
    double relGateSum = 0.0;
    int relGateCount = 0;

    for (float bl : gatingBlocks)
    {
        if (bl > -70.0f && bl > relativeThreshold)
        {
            relGateSum += std::pow (10.0, (double) bl / 10.0);
            ++relGateCount;
        }
    }

    if (relGateCount > 0)
    {
        double relGateMean = relGateSum / (double) relGateCount;
        integratedLUFS.store ((float) (10.0 * std::log10 (relGateMean)));
    }
}

void LUFSMeter::reset()
{
    std::fill (momentaryBuffer.begin(), momentaryBuffer.end(), 0.0f);
    std::fill (shortTermBuffer.begin(), shortTermBuffer.end(), 0.0f);
    momentaryWritePos = 0;
    shortTermWritePos = 0;
    gatingBlocks.clear();
    integratedSum = 0.0;
    integratedCount = 0;
    momentaryLUFS.store (-100.0f);
    shortTermLUFS.store (-100.0f);
    integratedLUFS.store (-100.0f);
    truePeak.store (-100.0f);
}

float LUFSMeter::getGainToMatch (float targetLUFS) const
{
    float current = integratedLUFS.load();
    if (current <= -100.0f)
        return 0.0f;

    return targetLUFS - current;
}
