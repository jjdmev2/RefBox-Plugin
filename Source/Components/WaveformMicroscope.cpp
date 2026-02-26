#include "WaveformMicroscope.h"
#include "../PluginProcessor.h"
#include "RefBoxLookAndFeel.h"
#include <cmath>

WaveformMicroscope::WaveformMicroscope (RefBoxProcessor& p)
    : processor (p)
{
    startTimerHz (120);
}

void WaveformMicroscope::timerCallback()
{
    int activeIdx = processor.getActiveReferenceIndex();
    if (activeIdx != lastActiveRef)
    {
        lastActiveRef = activeIdx;
        displayPosInitialized = false;
        analyzeTrack();
    }

    double targetPos = processor.getReferencePosition();
    currentPosition = targetPos;

    // Smoothly interpolate display position toward target (lerp each frame)
    if (! displayPosInitialized || ! analyzed)
    {
        displayPosition = targetPos;
        displayPosInitialized = true;
    }
    else
    {
        displayPosition += (targetPos - displayPosition) * positionLerp;
    }

    // Always repaint at 120Hz for butter-smooth scrolling
    repaint();
}

void WaveformMicroscope::analyzeTrack()
{
    waveBlocks.clear();
    colorBlocks.clear();
    analyzed = false;
    trackLengthSec = 0.0;
    totalSamples = 0;

    auto* reader = processor.getActiveReferenceReader();
    if (reader == nullptr)
        return;

    sampleRate = reader->sampleRate;
    totalSamples = reader->lengthInSamples;
    trackLengthSec = (double) totalSamples / sampleRate;

    if (totalSamples <= 0)
        return;

    // --- Pass 1: Fine waveform peaks (64 samples per block) ---
    int numWaveBlocks = (int) (totalSamples / waveSamplesPerBlock);
    waveBlocks.resize ((size_t) numWaveBlocks);

    {
        juce::AudioBuffer<float> buffer ((int) reader->numChannels, waveSamplesPerBlock);
        for (int b = 0; b < numWaveBlocks; ++b)
        {
            juce::int64 start = (juce::int64) b * waveSamplesPerBlock;
            reader->read (&buffer, 0, waveSamplesPerBlock, start, true, true);

            float pMax = 0.0f, pMin = 0.0f;
            for (int i = 0; i < waveSamplesPerBlock; ++i)
            {
                float s = 0.0f;
                for (int ch = 0; ch < (int) reader->numChannels; ++ch)
                    s += buffer.getSample (ch, i);
                s /= (float) reader->numChannels;
                pMax = std::max (pMax, s);
                pMin = std::min (pMin, s);
            }
            waveBlocks[(size_t) b].peakPos = pMax;
            waveBlocks[(size_t) b].peakNeg = pMin;
        }
    }

    // --- Pass 2: Color blocks via FFT (1024 samples per block) ---
    int numColorBlocks = (int) (totalSamples / colorSamplesPerBlock);
    colorBlocks.resize ((size_t) numColorBlocks);

    {
        const int fftOrder = 10;  // 1024
        const int fftSize = 1 << fftOrder;
        juce::dsp::FFT fft (fftOrder);
        juce::dsp::WindowingFunction<float> window (fftSize, juce::dsp::WindowingFunction<float>::hann);

        std::vector<float> fftData ((size_t) fftSize * 2, 0.0f);
        juce::AudioBuffer<float> buffer ((int) reader->numChannels, colorSamplesPerBlock);

        float binWidth = (float) sampleRate / (float) fftSize;
        int lowBinEnd = std::max (1, (int) (250.0f / binWidth));
        int midBinEnd = std::max (lowBinEnd + 1, (int) (5000.0f / binWidth));
        int highBinEnd = fftSize / 2;

        for (int b = 0; b < numColorBlocks; ++b)
        {
            juce::int64 start = (juce::int64) b * colorSamplesPerBlock;
            reader->read (&buffer, 0, colorSamplesPerBlock, start, true, true);

            for (int i = 0; i < colorSamplesPerBlock; ++i)
            {
                float s = 0.0f;
                for (int ch = 0; ch < (int) reader->numChannels; ++ch)
                    s += buffer.getSample (ch, i);
                fftData[(size_t) i] = s / (float) reader->numChannels;
            }
            for (int i = colorSamplesPerBlock; i < fftSize * 2; ++i)
                fftData[(size_t) i] = 0.0f;

            window.multiplyWithWindowingTable (fftData.data(), fftSize);
            fft.performFrequencyOnlyForwardTransform (fftData.data());

            float lowE = 0.0f, midE = 0.0f, highE = 0.0f;
            for (int bin = 1; bin <= lowBinEnd && bin < highBinEnd; ++bin)
                lowE += fftData[(size_t) bin] * fftData[(size_t) bin];
            for (int bin = lowBinEnd + 1; bin <= midBinEnd && bin < highBinEnd; ++bin)
                midE += fftData[(size_t) bin] * fftData[(size_t) bin];
            for (int bin = midBinEnd + 1; bin < highBinEnd; ++bin)
                highE += fftData[(size_t) bin] * fftData[(size_t) bin];

            float total = lowE + midE + highE;
            if (total > 0.0f)
            {
                colorBlocks[(size_t) b].lowEnergy = lowE / total;
                colorBlocks[(size_t) b].midEnergy = midE / total;
                colorBlocks[(size_t) b].highEnergy = highE / total;
            }
        }
    }

    analyzed = true;
}

