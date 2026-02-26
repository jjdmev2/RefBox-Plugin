#include "SystemSimPanel.h"

// --- SimButton ---

SystemSimPanel::SimButton::SimButton (SystemSimPanel& o, int idx, const juce::String& name)
    : owner (o), systemIndex (idx), systemName (name)
{
}

void SystemSimPanel::SimButton::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);

    juce::Colour bgCol = active ? RefBoxColours::accent.withAlpha (0.2f) : RefBoxColours::slotBg;
    g.setColour (bgCol);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (active ? RefBoxColours::accent.withAlpha (0.5f) : RefBoxColours::panelBorder);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    // Icon area (top 60%)
    auto iconArea = bounds.reduced (4.0f).removeFromTop (bounds.getHeight() * 0.55f);
    drawIcon (g, iconArea);

    // Label (bottom)
    g.setColour (active ? RefBoxColours::textPrimary : RefBoxColours::textSecondary);
    g.setFont (juce::FontOptions (std::min (8.5f, bounds.getHeight() * 0.16f)));

    // Short name
    juce::String shortName = systemName;
    if (shortName.length() > 10)
    {
        // Abbreviate
        if (shortName.contains ("Bluetooth")) shortName = "BT Speaker";
        else if (shortName.contains ("Consumer")) shortName = "Headphones";
        else if (shortName.contains ("Yamaha")) shortName = "NS-10";
        else if (shortName.contains ("Auratone")) shortName = "Mixcube";
        else if (shortName.contains ("Smart")) shortName = "HomePod";
    }

    g.drawText (shortName, bounds.reduced (2.0f).removeFromBottom (bounds.getHeight() * 0.32f),
                juce::Justification::centred, true);
}

void SystemSimPanel::SimButton::drawIcon (juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour (active ? RefBoxColours::accent : RefBoxColours::textDim);
    float cx = area.getCentreX(), cy = area.getCentreY();
    float sz = std::min (area.getWidth(), area.getHeight()) * 0.7f;
    float hs = sz * 0.5f;

    auto sysType = (SystemSimulator::SystemType) systemIndex;

    switch (sysType)
    {
        case SystemSimulator::SystemType::Laptop:
        {
            // Laptop: screen + base
            g.drawRect (cx - hs, cy - hs * 0.6f, sz, sz * 0.6f, 1.5f);
            g.drawLine (cx - hs * 1.2f, cy + hs * 0.4f, cx + hs * 1.2f, cy + hs * 0.4f, 1.5f);
            break;
        }
        case SystemSimulator::SystemType::Phone:
        {
            // Phone: tall rounded rect
            g.drawRoundedRectangle (cx - hs * 0.35f, cy - hs * 0.7f, sz * 0.35f, sz * 0.7f, 2.0f, 1.5f);
            g.fillEllipse (cx - 1.5f, cy + hs * 0.45f, 3.0f, 3.0f);
            break;
        }
        case SystemSimulator::SystemType::Earbuds:
        {
            // Earbuds: two circles connected
            g.drawEllipse (cx - hs * 0.5f, cy - hs * 0.3f, sz * 0.3f, sz * 0.3f, 1.5f);
            g.drawEllipse (cx + hs * 0.2f, cy - hs * 0.3f, sz * 0.3f, sz * 0.3f, 1.5f);
            g.drawLine (cx - hs * 0.2f, cy - hs * 0.25f, cx + hs * 0.35f, cy - hs * 0.25f, 1.0f);
            break;
        }
        case SystemSimulator::SystemType::CarStereo:
        {
            // Car: simple car shape
            g.drawRoundedRectangle (cx - hs * 0.7f, cy - hs * 0.1f, sz * 0.7f, sz * 0.35f, 2.0f, 1.5f);
            g.drawRoundedRectangle (cx - hs * 0.45f, cy - hs * 0.45f, sz * 0.45f, sz * 0.4f, 3.0f, 1.5f);
            g.fillEllipse (cx - hs * 0.5f, cy + hs * 0.2f, sz * 0.15f, sz * 0.15f);
            g.fillEllipse (cx + hs * 0.25f, cy + hs * 0.2f, sz * 0.15f, sz * 0.15f);
            break;
        }
        case SystemSimulator::SystemType::TVSpeakers:
        {
            // TV: wide rect on stand
            g.drawRect (cx - hs * 0.8f, cy - hs * 0.45f, sz * 0.8f, sz * 0.5f, 1.5f);
            g.drawLine (cx, cy + hs * 0.1f, cx, cy + hs * 0.4f, 1.5f);
            g.drawLine (cx - hs * 0.3f, cy + hs * 0.4f, cx + hs * 0.3f, cy + hs * 0.4f, 1.5f);
            break;
        }
        case SystemSimulator::SystemType::ClubPA:
        {
            // Speaker stack
            g.drawRect (cx - hs * 0.4f, cy - hs * 0.6f, sz * 0.4f, sz * 0.6f, 1.5f);
            g.drawEllipse (cx - hs * 0.15f, cy - hs * 0.35f, sz * 0.15f, sz * 0.15f, 1.5f);
            g.drawEllipse (cx - hs * 0.2f, cy - hs * 0.05f, sz * 0.2f, sz * 0.2f, 1.5f);
            break;
        }
        case SystemSimulator::SystemType::BluetoothSmall:
        {
            // Cylinder-ish speaker
            g.drawRoundedRectangle (cx - hs * 0.5f, cy - hs * 0.25f, sz * 0.5f, sz * 0.25f, 4.0f, 1.5f);
            // BT symbol
            g.drawLine (cx, cy - hs * 0.5f, cx + hs * 0.15f, cy - hs * 0.35f, 1.0f);
            g.drawLine (cx + hs * 0.15f, cy - hs * 0.35f, cx - hs * 0.1f, cy - hs * 0.2f, 1.0f);
            break;
        }
        case SystemSimulator::SystemType::StudioNS10:
        {
            // Studio monitor (rect with cone circle)
            g.drawRect (cx - hs * 0.35f, cy - hs * 0.55f, sz * 0.35f, sz * 0.55f, 1.5f);
            g.drawEllipse (cx - hs * 0.18f, cy - hs * 0.15f, sz * 0.18f, sz * 0.18f, 1.5f);
            g.fillEllipse (cx - hs * 0.08f, cy - hs * 0.45f, sz * 0.08f, sz * 0.08f);
            break;
        }
        case SystemSimulator::SystemType::AuratoneC5:
        {
            // Small cube speaker
            float cubeS = sz * 0.35f;
            g.drawRect (cx - cubeS * 0.5f, cy - cubeS * 0.5f, cubeS, cubeS, 1.5f);
            g.drawEllipse (cx - cubeS * 0.25f, cy - cubeS * 0.25f, cubeS * 0.5f, cubeS * 0.5f, 1.5f);
            break;
        }
        case SystemSimulator::SystemType::HomePod:
        {
            // Rounded cylinder
            g.drawRoundedRectangle (cx - hs * 0.25f, cy - hs * 0.4f, sz * 0.25f, sz * 0.4f, 6.0f, 1.5f);
            break;
        }
        case SystemSimulator::SystemType::Headphones:
        {
            // Headband + cups
            juce::Path arc;
            arc.addArc (cx - hs * 0.35f, cy - hs * 0.55f, sz * 0.35f, sz * 0.3f,
                        juce::MathConstants<float>::pi, 0.0f);
            g.strokePath (arc, juce::PathStrokeType (1.5f));
            g.drawRect (cx - hs * 0.4f, cy - hs * 0.1f, sz * 0.15f, sz * 0.25f, 1.5f);
            g.drawRect (cx + hs * 0.15f, cy - hs * 0.1f, sz * 0.15f, sz * 0.25f, 1.5f);
            break;
        }
        default:
            break;
    }
}

