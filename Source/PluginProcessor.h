#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "DSP/SpectralAnalyzer.h"
#include "DSP/LUFSMeter.h"
#include "DSP/ReferenceCurve.h"
#include "DSP/SystemSimulator.h"
#include "DSP/EQMatcher.h"

class RefBoxProcessor : public juce::AudioProcessor
{
public:
    RefBoxProcessor();
    ~RefBoxProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- Public access to DSP components for the editor ---

    SpectralAnalyzer& getMixAnalyzer() { return mixAnalyzer; }
    SpectralAnalyzer& getReferenceAnalyzer() { return referenceAnalyzer; }
    LUFSMeter& getMixMeter() { return mixMeter; }
    LUFSMeter& getReferenceMeter() { return referenceMeter; }
    ReferenceCurveManager& getCurveManager() { return curveManager; }
    SystemSimulator& getSystemSim() { return systemSimulator; }
    EQMatcher& getEQMatcher() { return eqMatcher; }

    // Reference track management
    void loadReferenceTrack (const juce::File& file);
    void removeReferenceTrack (int index);
    void setActiveReference (int index);
    int getActiveReferenceIndex() const { return activeReferenceIndex.load(); }
    bool isPlayingReference() const { return playingReference.load(); }
    void setPlayingReference (bool shouldPlay);

    struct ReferenceTrack
    {
        juce::String name;
        juce::File file;
        std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
        std::unique_ptr<juce::AudioTransportSource> transport;
        float integratedLUFS = -100.0f;
    };

    const std::vector<ReferenceTrack>& getReferenceTracks() const { return referenceTracks; }

    // Reference track position and seeking
    double getReferencePosition() const;     // 0.0 - 1.0 normalized
    double getReferenceLengthSeconds() const;
    void seekReference (double normalizedPosition);

    // Get the audio thumbnail for waveform display
    juce::AudioFormatReader* getActiveReferenceReader() const;

    // A/B control
    bool isABActive() const { return abActive; }
    void setABActive (bool active) { abActive = active; }

    // Loudness match
    bool isLoudnessMatchEnabled() const { return loudnessMatchEnabled; }
    void setLoudnessMatchEnabled (bool enabled) { loudnessMatchEnabled = enabled; }

    // Reset meters
    void resetMeters() { mixMeter.reset(); referenceMeter.reset(); }

    // Easter egg
    bool isKittenMode() const { return kittenMode; }
    void setKittenMode (bool enabled) { kittenMode = enabled; }
    bool* getKittenModePtr() { return &kittenMode; }

    juce::AudioFormatManager& getFormatManager() { return formatManager; }

private:
    // DSP
    SpectralAnalyzer mixAnalyzer;
    SpectralAnalyzer referenceAnalyzer;
    LUFSMeter mixMeter;
    LUFSMeter referenceMeter;
    ReferenceCurveManager curveManager;
    SystemSimulator systemSimulator;
    EQMatcher eqMatcher;

    // Reference tracks
    juce::AudioFormatManager formatManager;
    std::vector<ReferenceTrack> referenceTracks;
    std::atomic<int> activeReferenceIndex { -1 };
    std::atomic<bool> playingReference { false };
    mutable juce::CriticalSection trackLock;  // protects referenceTracks vector

    // A/B
    bool abActive = false;
    bool loudnessMatchEnabled = true;
    bool kittenMode = false;

    // Internal buffers
    juce::AudioBuffer<float> referenceBuffer;
    juce::AudioBuffer<float> mixCopy;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RefBoxProcessor)
};
