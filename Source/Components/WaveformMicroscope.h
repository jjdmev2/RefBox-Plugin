#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

class RefBoxProcessor;

class WaveformMicroscope : public juce::Component, public juce::Timer
{
public:
    WaveformMicroscope (RefBoxProcessor& processor);

    void paint (juce::Graphics& g) override;
    void timerCallback() override;

    void analyzeTrack();

private:
    RefBoxProcessor& processor;

    // Fine-grained waveform peaks for crisp shape (64 samples per block, ~1.5ms)
    struct WaveBlock
    {
        float peakPos = 0.0f;   // max positive sample
        float peakNeg = 0.0f;   // min negative sample (stored as negative)
    };

    // Coarser color blocks for frequency-based RGB (1024 samples, ~23ms)
    struct ColorBlock
    {
        float lowEnergy = 0.0f;
        float midEnergy = 0.0f;
        float highEnergy = 0.0f;
    };

    std::vector<WaveBlock> waveBlocks;
    std::vector<ColorBlock> colorBlocks;

    double sampleRate = 44100.0;
    juce::int64 totalSamples = 0;
    double trackLengthSec = 0.0;

    double currentPosition = 0.0;    // raw position from processor
    double displayPosition = 0.0;    // smoothly interpolated for rendering
    bool displayPosInitialized = false;
    static constexpr double positionLerp = 0.18;  // smoothing factor per frame at 120Hz
    int lastActiveRef = -1;
    bool analyzed = false;

    static constexpr int waveSamplesPerBlock = 64;     // ~1.5ms — crisp transients
    static constexpr int colorSamplesPerBlock = 1024;  // ~23ms — enough for FFT color
    static constexpr double visibleSeconds = 6.0;

    juce::Colour getColourForSample (juce::int64 samplePos) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformMicroscope)
};
