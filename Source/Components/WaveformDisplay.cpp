#include "WaveformDisplay.h"
#include "../PluginProcessor.h"
#include "RefBoxLookAndFeel.h"

WaveformDisplay::WaveformDisplay (RefBoxProcessor& p)
    : processor (p),
      thumbnailCache (5),
      thumbnail (512, p.getFormatManager(), thumbnailCache)
{
    formatManager.registerBasicFormats();
    thumbnail.addChangeListener (this);
    startTimerHz (15);
}

WaveformDisplay::~WaveformDisplay()
{
    thumbnail.removeChangeListener (this);
}

void WaveformDisplay::loadThumbnail()
{
    auto& tracks = processor.getReferenceTracks();
    int activeIdx = processor.getActiveReferenceIndex();

    if (activeIdx >= 0 && activeIdx < (int) tracks.size())
    {
        auto& track = tracks[(size_t) activeIdx];
        if (track.file.existsAsFile())
        {
            thumbnail.clear();
            thumbnailCache.clear();
            thumbnail.setSource (new juce::FileInputSource (track.file));
            thumbnailLoaded = true;
        }
    }
    else
    {
        thumbnail.clear();
        thumbnailLoaded = false;
    }
}

void WaveformDisplay::timerCallback()
{
    // Check if active reference changed
    int activeIdx = processor.getActiveReferenceIndex();
    if (activeIdx != lastActiveRef)
    {
        lastActiveRef = activeIdx;
        loadThumbnail();
    }

    // Update playback position
    double newPos = processor.getReferencePosition();
    if (std::abs (newPos - currentPosition) > 0.001)
    {
        currentPosition = newPos;
        repaint();
    }
}

void WaveformDisplay::changeListenerCallback (juce::ChangeBroadcaster*)
{
    repaint();
}

void WaveformDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour (RefBoxColours::panelBg);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (RefBoxColours::panelBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    // "WAVEFORM" label
    g.setColour (RefBoxColours::textDim);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("WAVEFORM", bounds.getX() + 6.0f, bounds.getY() + 2.0f, 70.0f, 12.0f,
                juce::Justification::centredLeft);

    if (! thumbnailLoaded || thumbnail.getTotalLength() <= 0.0)
    {
        g.setColour (RefBoxColours::textDim);
        g.setFont (juce::FontOptions (std::max (10.0f, bounds.getHeight() * 0.22f)));
        g.drawText ("Drop or load a reference track",
                     bounds, juce::Justification::centred);
        return;
    }

    auto waveArea = bounds.reduced (3.0f, 2.0f);

    // Center line
    g.setColour (RefBoxColours::panelBorder);
    g.drawHorizontalLine ((int) waveArea.getCentreY(), waveArea.getX(), waveArea.getRight());

    // Waveform
    g.setColour (RefBoxColours::accent.withAlpha (0.55f));
    thumbnail.drawChannels (g, waveArea.toNearestInt(),
                            0.0, thumbnail.getTotalLength(), 1.0f);

    // Playback position
    if (currentPosition > 0.0 && currentPosition < 1.0)
    {
        float xPos = waveArea.getX() + (float) currentPosition * waveArea.getWidth();

        // Played region tint
        g.setColour (RefBoxColours::accent.withAlpha (0.06f));
        g.fillRect (waveArea.getX(), waveArea.getY(),
                    xPos - waveArea.getX(), waveArea.getHeight());

        // Playhead line
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillRect (xPos - 0.5f, waveArea.getY(), 1.5f, waveArea.getHeight());

        // Time display
        double totalSec = processor.getReferenceLengthSeconds();
        double curSec = currentPosition * totalSec;
        int curMin = (int) curSec / 60;
        int curS = (int) curSec % 60;
        int totMin = (int) totalSec / 60;
        int totS = (int) totalSec % 60;

        auto timeStr = juce::String::formatted ("%d:%02d / %d:%02d", curMin, curS, totMin, totS);
        g.setColour (RefBoxColours::textPrimary);
        g.setFont (juce::FontOptions (std::max (9.0f, bounds.getHeight() * 0.2f)));
        g.drawText (timeStr, (int) (waveArea.getX() + 6), (int) (waveArea.getY() + 2),
                    120, 14, juce::Justification::centredLeft);
    }

    // Track name
    auto& tracks = processor.getReferenceTracks();
    int idx = processor.getActiveReferenceIndex();
    if (idx >= 0 && idx < (int) tracks.size())
    {
        g.setColour (RefBoxColours::reference);
        g.setFont (juce::FontOptions (std::max (9.0f, bounds.getHeight() * 0.2f)));
        g.drawText (tracks[(size_t) idx].name,
                    (int) (waveArea.getRight() - 220), (int) (waveArea.getY() + 2),
                    214, 14, juce::Justification::centredRight);
    }
}

void WaveformDisplay::resized()
{
}

void WaveformDisplay::mouseDown (const juce::MouseEvent& e)
{
    seekToMousePosition (e);
}

void WaveformDisplay::mouseDrag (const juce::MouseEvent& e)
{
    seekToMousePosition (e);
}

void WaveformDisplay::seekToMousePosition (const juce::MouseEvent& e)
{
    auto waveArea = getLocalBounds().toFloat().reduced (2.0f);

    if (waveArea.getWidth() <= 0.0f || ! thumbnailLoaded)
        return;

    float relativeX = ((float) e.x - waveArea.getX()) / waveArea.getWidth();
    double normalized = juce::jlimit (0.0, 1.0, (double) relativeX);

    processor.seekReference (normalized);
    currentPosition = normalized;
    repaint();
}
