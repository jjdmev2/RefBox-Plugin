#include "SystemSimulator.h"
#include <cmath>

SystemSimulator::SystemSimulator()
{
}

void SystemSimulator::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    configureFilters();
}

void SystemSimulator::reset()
{
    for (auto& f : filters)
    {
        f.left.reset();
        f.right.reset();
    }
    bassRolloffL.reset();
    bassRolloffR.reset();
    trebleRolloffL.reset();
    trebleRolloffR.reset();
}

void SystemSimulator::setSystemType (SystemType type)
{
    currentSystem = type;
    configureFilters();
}

void SystemSimulator::configureFilters()
{
    // Reset all filters
    for (auto& f : filters)
        f.active = false;

    hasBassRolloff = false;
    hasTrebleRolloff = false;

    if (currentSystem == SystemType::Flat)
        return;

    auto profile = getProfile (currentSystem);

    // Bass rolloff (high-pass)
    if (profile.bassRolloffFreq > 0.0f)
    {
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (
            currentSampleRate, profile.bassRolloffFreq, 0.707f);
        bassRolloffL.coefficients = coeffs;
        bassRolloffR.coefficients = coeffs;
        bassRolloffL.reset();
        bassRolloffR.reset();
        hasBassRolloff = true;
    }

    // Treble rolloff (low-pass)
    if (profile.trebleRolloffFreq > 0.0f)
    {
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (
            currentSampleRate, profile.trebleRolloffFreq, 0.707f);
        trebleRolloffL.coefficients = coeffs;
        trebleRolloffR.coefficients = coeffs;
        trebleRolloffL.reset();
        trebleRolloffR.reset();
        hasTrebleRolloff = true;
    }

    // Parametric EQ bands
    for (size_t i = 0; i < profile.eqBands.size() && i < (size_t) maxFilters; ++i)
    {
        auto& band = profile.eqBands[i];
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            currentSampleRate, band.freq, band.q,
            juce::Decibels::decibelsToGain (band.gainDb));

        filters[i].left.coefficients = coeffs;
        filters[i].right.coefficients = coeffs;
        filters[i].left.reset();
        filters[i].right.reset();
        filters[i].active = true;
    }
}

void SystemSimulator::processBlock (juce::AudioBuffer<float>& buffer)
{
    if (! isEnabled || currentSystem == SystemType::Flat)
        return;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        float left = buffer.getSample (0, i);
        float right = numChannels > 1 ? buffer.getSample (1, i) : left;

        // Bass rolloff
        if (hasBassRolloff)
        {
            left = bassRolloffL.processSample (left);
            right = bassRolloffR.processSample (right);
        }

        // Treble rolloff
        if (hasTrebleRolloff)
        {
            left = trebleRolloffL.processSample (left);
            right = trebleRolloffR.processSample (right);
        }

        // EQ bands
        for (auto& f : filters)
        {
            if (! f.active)
                break;
            left = f.left.processSample (left);
            right = f.right.processSample (right);
        }

        buffer.setSample (0, i, left);
        if (numChannels > 1)
            buffer.setSample (1, i, right);
    }
}

std::string SystemSimulator::getSystemName (SystemType type)
{
    switch (type)
    {
        case SystemType::Flat:            return "Flat (Bypass)";
        case SystemType::Laptop:          return "Laptop Speakers";
        case SystemType::Phone:           return "Phone Speaker";
        case SystemType::Earbuds:         return "Earbuds";
        case SystemType::CarStereo:       return "Car Stereo";
        case SystemType::TVSpeakers:      return "TV Speakers";
        case SystemType::ClubPA:          return "Club PA";
        case SystemType::BluetoothSmall:  return "Bluetooth Speaker";
        case SystemType::StudioNS10:      return "Yamaha NS-10";
        case SystemType::AuratoneC5:      return "Auratone / Mixcube";
        case SystemType::HomePod:         return "Smart Speaker";
        case SystemType::Headphones:      return "Consumer Headphones";
        default:                          return "Unknown";
    }
}

