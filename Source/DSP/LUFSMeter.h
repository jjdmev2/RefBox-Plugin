#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <atomic>

class LUFSMeter
{
public:
    LUFSMeter();

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void processBlock (const juce::AudioBuffer<float>& buffer);
    void reset();

    float getMomentaryLUFS() const { return momentaryLUFS.load(); }
    float getShortTermLUFS() const { return shortTermLUFS.load(); }
    float getIntegratedLUFS() const { return integratedLUFS.load(); }
    float getTruePeak() const { return truePeak.load(); }

    // For loudness matching: returns gain in dB to match target LUFS
    float getGainToMatch (float targetLUFS) const;

private:
    double currentSampleRate = 44100.0;
    int currentNumChannels = 2;

    // K-weighting filters (ITU-R BS.1770)
    struct KWeightChannel
    {
        // Stage 1: Pre-filter (high shelf)
        juce::dsp::IIR::Filter<float> preFilter;
        // Stage 2: RLB weighting (high pass)
        juce::dsp::IIR::Filter<float> rlbFilter;
    };

    std::vector<KWeightChannel> kWeightChannels;

    // Gating block buffers
    // 400ms momentary, 3s short-term
    std::vector<float> momentaryBuffer;
    std::vector<float> shortTermBuffer;
    int momentaryWritePos = 0;
    int shortTermWritePos = 0;
    int momentaryBlockSize = 0;
    int shortTermBlockSize = 0;

    // Integrated loudness (gated)
    std::vector<float> gatingBlocks;
    double integratedSum = 0.0;
    int integratedCount = 0;

    // True peak detection (4x oversampling)
    juce::dsp::Oversampling<float> oversampling;

    std::atomic<float> momentaryLUFS { -100.0f };
    std::atomic<float> shortTermLUFS { -100.0f };
    std::atomic<float> integratedLUFS { -100.0f };
    std::atomic<float> truePeak { -100.0f };

    void updateKWeightingFilters();
    float calculateLoudness (const float* data, int numSamples) const;
    void updateIntegrated (float blockLoudness);
};