void SystemSimPanel::SimButton::mouseDown (const juce::MouseEvent&)
{
    // If clicking the already-active button, toggle off
    if (owner.simulator.getEnabled()
        && (int) owner.simulator.getSystemType() == systemIndex)
    {
        owner.simulator.setEnabled (false);
    }
    else
    {
        owner.simulator.setSystemType ((SystemSimulator::SystemType) systemIndex);
        owner.simulator.setEnabled (true);
    }
}

// --- SystemSimPanel ---

SystemSimPanel::SystemSimPanel (SystemSimulator& sim) : simulator (sim)
{
    for (int i = 1; i < SystemSimulator::getNumSystems(); ++i)
    {
        auto name = SystemSimulator::getSystemName ((SystemSimulator::SystemType) i);
        auto btn = std::make_unique<SimButton> (*this, i, name);
        addAndMakeVisible (btn.get());
        simButtons.push_back (std::move (btn));
    }

    startTimerHz (10);
}

void SystemSimPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (RefBoxColours::panelBg);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (RefBoxColours::panelBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    g.setColour (RefBoxColours::textSecondary);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("SYSTEM SIM", bounds.removeFromTop (18), juce::Justification::centred);
}

void SystemSimPanel::resized()
{
    auto bounds = getLocalBounds().reduced (4);
    bounds.removeFromTop (20);

    int cols = 3;
    int numItems = (int) simButtons.size();
    int rows = (numItems + cols - 1) / cols;

    int cellW = bounds.getWidth() / cols;
    int cellH = std::min (bounds.getHeight() / std::max (1, rows), 60);

    for (int i = 0; i < numItems; ++i)
    {
        int col = i % cols;
        int row = i / cols;
        simButtons[(size_t) i]->setBounds (bounds.getX() + col * cellW,
                                            bounds.getY() + row * cellH,
                                            cellW, cellH);
    }
}

void SystemSimPanel::timerCallback()
{
    bool enabled = simulator.getEnabled();
    auto currentType = simulator.getSystemType();

    for (int i = 0; i < (int) simButtons.size(); ++i)
    {
        simButtons[(size_t) i]->active = enabled && ((int) currentType == i + 1);
        simButtons[(size_t) i]->repaint();
    }
}