juce::Colour WaveformMicroscope::getColourForSample (juce::int64 samplePos) const
{
    int colorIdx = (int) (samplePos / colorSamplesPerBlock);
    if (colorIdx < 0 || colorIdx >= (int) colorBlocks.size())
        return juce::Colour (0xFF447788);

    auto& cb = colorBlocks[(size_t) colorIdx];

    // Pioneer CDJ RGB: Red = lows, Green = mids, Blue = highs
    float r = std::min (1.0f, cb.lowEnergy * 2.5f);
    float g = std::min (1.0f, cb.midEnergy * 1.8f);
    float b = std::min (1.0f, cb.highEnergy * 3.0f);

    // Boost saturation
    float maxC = std::max ({r, g, b});
    if (maxC > 0.0f)
    {
        float boost = 1.0f + (1.0f / maxC - 1.0f) * 0.5f;
        r = std::min (1.0f, r * boost);
        g = std::min (1.0f, g * boost);
        b = std::min (1.0f, b * boost);
    }

    return juce::Colour::fromFloatRGBA (r, g, b, 1.0f);
}

void WaveformMicroscope::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xFF0A0A0E));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (RefBoxColours::panelBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

    if (! analyzed || waveBlocks.empty() || trackLengthSec <= 0.0)
    {
        g.setColour (RefBoxColours::textDim);
        g.setFont (juce::FontOptions (9.0f));
        g.drawText ("MICROSCOPE", bounds, juce::Justification::centred);
        return;
    }

    auto waveArea = bounds.reduced (2.0f, 1.0f);
    float w = waveArea.getWidth();
    float h = waveArea.getHeight();
    float centerY = waveArea.getCentreY();
    float halfH = h * 0.47f;

    // Center line
    g.setColour (juce::Colour (0xFF141418));
    g.drawHorizontalLine ((int) centerY, waveArea.getX(), waveArea.getRight());

    // Time range visible (use interpolated displayPosition for smooth scrolling)
    double currentTimeSec = displayPosition * trackLengthSec;
    double startTimeSec = currentTimeSec - visibleSeconds * 0.5;

    // Samples per pixel for the visible window
    double samplesPerPixel = (visibleSeconds * sampleRate) / (double) w;

    int numWave = (int) waveBlocks.size();

    for (int px = 0; px < (int) w; ++px)
    {
        // Sample range for this pixel
        double pixelTimeSec = startTimeSec + ((double) px / (double) w) * visibleSeconds;
        juce::int64 sampleStart = (juce::int64) (pixelTimeSec * sampleRate);
        juce::int64 sampleEnd = sampleStart + (juce::int64) samplesPerPixel;

        if (sampleEnd < 0 || sampleStart >= totalSamples)
            continue;

        sampleStart = std::max ((juce::int64) 0, sampleStart);
        sampleEnd = std::min (totalSamples, sampleEnd);

        // Find peak envelope across wave blocks in this pixel's range
        int wbStart = (int) (sampleStart / waveSamplesPerBlock);
        int wbEnd = (int) (sampleEnd / waveSamplesPerBlock);
        wbStart = std::max (0, std::min (wbStart, numWave - 1));
        wbEnd = std::max (0, std::min (wbEnd, numWave - 1));

        float pMax = 0.0f, pMin = 0.0f;
        for (int wb = wbStart; wb <= wbEnd; ++wb)
        {
            pMax = std::max (pMax, waveBlocks[(size_t) wb].peakPos);
            pMin = std::min (pMin, waveBlocks[(size_t) wb].peakNeg);
        }

        // Get colour from the center of this pixel's time range
        juce::int64 midSample = (sampleStart + sampleEnd) / 2;
        auto col = getColourForSample (midSample);

        float x = waveArea.getX() + (float) px;
        float yTop = centerY - pMax * halfH;
        float yBot = centerY - pMin * halfH;  // pMin is negative, so this goes below center

        // Draw the filled waveform column
        g.setColour (col);
        g.fillRect (x, yTop, 1.0f, yBot - yTop);

        // Dim glow around peaks
        g.setColour (col.withAlpha (0.15f));
        g.fillRect (x, yTop - 1.0f, 1.0f, 1.0f);
        g.fillRect (x, yBot, 1.0f, 1.0f);
    }

    // Playhead (center white line)
    float playheadX = waveArea.getX() + w * 0.5f;
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.fillRect (playheadX - 0.5f, waveArea.getY(), 1.0f, h);

    // Label
    g.setColour (RefBoxColours::textDim.withAlpha (0.5f));
    g.setFont (juce::FontOptions (8.0f));
    g.drawText ("MICROSCOPE", (int) (waveArea.getX() + 3), (int) waveArea.getY(), 70, 10,
                juce::Justification::centredLeft);
}
