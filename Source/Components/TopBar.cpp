#include "TopBar.h"
#include "SpectrumDisplay.h"
#include "../PluginProcessor.h"

TopBar::TopBar (RefBoxProcessor& p, SpectrumDisplay& sd)
    : processor (p), spectrumDisplay (sd)
{
    addAndMakeVisible (refCurveBtn);
    addAndMakeVisible (deltaBtn);
    addAndMakeVisible (sysCurveBtn);
    addAndMakeVisible (lufsMatchBtn);
    addAndMakeVisible (playRefBtn);
    addAndMakeVisible (resetBtn);
    addAndMakeVisible (spectrumModeSelector);
    addAndMakeVisible (genreSelector);
    addAndMakeVisible (colorSchemeSelector);

    styleToggle (refCurveBtn, showRef);
    styleToggle (deltaBtn, showDelta);
    styleToggle (sysCurveBtn, showSys);

    refCurveBtn.onClick = [this] {
        showRef = ! showRef;
        spectrumDisplay.setShowReference (showRef);
        styleToggle (refCurveBtn, showRef);
    };

    deltaBtn.onClick = [this] {
        showDelta = ! showDelta;
        spectrumDisplay.setShowDelta (showDelta);
        styleToggle (deltaBtn, showDelta);
    };

    sysCurveBtn.onClick = [this] {
        showSys = ! showSys;
        spectrumDisplay.setShowSystemResponse (showSys);
        styleToggle (sysCurveBtn, showSys);
    };

    lufsMatchBtn.onClick = [this] {
        processor.setLoudnessMatchEnabled (! processor.isLoudnessMatchEnabled());
    };

    playRefBtn.onClick = [this] {
        processor.setPlayingReference (! processor.isPlayingReference());
    };

    resetBtn.onClick = [this] {
        processor.resetMeters();
    };

    // Spectrum mode selector
    spectrumModeSelector.addItem ("Fast", 1);
    spectrumModeSelector.addItem ("Average", 2);
    spectrumModeSelector.addItem ("Smooth", 3);
    spectrumModeSelector.addItem ("Peak", 4);
    spectrumModeSelector.setSelectedId (2); // Average is default
    spectrumModeSelector.onChange = [this] {
        switch (spectrumModeSelector.getSelectedId())
        {
            case 1: spectrumDisplay.setSpectrumMode (SpectrumMode::Fast);    break;
            case 2: spectrumDisplay.setSpectrumMode (SpectrumMode::Average); break;
            case 3: spectrumDisplay.setSpectrumMode (SpectrumMode::Smooth);  break;
            case 4: spectrumDisplay.setSpectrumMode (SpectrumMode::Peak);    break;
            default: break;
        }
    };

    // Color scheme
    colorSchemeSelector.addItem ("Classic", 1);
    colorSchemeSelector.addItem ("Pioneer RGB", 2);
    colorSchemeSelector.addItem ("Amber", 3);
    colorSchemeSelector.addItem ("Purple", 4);
    colorSchemeSelector.addItem ("Fire", 5);
    colorSchemeSelector.setSelectedId (1);
    colorSchemeSelector.onChange = [this] {
        switch (colorSchemeSelector.getSelectedId())
        {
            case 1: spectrumDisplay.setColorScheme (VisualizerColorScheme::Classic);    break;
            case 2: spectrumDisplay.setColorScheme (VisualizerColorScheme::PioneerRGB); break;
            case 3: spectrumDisplay.setColorScheme (VisualizerColorScheme::Amber);      break;
            case 4: spectrumDisplay.setColorScheme (VisualizerColorScheme::Purple);     break;
            case 5: spectrumDisplay.setColorScheme (VisualizerColorScheme::Fire);       break;
            default: break;
        }
    };

    // Genre presets
    genreSelector.addItem ("Genre...", 1);
    auto genres = ReferenceCurveManager::getAvailableGenres();
    for (int i = 0; i < (int) genres.size(); ++i)
        genreSelector.addItem (genres[(size_t) i], i + 2);
    genreSelector.setSelectedId (1);
    genreSelector.onChange = [this] {
        int idx = genreSelector.getSelectedId() - 2;
        auto g2 = ReferenceCurveManager::getAvailableGenres();
        if (idx >= 0 && idx < (int) g2.size())
        {
            auto curve = ReferenceCurveManager::getGenrePreset (g2[(size_t) idx]);
            processor.getCurveManager().addCurve (curve);
            processor.getCurveManager().setActiveCurveIndex (
                (int) processor.getCurveManager().getCurves().size() - 1);
        }
    };

    startTimerHz (10);
}

