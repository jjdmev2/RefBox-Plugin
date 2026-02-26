#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"

class ReferencePanel : public juce::Component,
                       public juce::Button::Listener,
                       public juce::Timer
{
public:
    ReferencePanel (RefBoxProcessor& processor);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked (juce::Button* button) override;
    void timerCallback() override;

private:
    RefBoxProcessor& processor;

    juce::TextButton loadButton { "Load Reference" };
    juce::TextButton loadFolderButton { "Load Folder" };
    juce::TextButton abButton { "A/B" };
    juce::TextButton loudnessMatchButton { "LUFS Match" };
    juce::TextButton playRefButton { "Play Ref" };

    // Reference track list
    juce::ListBox referenceList;

    class ReferenceListModel : public juce::ListBoxModel
    {
    public:
        ReferenceListModel (RefBoxProcessor& p) : processor (p) {}
        int getNumRows() override;
        void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override;
        void listBoxItemClicked (int row, const juce::MouseEvent&) override;

    private:
        RefBoxProcessor& processor;
    };

    ReferenceListModel listModel;

    // Genre preset selector
    juce::ComboBox genreSelector;

    void loadReferenceFile();
    void loadReferenceFolder();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReferencePanel)
};
