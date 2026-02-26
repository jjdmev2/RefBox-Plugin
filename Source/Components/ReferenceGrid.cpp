#include "ReferenceGrid.h"
#include "../PluginProcessor.h"

// --- Slot ---

ReferenceGrid::Slot::Slot (ReferenceGrid& o, int idx)
    : owner (o), slotIndex (idx)
{
}

void ReferenceGrid::Slot::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    int trackIdx = owner.slotMapping[(size_t) slotIndex];
    bool isActive = (trackIdx >= 0 && trackIdx == owner.processor.getActiveReferenceIndex());
    bool hasTrack = (trackIdx >= 0);

    // Push-down effect: shift content down 1px when pressed
    auto drawBounds = bounds;
    if (pressed)
        drawBounds.translate (0.0f, 1.0f);

    // Background
    juce::Colour bgCol = dragOver  ? RefBoxColours::slotActive.brighter (0.1f)
                       : pressed   ? RefBoxColours::accent.withAlpha (0.22f)
                       : isActive  ? RefBoxColours::accent.withAlpha (0.15f)
                       : (hovered && hasTrack)  ? RefBoxColours::slotBg.brighter (0.06f)
                       : (hovered && !hasTrack) ? RefBoxColours::bg.brighter (0.06f)
                       : hasTrack  ? RefBoxColours::slotBg
                       : RefBoxColours::bg.brighter (0.02f);

    g.setColour (bgCol);
    g.fillRoundedRectangle (drawBounds, 4.0f);

    // Border
    juce::Colour borderCol = pressed  ? RefBoxColours::accent.withAlpha (0.6f)
                           : isActive ? RefBoxColours::accent.withAlpha (0.5f)
                           : (hovered && !hasTrack) ? RefBoxColours::textDim.withAlpha (0.25f)
                           : RefBoxColours::panelBorder;
    g.setColour (borderCol);
    g.drawRoundedRectangle (drawBounds, 4.0f, 1.0f);

    // Slot number
    g.setColour (RefBoxColours::textDim);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText (juce::String (slotIndex + 1), drawBounds.reduced (3.0f, 1.0f),
                juce::Justification::topLeft);

    if (hasTrack)
    {
        auto& tracks = owner.processor.getReferenceTracks();
        if (trackIdx < (int) tracks.size())
        {
            // Track name
            g.setColour (isActive ? RefBoxColours::accent : RefBoxColours::textPrimary);
            g.setFont (juce::FontOptions (std::min (11.0f, drawBounds.getHeight() * 0.35f)));
            g.drawText (tracks[(size_t) trackIdx].name,
                        drawBounds.reduced (4.0f, 2.0f),
                        juce::Justification::centred, true);
        }
    }
    else
    {
        g.setColour (hovered ? RefBoxColours::textDim.withAlpha (0.5f) : RefBoxColours::textDim.withAlpha (0.3f));
        g.setFont (juce::FontOptions (16.0f));
        g.drawText ("+", drawBounds, juce::Justification::centred);
    }
}

void ReferenceGrid::Slot::mouseDown (const juce::MouseEvent&)
{
    pressed = true;
    repaint();

    int trackIdx = owner.slotMapping[(size_t) slotIndex];

    if (trackIdx >= 0)
    {
        // Select this track
        owner.processor.setActiveReference (trackIdx);
    }
    else
    {
        // Empty slot - load a track
        owner.loadTrackIntoSlot (slotIndex);
    }
}

void ReferenceGrid::Slot::mouseUp (const juce::MouseEvent&)
{
    pressed = false;
    repaint();
}

void ReferenceGrid::Slot::mouseEnter (const juce::MouseEvent&)
{
    hovered = true;
    repaint();
}

void ReferenceGrid::Slot::mouseExit (const juce::MouseEvent&)
{
    hovered = false;
    pressed = false;
    repaint();
}

void ReferenceGrid::Slot::mouseDrag (const juce::MouseEvent& e)
{
    int trackIdx = owner.slotMapping[(size_t) slotIndex];
    if (trackIdx < 0) return;

    if (e.getDistanceFromDragStart() > 5)
    {
        auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this);
        if (container != nullptr)
        {
            juce::var dragData (slotIndex);
            container->startDragging (dragData, this);
        }
    }
}

void ReferenceGrid::Slot::itemDropped (const SourceDetails& details)
{
    dragOver = false;
    itemDroppedOnto (details);
    repaint();
}

void ReferenceGrid::Slot::itemDroppedOnto (const SourceDetails& details)
{
    int sourceSlot = (int) details.description;
    if (sourceSlot == slotIndex) return;

    // Swap the two slots
    std::swap (owner.slotMapping[(size_t) sourceSlot],
               owner.slotMapping[(size_t) slotIndex]);

    owner.repaint();
}

// --- ReferenceGrid ---

ReferenceGrid::ReferenceGrid (RefBoxProcessor& p) : processor (p)
{
    slotMapping.fill (-1);

    for (int i = 0; i < numSlots; ++i)
    {
        slots[(size_t) i] = std::make_unique<Slot> (*this, i);
        addAndMakeVisible (slots[(size_t) i].get());
    }

    startTimerHz (8);
}

void ReferenceGrid::timerCallback()
{
    // Sync slot mapping with loaded tracks
    auto& tracks = processor.getReferenceTracks();
    int numTracks = (int) tracks.size();

    // Auto-assign new tracks to empty slots
    for (int t = 0; t < numTracks && t < numSlots; ++t)
    {
        bool found = false;
        for (int s = 0; s < numSlots; ++s)
        {
            if (slotMapping[(size_t) s] == t) { found = true; break; }
        }
        if (! found)
        {
            for (int s = 0; s < numSlots; ++s)
            {
                if (slotMapping[(size_t) s] < 0)
                {
                    slotMapping[(size_t) s] = t;
                    break;
                }
            }
        }
    }

    // Clear slots that point to removed tracks
    for (int s = 0; s < numSlots; ++s)
    {
        if (slotMapping[(size_t) s] >= numTracks)
            slotMapping[(size_t) s] = -1;
    }

    repaint();
}

void ReferenceGrid::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (RefBoxColours::panelBg);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (RefBoxColours::panelBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    // Title
    g.setColour (RefBoxColours::textDim);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("REFERENCES", bounds.getX() + 8.0f, bounds.getY() + 2.0f, 80.0f, 14.0f,
                juce::Justification::centredLeft);
}

void ReferenceGrid::resized()
{
    auto bounds = getLocalBounds().reduced (4);
    bounds.removeFromTop (14); // space for title
    int cellW = bounds.getWidth() / numCols;
    int cellH = bounds.getHeight() / numRows;

    for (int i = 0; i < numSlots; ++i)
    {
        int col = i % numCols;
        int row = i / numCols;
        slots[(size_t) i]->setBounds (bounds.getX() + col * cellW,
                                       bounds.getY() + row * cellH,
                                       cellW, cellH);
    }
}

void ReferenceGrid::loadTrackIntoSlot (int slotIndex)
{
    auto chooser = std::make_shared<juce::FileChooser> (
        "Load Reference Track",
        juce::File::getSpecialLocation (juce::File::userMusicDirectory),
        "*.wav;*.aiff;*.aif;*.mp3;*.flac");

    chooser->launchAsync (
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, slotIndex, chooser] (const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                processor.loadReferenceTrack (file);
                int newIdx = (int) processor.getReferenceTracks().size() - 1;
                slotMapping[(size_t) slotIndex] = newIdx;
                processor.setActiveReference (newIdx);
            }
        });
}