SystemSimulator::SystemProfile SystemSimulator::getProfile (SystemType type)
{
    SystemProfile profile;

    switch (type)
    {
        case SystemType::Laptop:
            profile.bassRolloffFreq = 200.0f;
            profile.trebleRolloffFreq = 14000.0f;
            profile.eqBands = {
                { 300.0f, 4.0f, 1.2f },     // boxy resonance
                { 800.0f, 2.0f, 0.8f },      // midrange honk
                { 2500.0f, 3.0f, 1.0f },     // presence boost (small drivers)
                { 5000.0f, -2.0f, 1.0f },    // slight dip
            };
            break;

        case SystemType::Phone:
            profile.bassRolloffFreq = 300.0f;
            profile.trebleRolloffFreq = 12000.0f;
            profile.eqBands = {
                { 400.0f, 3.0f, 1.5f },
                { 1000.0f, 4.0f, 0.8f },     // midrange emphasis
                { 3000.0f, 5.0f, 1.0f },     // high presence peak
                { 6000.0f, -3.0f, 1.2f },
            };
            break;

        case SystemType::Earbuds:
            profile.bassRolloffFreq = 30.0f;
            profile.eqBands = {
                { 100.0f, 3.0f, 0.8f },      // consumer bass boost
                { 250.0f, -1.0f, 1.0f },
                { 3000.0f, 2.0f, 1.2f },     // presence
                { 6000.0f, 3.0f, 1.0f },     // sibilance emphasis
                { 10000.0f, -2.0f, 0.8f },   // air rolloff
            };
            break;

        case SystemType::CarStereo:
            profile.bassRolloffFreq = 40.0f;
            profile.trebleRolloffFreq = 16000.0f;
            profile.eqBands = {
                { 80.0f, 4.0f, 0.7f },       // car bass boost
                { 200.0f, -2.0f, 1.0f },     // cabin resonance dip
                { 500.0f, 2.0f, 0.8f },      // dash reflection
                { 2000.0f, -3.0f, 1.0f },    // midrange dip
                { 4000.0f, 2.0f, 1.2f },
            };
            break;

        case SystemType::TVSpeakers:
            profile.bassRolloffFreq = 150.0f;
            profile.trebleRolloffFreq = 13000.0f;
            profile.eqBands = {
                { 250.0f, 2.0f, 1.0f },
                { 600.0f, 3.0f, 0.8f },      // midrange emphasis
                { 2000.0f, 2.0f, 1.0f },
                { 4000.0f, -2.0f, 1.2f },
            };
            break;

        case SystemType::ClubPA:
            profile.bassRolloffFreq = 30.0f;
            profile.eqBands = {
                { 50.0f, 5.0f, 0.8f },       // sub boost
                { 100.0f, 3.0f, 0.7f },      // bass boost
                { 400.0f, -2.0f, 1.0f },     // mud scoop
                { 2000.0f, 2.0f, 1.0f },     // presence
                { 8000.0f, -3.0f, 0.8f },    // HF rolloff from distance
                { 12000.0f, -5.0f, 0.7f },
            };
            break;

        case SystemType::BluetoothSmall:
            profile.bassRolloffFreq = 120.0f;
            profile.trebleRolloffFreq = 15000.0f;
            profile.eqBands = {
                { 200.0f, 3.0f, 1.0f },      // psychoacoustic bass attempt
                { 500.0f, 2.0f, 0.8f },
                { 2000.0f, 3.0f, 1.0f },     // presence boost
                { 5000.0f, -1.0f, 1.2f },
            };
            break;

        case SystemType::StudioNS10:
            profile.bassRolloffFreq = 60.0f;
            profile.eqBands = {
                { 100.0f, -2.0f, 0.8f },
                { 2000.0f, 3.0f, 0.6f },     // famous midrange presence
                { 4000.0f, 2.0f, 1.0f },
                { 8000.0f, -4.0f, 0.7f },    // harsh treble rolloff
                { 12000.0f, -6.0f, 0.8f },
            };
            break;

        case SystemType::AuratoneC5:
            profile.bassRolloffFreq = 150.0f;
            profile.trebleRolloffFreq = 10000.0f;
            profile.eqBands = {
                { 300.0f, 2.0f, 0.8f },
                { 1000.0f, 3.0f, 0.6f },     // very midrange-forward
                { 3000.0f, 2.0f, 1.0f },
            };
            break;

        case SystemType::HomePod:
            profile.bassRolloffFreq = 50.0f;
            profile.trebleRolloffFreq = 16000.0f;
            profile.eqBands = {
                { 80.0f, 4.0f, 0.8f },       // DSP bass enhancement
                { 200.0f, 1.0f, 1.0f },
                { 1000.0f, -1.0f, 0.8f },
                { 3000.0f, 2.0f, 1.0f },
                { 6000.0f, 1.0f, 1.2f },
            };
            break;

        case SystemType::Headphones:
            profile.eqBands = {
                { 60.0f, 3.0f, 0.7f },       // consumer bass boost
                { 200.0f, 1.0f, 1.0f },
                { 1000.0f, -1.0f, 0.8f },    // slight mid scoop
                { 3000.0f, 2.0f, 1.0f },     // Harman target presence
                { 6000.0f, 3.0f, 1.2f },     // treble emphasis
                { 10000.0f, 1.0f, 0.8f },
            };
            break;

        default:
            break;
    }

    return profile;
}

