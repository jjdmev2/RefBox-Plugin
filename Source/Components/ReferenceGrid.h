#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "RefBoxLookAndFeel.h"

class RefBoxProcessor;

class ReferenceGrid : public juce::Component,
                      public juce::DragAndDropContainer,
                      public juce::Timer
{
public:
    static constexpr int numSlots = 16;
    static constexpr int numCols = 8;
    static constexpr int numRows = 2;

    ReferenceGrid (RefBoxProcessor& processor);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    RefBoxProcessor& processor;

    struct Slot : public juce::Component,
                  public juce::DragAndDropTarget
    {
        Slot (ReferenceGrid& owner, int index);
        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent& e) override;
        void mouseUp (const juce::MouseEvent& e) override;
        void mouseDrag (const juce::MouseEvent& e) override;
        void mouseEnter (const juce::MouseEvent& e) override;
        void mouseExit (const juce::MouseEvent& e) override;

        bool isInterestedInDragSource (const SourceDetails&) override { return true; }
        void itemDroppedOnto (const SourceDetails& details);
        void itemDragEnter (const SourceDetails&) override { dragOver = true; repaint(); }
        void itemDragExit (const SourceDetails&) override { dragOver = false; repaint(); }
        void itemDropped (const SourceDetails& details) override;

        ReferenceGrid& owner;
        int slotIndex;
        bool dragOver = false;
        bool hovered = false;
        bool pressed = false;
    };

    std::array<std::unique_ptr<Slot>, numSlots> slots;

    // Mapping: slot index -> reference track index (-1 = empty)
    std::array<int, numSlots> slotMapping;

    void loadTrackIntoSlot (int slotIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReferenceGrid)
};
