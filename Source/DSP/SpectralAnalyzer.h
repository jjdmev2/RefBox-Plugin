#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include <atomic>

enum class SpectrumMode
{
    Fast,     // Low smoothing, responsive - good for mixing
    Average,  // Standard exponential average (default)
    Smooth,   // Long-term average - good for mastering
    Peak      // Peak detection with slow decay (fast attack, slow release)
};

class SpectralAnalyzer
{
public:
    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder; // 4096
    static constexpr int numBands = 128;

    SpectralAnalyzer();

    void prepare (double sampleRate, int samplesPerBlock);
    void pushSamples (const float* data, int numSamples);
    void processFFT();

    // Get magnitude spectrum in dB, averaged into bands (log-spaced)
    std::array<float, numBands>& getBandMagnitudes();

    // Get raw FFT magnitudes (for EQ matching)
    const std::vector<float>& getRawMagnitudes() const;

    bool isFFTDataReady() const { return fftDataReady.load(); }
    double getSampleRate() const { return currentSampleRate; }

    // Spectrum mode
    void setMode (SpectrumMode newMode) { mode = newMode; }
    SpectrumMode getMode() const { return mode; }

    // Band frequency helpers
    float getBandFrequency (int bandIndex) const;
    int frequencyToBand (float frequency) const;

private:
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    std::array<float, fftSize * 2> fftData{};
    std::array<float, fftSize * 2> fifoBuffer{};
    int fifoIndex = 0;

    std::array<float, numBands> bandMagnitudes{};
    std::vector<float> rawMagnitudes;

    std::atomic<bool> fftDataReady { false };
    double currentSampleRate = 44100.0;

    // Mode and smoothing
    SpectrumMode mode = SpectrumMode::Average;
    std::array<float, numBands> smoothedMagnitudes{};

    void calculateBandMagnitudes();
};
