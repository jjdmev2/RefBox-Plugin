#include "PluginEditor.h"

RefBoxEditor::RefBoxEditor (RefBoxProcessor& p)
    : AudioProcessorEditor (&p),
      refBoxProcessor (p),
      spectrumDisplay (p.getMixAnalyzer(), p.getReferenceAnalyzer(),
                       p.getCurveManager(), p.getEQMatcher(), p.getSystemSim()),
      topBar (p, spectrumDisplay),
      waveformDisplay (p),
      waveformMicroscope (p),
      referenceGrid (p),
      systemSimPanel (p.getSystemSim()),
      meterPanel (p.getMixMeter(), p.getReferenceMeter(), p.getEQMatcher()),
      abButton (p)
{
    setLookAndFeel (&lookAndFeel);

    setSize (defaultWidth, defaultHeight);
    setResizable (true, true);
    setResizeLimits (800, 500, 3840, 2160);

    // Wire kitten easter egg pointer
    spectrumDisplay.setKittenModePtr (p.getKittenModePtr());

    addAndMakeVisible (topBar);
    addAndMakeVisible (spectrumDisplay);
    addAndMakeVisible (waveformDisplay);
    addAndMakeVisible (waveformMicroscope);
    addAndMakeVisible (referenceGrid);
    addAndMakeVisible (systemSimPanel);
    addAndMakeVisible (meterPanel);
    addAndMakeVisible (abButton);
}

RefBoxEditor::~RefBoxEditor()
{
    setLookAndFeel (nullptr);
}

void RefBoxEditor::paint (juce::Graphics& g)
{
    // Metallic dark background
    juce::ColourGradient bgGrad (
        juce::Colour (0xFF111116), 0.0f, 0.0f,
        juce::Colour (0xFF0A0A0D), 0.0f, (float) getHeight(), false);
    g.setGradientFill (bgGrad);
    g.fillAll();

    // Subtle top accent line
    g.setColour (RefBoxColours::accent.withAlpha (0.35f));
    g.fillRect (0, 0, getWidth(), 2);
}

void RefBoxEditor::resized()
{
    auto bounds = getLocalBounds();

    bounds.removeFromTop (2); // accent line

    // Top bar
    int topH = std::max (36, (int) (bounds.getHeight() * 0.055f));
    topBar.setBounds (bounds.removeFromTop (topH));

    auto content = bounds.reduced (6, 4);

    // Right side: meters + system sim stacked
    int rightW = std::max (140, (int) (content.getWidth() * 0.15f));
    auto rightPanel = content.removeFromRight (rightW);
    content.removeFromRight (4);

    int simH = (int) (rightPanel.getHeight() * 0.42f);
    systemSimPanel.setBounds (rightPanel.removeFromBottom (simH).reduced (0, 2));
    rightPanel.removeFromBottom (4);
    meterPanel.setBounds (rightPanel.reduced (0, 2));

    // Bottom section: A/B button on left + reference grid
    int bottomH = std::max (110, (int) (content.getHeight() * 0.20f));
    auto bottomArea = content.removeFromBottom (bottomH);

    // A/B button - round, bottom-left
    int abSize = std::min (bottomH, 130);
    abButton.setBounds (bottomArea.getX(), bottomArea.getY() + (bottomH - abSize) / 2,
                        abSize, abSize);
    bottomArea.removeFromLeft (abSize + 6);

    referenceGrid.setBounds (bottomArea.reduced (0, 2));

    content.removeFromBottom (3);

    // Waveform strip
    int waveH = std::max (50, (int) (content.getHeight() * 0.13f));
    waveformDisplay.setBounds (content.removeFromBottom (waveH).reduced (0, 2));

    content.removeFromBottom (2);

    // Waveform microscope (CDJ-style zoomed transient view)
    int microH = std::max (36, (int) (content.getHeight() * 0.10f));
    waveformMicroscope.setBounds (content.removeFromBottom (microH).reduced (0, 1));

    content.removeFromBottom (2);

    // Spectrum display takes the rest
    spectrumDisplay.setBounds (content.reduced (0, 2));
}
