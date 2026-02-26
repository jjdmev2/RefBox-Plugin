#include "PluginProcessor.h"
#include "PluginEditor.h"

RefBoxProcessor::RefBoxProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats();
}

RefBoxProcessor::~RefBoxProcessor()
{
    for (auto& ref : referenceTracks)
    {
        if (ref.transport)
            ref.transport->setSource (nullptr);
    }
}

void RefBoxProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mixAnalyzer.prepare (sampleRate, samplesPerBlock);
    referenceAnalyzer.prepare (sampleRate, samplesPerBlock);
    mixMeter.prepare (sampleRate, samplesPerBlock, 2);
    referenceMeter.prepare (sampleRate, samplesPerBlock, 2);
    systemSimulator.prepare (sampleRate, samplesPerBlock);

    referenceBuffer.setSize (2, samplesPerBlock);
    mixCopy.setSize (2, samplesPerBlock);

    for (auto& ref : referenceTracks)
    {
        if (ref.transport)
            ref.transport->prepareToPlay (samplesPerBlock, sampleRate);
    }
}

void RefBoxProcessor::releaseResources()
{
    for (auto& ref : referenceTracks)
    {
        if (ref.transport)
            ref.transport->releaseResources();
    }
}

bool RefBoxProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void RefBoxProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Store a copy of the mix for analysis
    mixCopy.setSize (numChannels, numSamples, false, false, true);
    for (int ch = 0; ch < numChannels; ++ch)
        mixCopy.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    // Analyze the mix signal
    {
        // Mix to mono for spectral analysis
        float monoSample;
        for (int i = 0; i < numSamples; ++i)
        {
            monoSample = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                monoSample += buffer.getSample (ch, i);
            monoSample /= (float) numChannels;

            // Push to analyzer one sample at a time
            mixAnalyzer.pushSamples (&monoSample, 1);
        }

        mixAnalyzer.processFFT();
        mixMeter.processBlock (buffer);
    }

    // Handle reference track playback
    if (playingReference.load())
    {
        const juce::ScopedTryLock stl (trackLock);

        if (stl.isLocked())
        {
            int refIdx = activeReferenceIndex.load();

            if (refIdx >= 0 && refIdx < (int) referenceTracks.size())
            {
                auto& ref = referenceTracks[(size_t) refIdx];

                if (ref.transport && ref.transport->isPlaying())
                {
                    referenceBuffer.setSize (numChannels, numSamples, false, false, true);
                    referenceBuffer.clear();

                    juce::AudioSourceChannelInfo info (&referenceBuffer, 0, numSamples);
                    ref.transport->getNextAudioBlock (info);

                    // Analyze reference
                    for (int i = 0; i < numSamples; ++i)
                    {
                        float monoRef = 0.0f;
                        for (int ch = 0; ch < std::min (numChannels, referenceBuffer.getNumChannels()); ++ch)
                            monoRef += referenceBuffer.getSample (ch, i);
                        monoRef /= (float) numChannels;
                        referenceAnalyzer.pushSamples (&monoRef, 1);
                    }
                    referenceAnalyzer.processFFT();
                    referenceMeter.processBlock (referenceBuffer);

                    // Only output reference audio when A/B is active
                    if (abActive)
                    {
                        for (int ch = 0; ch < numChannels; ++ch)
                            buffer.copyFrom (ch, 0, referenceBuffer,
                                             std::min (ch, referenceBuffer.getNumChannels() - 1),
                                             0, numSamples);

                        // Loudness match: adjust ref level to match mix
                        if (loudnessMatchEnabled)
                        {
                            float mLUFS = mixMeter.getIntegratedLUFS();
                            float rLUFS = referenceMeter.getIntegratedLUFS();

                            if (mLUFS > -100.0f && rLUFS > -100.0f)
                            {
                                float gainDb = mLUFS - rLUFS;
                                float gain = juce::Decibels::decibelsToGain (gainDb);
                                buffer.applyGain (gain);
                            }
                        }
                    }
                }
            }
        }
        // If lock not acquired, skip reference playback this block (inaudible gap)
    }

    // EQ matching - compare mix spectrum against active reference curve
    if (auto* activeCurve = curveManager.getActiveCurve())
    {
        eqMatcher.calculate (mixAnalyzer.getBandMagnitudes(), *activeCurve);
    }

    // Apply system simulation (post-analysis, so we analyze the true mix)
    systemSimulator.processBlock (buffer);
}

void RefBoxProcessor::loadReferenceTrack (const juce::File& file)
{
    auto reader = formatManager.createReaderFor (file);
    if (reader == nullptr)
        return;

    ReferenceTrack track;
    track.name = file.getFileNameWithoutExtension();
    track.file = file;
    track.readerSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
    track.transport = std::make_unique<juce::AudioTransportSource>();
    track.transport->setSource (track.readerSource.get(), 0, nullptr, reader->sampleRate);

    if (getSampleRate() > 0)
        track.transport->prepareToPlay (getBlockSize(), getSampleRate());

    {
        const juce::ScopedLock sl (trackLock);
        referenceTracks.push_back (std::move (track));

        if (activeReferenceIndex.load() < 0)
            activeReferenceIndex = 0;
    }
}

