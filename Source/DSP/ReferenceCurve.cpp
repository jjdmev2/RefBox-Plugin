#include "ReferenceCurve.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cmath>

std::vector<ReferenceCurveData> ReferenceCurveManager::genrePresets;
bool ReferenceCurveManager::presetsInitialized = false;

ReferenceCurveManager::ReferenceCurveManager()
{
    if (! presetsInitialized)
        initGenrePresets();
}

void ReferenceCurveManager::startCapture (const std::string& name)
{
    currentCapture = ReferenceCurveData();
    currentCapture.name = name;
    currentCapture.runningSum.fill (0.0);
    currentCapture.averageMagnitudes.fill (-100.0f);
    currentCapture.maxMagnitudes.fill (-100.0f);
    isCapturing = true;
}

void ReferenceCurveManager::captureFrame (const std::array<float, SpectralAnalyzer::numBands>& magnitudes)
{
    if (! isCapturing)
        return;

    currentCapture.numFramesAnalyzed++;

    for (int i = 0; i < SpectralAnalyzer::numBands; ++i)
    {
        currentCapture.runningSum[(size_t) i] += (double) magnitudes[(size_t) i];
        currentCapture.averageMagnitudes[(size_t) i] =
            (float) (currentCapture.runningSum[(size_t) i] / (double) currentCapture.numFramesAnalyzed);

        if (magnitudes[(size_t) i] > currentCapture.maxMagnitudes[(size_t) i])
            currentCapture.maxMagnitudes[(size_t) i] = magnitudes[(size_t) i];
    }
}

ReferenceCurveData ReferenceCurveManager::finishCapture()
{
    isCapturing = false;
    return currentCapture;
}

void ReferenceCurveManager::analyzeFile (const juce::File& audioFile, double sampleRate)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (audioFile));
    if (reader == nullptr)
        return;

    SpectralAnalyzer analyzer;
    analyzer.prepare (reader->sampleRate, SpectralAnalyzer::fftSize);

    const int blockSize = SpectralAnalyzer::fftSize;
    juce::AudioBuffer<float> buffer (1, blockSize);

    juce::int64 totalSamples = reader->lengthInSamples;
    juce::int64 position = 0;

    while (position < totalSamples)
    {
        int samplesToRead = (int) std::min ((juce::int64) blockSize, totalSamples - position);
        reader->read (&buffer, 0, samplesToRead, position, true, false);

        // Mix to mono if stereo
        if (reader->numChannels > 1)
        {
            juce::AudioBuffer<float> stereoBuffer ((int) reader->numChannels, samplesToRead);
            reader->read (&stereoBuffer, 0, samplesToRead, position, true, true);

            for (int i = 0; i < samplesToRead; ++i)
            {
                float sum = 0.0f;
                for (int ch = 0; ch < (int) reader->numChannels; ++ch)
                    sum += stereoBuffer.getSample (ch, i);
                buffer.setSample (0, i, sum / (float) reader->numChannels);
            }
        }

        analyzer.pushSamples (buffer.getReadPointer (0), samplesToRead);
        analyzer.processFFT();

        if (analyzer.isFFTDataReady() == false) // FFT was processed
            captureFrame (analyzer.getBandMagnitudes());

        position += samplesToRead;
    }
}

ReferenceCurveData ReferenceCurveManager::analyzeFolder (const juce::File& folder, double sampleRate)
{
    startCapture (folder.getFileName().toStdString());

    auto files = folder.findChildFiles (juce::File::findFiles, false, "*.wav;*.aiff;*.aif;*.mp3;*.flac");

    for (const auto& file : files)
        analyzeFile (file, sampleRate);

    return finishCapture();
}

void ReferenceCurveManager::addCurve (const ReferenceCurveData& curve)
{
    loadedCurves.push_back (curve);
    if (activeCurveIndex < 0)
        activeCurveIndex = 0;
}

void ReferenceCurveManager::removeCurve (int index)
{
    if (index >= 0 && index < (int) loadedCurves.size())
    {
        loadedCurves.erase (loadedCurves.begin() + index);
        if (activeCurveIndex >= (int) loadedCurves.size())
            activeCurveIndex = (int) loadedCurves.size() - 1;
    }
}

const ReferenceCurveData* ReferenceCurveManager::getActiveCurve() const
{
    if (activeCurveIndex >= 0 && activeCurveIndex < (int) loadedCurves.size())
        return &loadedCurves[(size_t) activeCurveIndex];
    return nullptr;
}

void ReferenceCurveManager::saveCurve (const ReferenceCurveData& curve, const juce::File& file)
{
    juce::var data;
    data.append (juce::String (curve.name));
    data.append (curve.numFramesAnalyzed);

    juce::var avgArray;
    juce::var maxArray;

    for (int i = 0; i < SpectralAnalyzer::numBands; ++i)
    {
        avgArray.append (curve.averageMagnitudes[(size_t) i]);
        maxArray.append (curve.maxMagnitudes[(size_t) i]);
    }

    auto obj = std::make_unique<juce::DynamicObject>();
    obj->setProperty ("name", juce::String (curve.name));
    obj->setProperty ("frames", curve.numFramesAnalyzed);
    obj->setProperty ("average", avgArray);
    obj->setProperty ("maximum", maxArray);

    juce::var json (obj.release());
    file.replaceWithText (juce::JSON::toString (json));
}

