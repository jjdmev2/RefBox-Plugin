#include "ReferencePanel.h"

ReferencePanel::ReferencePanel (RefBoxProcessor& p)
    : processor (p), listModel (p)
{
    addAndMakeVisible (loadButton);
    addAndMakeVisible (loadFolderButton);
    addAndMakeVisible (abButton);
    addAndMakeVisible (loudnessMatchButton);
    addAndMakeVisible (playRefButton);
    addAndMakeVisible (referenceList);
    addAndMakeVisible (genreSelector);

    loadButton.addListener (this);
    loadFolderButton.addListener (this);
    abButton.addListener (this);
    loudnessMatchButton.addListener (this);
    playRefButton.addListener (this);

    referenceList.setModel (&listModel);
    referenceList.setRowHeight (24);
    referenceList.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xFF1A1A1A));

    // Genre presets
    genreSelector.addItem ("-- Genre Preset --", 1);
    auto genres = ReferenceCurveManager::getAvailableGenres();
    for (int i = 0; i < (int) genres.size(); ++i)
        genreSelector.addItem (genres[(size_t) i], i + 2);

    genreSelector.setSelectedId (1);
    genreSelector.onChange = [this]
    {
        int idx = genreSelector.getSelectedId() - 2;
        auto genres = ReferenceCurveManager::getAvailableGenres();
        if (idx >= 0 && idx < (int) genres.size())
        {
            auto curve = ReferenceCurveManager::getGenrePreset (genres[(size_t) idx]);
            processor.getCurveManager().addCurve (curve);
            processor.getCurveManager().setActiveCurveIndex (
                (int) processor.getCurveManager().getCurves().size() - 1);
        }
    };

    // Style buttons
    auto styleButton = [] (juce::TextButton& btn, juce::Colour colour)
    {
        btn.setColour (juce::TextButton::buttonColourId, colour);
        btn.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    };

    styleButton (loadButton, juce::Colour (0xFF2A2A2A));
    styleButton (loadFolderButton, juce::Colour (0xFF2A2A2A));
    styleButton (abButton, juce::Colour (0xFF333355));
    styleButton (loudnessMatchButton, juce::Colour (0xFF335533));
    styleButton (playRefButton, juce::Colour (0xFF553333));

    startTimerHz (10);
}

void ReferencePanel::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xFF141414));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);

    g.setColour (juce::Colour (0xFF888888));
    g.setFont (12.0f);
    g.drawText ("REFERENCES", getLocalBounds().removeFromTop (22),
                juce::Justification::centred);
}

void ReferencePanel::resized()
{
    auto bounds = getLocalBounds().reduced (6);
    bounds.removeFromTop (24); // title

    auto buttonRow = bounds.removeFromTop (28);
    loadButton.setBounds (buttonRow.removeFromLeft (buttonRow.getWidth() / 2).reduced (2));
    loadFolderButton.setBounds (buttonRow.reduced (2));

    bounds.removeFromTop (4);

    auto controlRow = bounds.removeFromTop (28);
    int thirdW = controlRow.getWidth() / 3;
    abButton.setBounds (controlRow.removeFromLeft (thirdW).reduced (2));
    loudnessMatchButton.setBounds (controlRow.removeFromLeft (thirdW).reduced (2));
    playRefButton.setBounds (controlRow.reduced (2));

    bounds.removeFromTop (4);

    genreSelector.setBounds (bounds.removeFromTop (26).reduced (2));

    bounds.removeFromTop (4);
    referenceList.setBounds (bounds);
}

void ReferencePanel::buttonClicked (juce::Button* button)
{
    if (button == &loadButton)
        loadReferenceFile();
    else if (button == &loadFolderButton)
        loadReferenceFolder();
    else if (button == &abButton)
        processor.setABActive (! processor.isABActive());
    else if (button == &loudnessMatchButton)
        processor.setLoudnessMatchEnabled (! processor.isLoudnessMatchEnabled());
    else if (button == &playRefButton)
        processor.setPlayingReference (! processor.isPlayingReference());
}

void ReferencePanel::timerCallback()
{
    // Update button states
    abButton.setToggleState (processor.isABActive(), juce::dontSendNotification);
    abButton.setButtonText (processor.isABActive() ? "A/B [ON]" : "A/B");
    abButton.setColour (juce::TextButton::buttonColourId,
                        processor.isABActive() ? juce::Colour (0xFF4444AA) : juce::Colour (0xFF333355));

    loudnessMatchButton.setToggleState (processor.isLoudnessMatchEnabled(), juce::dontSendNotification);
    loudnessMatchButton.setButtonText (processor.isLoudnessMatchEnabled() ? "LUFS Match [ON]" : "LUFS Match");
    loudnessMatchButton.setColour (juce::TextButton::buttonColourId,
                                    processor.isLoudnessMatchEnabled() ? juce::Colour (0xFF44AA44) : juce::Colour (0xFF335533));

    playRefButton.setToggleState (processor.isPlayingReference(), juce::dontSendNotification);
    playRefButton.setButtonText (processor.isPlayingReference() ? "Stop" : "Play Ref");
    playRefButton.setColour (juce::TextButton::buttonColourId,
                              processor.isPlayingReference() ? juce::Colour (0xFFAA4444) : juce::Colour (0xFF553333));

    referenceList.updateContent();
}

void ReferencePanel::loadReferenceFile()
{
    auto chooser = std::make_shared<juce::FileChooser> (
        "Load Reference Track",
        juce::File::getSpecialLocation (juce::File::userMusicDirectory),
        "*.wav;*.aiff;*.aif;*.mp3;*.flac");

    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser] (const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
                processor.loadReferenceTrack (file);
        });
}

void ReferencePanel::loadReferenceFolder()
{
    auto chooser = std::make_shared<juce::FileChooser> (
        "Select Folder of Reference Tracks",
        juce::File::getSpecialLocation (juce::File::userMusicDirectory));

    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [this, chooser] (const juce::FileChooser& fc)
        {
            auto folder = fc.getResult();
            if (folder.isDirectory())
            {
                auto curve = processor.getCurveManager().analyzeFolder (
                    folder, processor.getSampleRate());
                processor.getCurveManager().addCurve (curve);
                processor.getCurveManager().setActiveCurveIndex (
                    (int) processor.getCurveManager().getCurves().size() - 1);
            }
        });
}

// ListBoxModel implementation
int ReferencePanel::ReferenceListModel::getNumRows()
{
    return (int) processor.getReferenceTracks().size();
}

void ReferencePanel::ReferenceListModel::paintListBoxItem (int row, juce::Graphics& g,
                                                            int width, int height, bool selected)
{
    auto& tracks = processor.getReferenceTracks();
    if (row < 0 || row >= (int) tracks.size())
        return;

    if (selected || row == processor.getActiveReferenceIndex())
        g.fillAll (juce::Colour (0xFF2A2A3A));
    else
        g.fillAll (juce::Colour (0xFF1A1A1A));

    g.setColour (row == processor.getActiveReferenceIndex()
                     ? juce::Colour (0xFF00CCFF)
                     : juce::Colour (0xFFCCCCCC));
    g.setFont (12.0f);
    g.drawText (tracks[(size_t) row].name, 8, 0, width - 16, height,
                juce::Justification::centredLeft);
}

void ReferencePanel::ReferenceListModel::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    processor.setActiveReference (row);
}