void TopBar::styleToggle (juce::TextButton& btn, bool active)
{
    btn.setColour (juce::TextButton::buttonColourId,
                   active ? RefBoxColours::accent.withAlpha (0.15f) : juce::Colour (0xFF1A1A22));
    btn.setColour (juce::TextButton::textColourOffId,
                   active ? RefBoxColours::accent : RefBoxColours::textDim);
}

void TopBar::timerCallback()
{
    bool lufsOn = processor.isLoudnessMatchEnabled();
    lufsMatchBtn.setButtonText (lufsOn ? "LUFS ON" : "LUFS");
    styleToggle (lufsMatchBtn, lufsOn);

    bool playing = processor.isPlayingReference();
    playRefBtn.setButtonText (playing ? "STOP" : "PLAY");
    playRefBtn.setColour (juce::TextButton::buttonColourId,
                          playing ? RefBoxColours::danger.withAlpha (0.25f) : juce::Colour (0xFF1A1A22));
    playRefBtn.setColour (juce::TextButton::textColourOffId,
                          playing ? RefBoxColours::danger : RefBoxColours::textSecondary);
}

void TopBar::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    int h = getHeight();

    // Metallic gradient background
    juce::ColourGradient bgGrad (
        juce::Colour (0xFF1C1C22), 0.0f, 0.0f,
        juce::Colour (0xFF101014), 0.0f, bounds.getBottom(), false);
    g.setGradientFill (bgGrad);
    g.fillRect (bounds);

    // Top edge highlight (metallic sheen)
    g.setColour (juce::Colour (0xFF303040).withAlpha (0.5f));
    g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), 1.0f);

    // Bottom border
    g.setColour (RefBoxColours::panelBorder);
    g.fillRect (bounds.getX(), bounds.getBottom() - 1.0f, bounds.getWidth(), 1.0f);

    // Logo
    g.setColour (RefBoxColours::accent);
    g.setFont (juce::FontOptions ((float) h * 0.45f, juce::Font::bold));
    g.drawText ("RefBox", 14, 0, 120, h, juce::Justification::centredLeft);
}

void TopBar::mouseDown (const juce::MouseEvent& e)
{
    // Easter egg: clicking the logo area
    if (e.x < 140)
    {
        if (processor.isKittenMode())
        {
            // Single click dismisses the kitten
            processor.setKittenMode (false);
            logoClickCount = 0;
            return;
        }

        auto now = juce::Time::getMillisecondCounter();
        if (now - lastLogoClickTime > 5000)
            logoClickCount = 0;

        lastLogoClickTime = now;
        logoClickCount++;

        if (logoClickCount >= 10)
        {
            processor.setKittenMode (true);
            logoClickCount = 0;
        }
    }
}

void TopBar::resized()
{
    int h = getHeight();
    int btnH = (int) (h * 0.6f);
    int y = (h - btnH) / 2;
    int btnW = std::max (52, (int) (getWidth() * 0.055f));
    int gap = 3;

    // Left side: logo (skip 140px) + toggle buttons + spectrum mode
    int leftX = 140;
    refCurveBtn.setBounds (leftX, y, btnW, btnH);
    deltaBtn.setBounds (leftX + (btnW + gap), y, btnW, btnH);
    sysCurveBtn.setBounds (leftX + (btnW + gap) * 2, y, btnW + 10, btnH);
    spectrumModeSelector.setBounds (leftX + (btnW + gap) * 3 + 10, y, 80, btnH);
    colorSchemeSelector.setBounds (leftX + (btnW + gap) * 3 + 94, y, 100, btnH);

    // Right side: genre, lufs match, reset, play
    int rightX = getWidth() - 6;
    rightX -= btnW;
    playRefBtn.setBounds (rightX, y, btnW, btnH);
    rightX -= (36 + gap);
    resetBtn.setBounds (rightX, y, 36, btnH);
    rightX -= (btnW + gap);
    lufsMatchBtn.setBounds (rightX, y, btnW, btnH);
    rightX -= (100 + gap);
    genreSelector.setBounds (rightX, y, 100, btnH);
}