ReferenceCurveData ReferenceCurveManager::loadCurve (const juce::File& file)
{
    ReferenceCurveData curve;
    auto json = juce::JSON::parse (file.loadFileAsString());

    if (auto* obj = json.getDynamicObject())
    {
        curve.name = obj->getProperty ("name").toString().toStdString();
        curve.numFramesAnalyzed = (int) obj->getProperty ("frames");

        auto avg = obj->getProperty ("average");
        auto maxVals = obj->getProperty ("maximum");

        if (avg.isArray() && maxVals.isArray())
        {
            for (int i = 0; i < SpectralAnalyzer::numBands && i < avg.size(); ++i)
            {
                curve.averageMagnitudes[(size_t) i] = (float) avg[i];
                curve.maxMagnitudes[(size_t) i] = (float) maxVals[i];
            }
        }
    }

    return curve;
}

ReferenceCurveData ReferenceCurveManager::getGenrePreset (const std::string& genre)
{
    if (! presetsInitialized)
        initGenrePresets();

    for (const auto& preset : genrePresets)
    {
        if (preset.name == genre)
            return preset;
    }

    return ReferenceCurveData();
}

std::vector<std::string> ReferenceCurveManager::getAvailableGenres()
{
    if (! presetsInitialized)
        initGenrePresets();

    std::vector<std::string> names;
    for (const auto& preset : genrePresets)
        names.push_back (preset.name);
    return names;
}

void ReferenceCurveManager::initGenrePresets()
{
    presetsInitialized = true;

    // These are approximate spectral profiles based on genre characteristics.
    // In a commercial product, these would be generated from analyzing hundreds
    // of reference tracks per genre.

    auto makePreset = [] (const std::string& name,
                          float subBass, float bass, float lowMid,
                          float mid, float upperMid, float presence,
                          float brilliance, float air) -> ReferenceCurveData
    {
        ReferenceCurveData curve;
        curve.name = name;
        curve.numFramesAnalyzed = 1;

        // Interpolate across bands
        // Band mapping: 0-15=sub, 16-31=bass, 32-47=lowMid, 48-63=mid,
        //               64-79=upperMid, 80-95=presence, 96-111=brilliance, 112-127=air
        float targets[8] = { subBass, bass, lowMid, mid, upperMid, presence, brilliance, air };

        for (int i = 0; i < SpectralAnalyzer::numBands; ++i)
        {
            float pos = (float) i / (float) SpectralAnalyzer::numBands * 7.0f;
            int idx = std::min ((int) pos, 6);
            float frac = pos - (float) idx;

            float val = targets[idx] * (1.0f - frac) + targets[idx + 1] * frac;
            curve.averageMagnitudes[(size_t) i] = val;
            curve.maxMagnitudes[(size_t) i] = val + 6.0f; // max is typically ~6dB above average
        }

        return curve;
    };

    // Raw spectral magnitudes matching real FFT output levels.
    // After 3.0 dB/octave display weighting, each genre shows its true character:
    // e.g. Techno visually shows heavy bass, Metal shows aggressive mids.
    //
    //                                           sub   bass  lmid  mid   umid  pres  bril  air
    genrePresets.push_back (makePreset ("Pop",    -24,  -21,  -26,  -28,  -33,  -38,  -49,  -59));
    genrePresets.push_back (makePreset ("Hip Hop",-10,  -15,  -30,  -30,  -35,  -42,  -51,  -61));
    genrePresets.push_back (makePreset ("EDM",    -10,  -13,  -26,  -32,  -33,  -36,  -47,  -59));
    genrePresets.push_back (makePreset ("D&B",    -10,  -15,  -30,  -32,  -33,  -36,  -45,  -57));
    genrePresets.push_back (makePreset ("House",  -12,  -15,  -26,  -32,  -35,  -38,  -49,  -59));
    genrePresets.push_back (makePreset ("Dubstep", -6,  -11,  -22,  -30,  -33,  -38,  -49,  -61));
    genrePresets.push_back (makePreset ("Rock",   -22,  -19,  -22,  -26,  -29,  -36,  -49,  -61));
    genrePresets.push_back (makePreset ("Metal",  -24,  -21,  -20,  -22,  -27,  -34,  -47,  -59));
    genrePresets.push_back (makePreset ("Jazz",   -26,  -23,  -26,  -28,  -35,  -42,  -53,  -65));
    genrePresets.push_back (makePreset ("Classical",-30, -27,  -28,  -30,  -37,  -44,  -55,  -67));
    genrePresets.push_back (makePreset ("R&B",    -12,  -17,  -26,  -28,  -33,  -40,  -51,  -61));
    genrePresets.push_back (makePreset ("Trap",    -6,  -13,  -32,  -34,  -35,  -36,  -47,  -59));
    genrePresets.push_back (makePreset ("Techno", -10,  -15,  -28,  -34,  -33,  -38,  -47,  -59));
    genrePresets.push_back (makePreset ("Ambient",-24,  -25,  -28,  -30,  -35,  -38,  -45,  -51));
    genrePresets.push_back (makePreset ("Reggaeton",-8, -13,  -26,  -32,  -35,  -38,  -49,  -59));
    genrePresets.push_back (makePreset ("Latin",  -16,  -17,  -24,  -28,  -33,  -38,  -49,  -61));
    genrePresets.push_back (makePreset ("Indie",  -24,  -21,  -24,  -26,  -31,  -38,  -49,  -61));
}