void RefBoxProcessor::removeReferenceTrack (int index)
{
    const juce::ScopedLock sl (trackLock);

    if (index >= 0 && index < (int) referenceTracks.size())
    {
        if (referenceTracks[(size_t) index].transport)
            referenceTracks[(size_t) index].transport->setSource (nullptr);

        referenceTracks.erase (referenceTracks.begin() + index);

        if (activeReferenceIndex.load() >= (int) referenceTracks.size())
            activeReferenceIndex = (int) referenceTracks.size() - 1;
    }
}

void RefBoxProcessor::setActiveReference (int index)
{
    const juce::ScopedLock sl (trackLock);

    if (index < 0 || index >= (int) referenceTracks.size())
        return;

    if (index == activeReferenceIndex.load())
        return;

    bool wasPlaying = false;

    // Stop the old transport
    int oldIdx = activeReferenceIndex.load();
    if (oldIdx >= 0 && oldIdx < (int) referenceTracks.size())
    {
        auto& oldRef = referenceTracks[(size_t) oldIdx];
        if (oldRef.transport)
        {
            wasPlaying = oldRef.transport->isPlaying();
            oldRef.transport->stop();
        }
    }

    activeReferenceIndex = index;

    // Start the new transport if we were playing (or if playingReference is on)
    if (playingReference.load() || wasPlaying)
    {
        auto& newRef = referenceTracks[(size_t) index];
        if (newRef.transport)
        {
            newRef.transport->setPosition (0.0);
            newRef.transport->start();
            playingReference = true;
        }
    }
}

void RefBoxProcessor::setPlayingReference (bool shouldPlay)
{
    const juce::ScopedLock sl (trackLock);
    playingReference = shouldPlay;

    int refIdx = activeReferenceIndex.load();
    if (refIdx >= 0 && refIdx < (int) referenceTracks.size())
    {
        auto& ref = referenceTracks[(size_t) refIdx];
        if (ref.transport)
        {
            if (shouldPlay)
            {
                ref.transport->setPosition (0.0);
                ref.transport->start();
            }
            else
            {
                ref.transport->stop();
            }
        }
    }
}

double RefBoxProcessor::getReferencePosition() const
{
    const juce::ScopedLock sl (trackLock);
    int refIdx = activeReferenceIndex.load();
    if (refIdx >= 0 && refIdx < (int) referenceTracks.size())
    {
        auto& ref = referenceTracks[(size_t) refIdx];
        if (ref.transport)
        {
            double len = ref.transport->getLengthInSeconds();
            if (len > 0.0)
                return ref.transport->getCurrentPosition() / len;
        }
    }
    return 0.0;
}

double RefBoxProcessor::getReferenceLengthSeconds() const
{
    const juce::ScopedLock sl (trackLock);
    int refIdx = activeReferenceIndex.load();
    if (refIdx >= 0 && refIdx < (int) referenceTracks.size())
    {
        auto& ref = referenceTracks[(size_t) refIdx];
        if (ref.transport)
            return ref.transport->getLengthInSeconds();
    }
    return 0.0;
}

void RefBoxProcessor::seekReference (double normalizedPosition)
{
    const juce::ScopedLock sl (trackLock);
    int refIdx = activeReferenceIndex.load();
    if (refIdx >= 0 && refIdx < (int) referenceTracks.size())
    {
        auto& ref = referenceTracks[(size_t) refIdx];
        if (ref.transport)
        {
            double len = ref.transport->getLengthInSeconds();
            double pos = juce::jlimit (0.0, len, normalizedPosition * len);
            ref.transport->setPosition (pos);

            if (! ref.transport->isPlaying())
            {
                playingReference = true;
                ref.transport->start();
            }
        }
    }
}

juce::AudioFormatReader* RefBoxProcessor::getActiveReferenceReader() const
{
    const juce::ScopedLock sl (trackLock);
    int refIdx = activeReferenceIndex.load();
    if (refIdx >= 0 && refIdx < (int) referenceTracks.size())
    {
        auto& ref = referenceTracks[(size_t) refIdx];
        if (ref.readerSource)
            return ref.readerSource->getAudioFormatReader();
    }
    return nullptr;
}

juce::AudioProcessorEditor* RefBoxProcessor::createEditor()
{
    return new RefBoxEditor (*this);
}

void RefBoxProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = juce::ValueTree ("RefBoxState");

    state.setProperty ("systemType", (int) systemSimulator.getSystemType(), nullptr);
    state.setProperty ("systemEnabled", systemSimulator.getEnabled(), nullptr);
    state.setProperty ("loudnessMatch", loudnessMatchEnabled, nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RefBoxProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr)
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.isValid())
        {
            systemSimulator.setSystemType (
                (SystemSimulator::SystemType) (int) state.getProperty ("systemType", 0));
            systemSimulator.setEnabled (state.getProperty ("systemEnabled", false));
            loudnessMatchEnabled = state.getProperty ("loudnessMatch", true);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RefBoxProcessor();
}
