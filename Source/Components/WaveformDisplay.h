#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>

class RefBoxProcessor;

class WaveformDisplay : public juce::Component,
                        public juce::Timer,
                        public juce::ChangeListener
{
public:
    WaveformDisplay (RefBoxProcessor& processor);
    ~WaveformDisplay() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

    // Call when the active reference track changes
    void loadThumbnail();

private:
    RefBoxProcessor& processor;

    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache;
    juce::AudioThumbnail thumbnail;

    double currentPosition = 0.0;  // 0.0 - 1.0
    bool thumbnailLoaded = false;
    int lastActiveRef = -1;

    void seekToMousePosition (const juce::MouseEvent& e);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};
