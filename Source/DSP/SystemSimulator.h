#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <string>
#include <array>

// Simulates how audio sounds on different playback systems
// Uses parametric EQ curves modeled after real device measurements
class SystemSimulator
{
public:
    enum class SystemType
    {
        Flat = 0,       // No simulation (bypass)
        Laptop,         // Typical laptop speakers
        Phone,          // Smartphone speaker
        Earbuds,        // Consumer earbuds (AirPods-like)
        CarStereo,      // Average car audio
        TVSpeakers,     // Flat panel TV
        ClubPA,         // Club PA system
        BluetoothSmall, // Small bluetooth speaker (JBL Flip-like)
        StudioNS10,     // Yamaha NS-10 approximation
        AuratoneC5,     // Auratone / Mixcube
        HomePod,        // Smart speaker
        Headphones,     // Consumer over-ear headphones
        NumSystems
    };

    SystemSimulator();

    void prepare (double sampleRate, int samplesPerBlock);
    void processBlock (juce::AudioBuffer<float>& buffer);
    void reset();

    void setSystemType (SystemType type);
    SystemType getSystemType() const { return currentSystem; }

    static std::string getSystemName (SystemType type);
    static int getNumSystems() { return (int) SystemType::NumSystems; }

    // Get the frequency response curve for display
    std::array<float, 128> getResponseCurve() const;

    void setEnabled (bool enabled) { isEnabled = enabled; }
    bool getEnabled() const { return isEnabled; }

private:
    SystemType currentSystem = SystemType::Flat;
    bool isEnabled = false;
    double currentSampleRate = 44100.0;

    // Filter chain for system simulation
    // Using cascaded biquads for flexible frequency shaping
    static constexpr int maxFilters = 8;

    struct FilterStage
    {
        juce::dsp::IIR::Filter<float> left;
        juce::dsp::IIR::Filter<float> right;
        bool active = false;
    };

    std::array<FilterStage, maxFilters> filters;

    // Bass rolloff for small speakers
    juce::dsp::IIR::Filter<float> bassRolloffL;
    juce::dsp::IIR::Filter<float> bassRolloffR;
    bool hasBassRolloff = false;

    // Treble rolloff for muffled systems
    juce::dsp::IIR::Filter<float> trebleRolloffL;
    juce::dsp::IIR::Filter<float> trebleRolloffR;
    bool hasTrebleRolloff = false;

    void configureFilters();

    struct SystemProfile
    {
        float bassRolloffFreq = 0.0f;     // HP frequency (0 = no rolloff)
        float trebleRolloffFreq = 0.0f;   // LP frequency (0 = no rolloff)

        struct Band
        {
            float freq;
            float gainDb;
            float q;
        };

        std::vector<Band> eqBands;
    };

    static SystemProfile getProfile (SystemType type);
};