std::array<float, 128> SystemSimulator::getResponseCurve() const
{
    std::array<float, 128> response{};

    if (currentSystem == SystemType::Flat || ! isEnabled)
    {
        response.fill (0.0f);
        return response;
    }

    // Calculate frequency response by evaluating filter chain at each band frequency
    const float minFreq = 20.0f;
    const float maxFreq = 20000.0f;
    const float logMin = std::log10 (minFreq);
    const float logMax = std::log10 (maxFreq);

    for (int i = 0; i < 128; ++i)
    {
        float freq = std::pow (10.0f, logMin + (logMax - logMin) * ((float) i + 0.5f) / 128.0f);
        double omega = 2.0 * juce::MathConstants<double>::pi * (double) freq / currentSampleRate;

        double totalGainDb = 0.0;

        // Evaluate each active filter
        auto evaluateFilter = [omega] (const juce::dsp::IIR::Filter<float>& filter) -> double
        {
            auto* coeffs = filter.coefficients.get();
            if (coeffs == nullptr)
                return 0.0;

            auto& c = coeffs->coefficients;
            if (c.size() < 5)
                return 0.0;

            // H(z) = (b0 + b1*z^-1 + b2*z^-2) / (a0 + a1*z^-1 + a2*z^-2)
            double cosw = std::cos (omega);
            double sinw = std::sin (omega);

            double realNum = (double) c[0] + (double) c[1] * cosw + (double) c[2] * std::cos (2.0 * omega);
            double imagNum = -(double) c[1] * sinw - (double) c[2] * std::sin (2.0 * omega);

            // a0 is normalized to 1.0 in JUCE IIR coefficients
            double realDen = 1.0 + (double) c[3] * cosw + (double) c[4] * std::cos (2.0 * omega);
            double imagDen = -(double) c[3] * sinw - (double) c[4] * std::sin (2.0 * omega);

            double magNum = std::sqrt (realNum * realNum + imagNum * imagNum);
            double magDen = std::sqrt (realDen * realDen + imagDen * imagDen);

            if (magDen < 1e-10)
                return 0.0;

            return 20.0 * std::log10 (magNum / magDen);
        };

        if (hasBassRolloff)
            totalGainDb += evaluateFilter (bassRolloffL);

        if (hasTrebleRolloff)
            totalGainDb += evaluateFilter (trebleRolloffL);

        for (const auto& f : filters)
        {
            if (! f.active)
                break;
            totalGainDb += evaluateFilter (f.left);
        }

        response[(size_t) i] = (float) totalGainDb;
    }

    return response;
}
