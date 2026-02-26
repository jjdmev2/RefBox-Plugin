# RefBox

Professional audio mastering reference plugin built with JUCE (C++20).

Builds as **AU**, **VST3**, **AAX**, and **Standalone**.

## Features

- **128-band Spectrum Analyzer** — 4096-point FFT, log-spaced bands (20Hz–20kHz), butter-smooth 120fps rendering with display-level interpolation
- **Multiple Spectrum Modes** — Fast, Average, Smooth, Peak (like SPAN/Pro-Q ballistics)
- **5 Color Schemes** — Classic, Pioneer CDJ RGB, Amber, Purple, Fire
- **17 Genre Reference Curves** — EDM, Hip-Hop, Pop, Rock, Jazz, Classical, Lo-Fi, Techno, DnB, Trap, R&B, Metal, Acoustic, Reggaeton, Ambient, Orchestral, Podcast — with EQ delta matching
- **LUFS Metering** — ITU-R BS.1770-4 compliant K-weighted loudness (momentary, short-term, integrated) + true peak
- **CDJ-Style Waveform Microscope** — Zoomed transient view with Pioneer RGB frequency coloring (Red=lows, Green=mids, Blue=highs)
- **16-Slot Reference Grid** — Drag-and-drop reference track management with hover/press feedback
- **System Simulator** — Room response modeling for mix translation
- **A/B Comparison** — Instant switching with automatic loudness matching
- **Sub-Bass Accuracy** — Stretched bass region display with markers at 20, 30, 40, 50, 80, 100Hz
- **Hover Frequency Tooltip** — Crosshair cursor showing exact frequency + dB at mouse position
- **Peak Holders** — Per-band peak hold with configurable decay

## Building

### Requirements

- CMake 3.22+
- C++20 compiler (Clang/Apple Clang recommended)
- [JUCE](https://github.com/juce-framework/JUCE) — place in `../JUCE` relative to this project
- (Optional) [AAX SDK](https://developer.avid.com) — place in `../aax-sdk-2-9-0` for Pro Tools support

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Plugins auto-install to system plugin directories:
- **AU** → `~/Library/Audio/Plug-Ins/Components/`
- **VST3** → `~/Library/Audio/Plug-Ins/VST3/`
- **AAX** → `/Library/Application Support/Avid/Audio/Plug-Ins/`

## Project Structure

```
Source/
├── PluginProcessor.cpp/h     — Audio processing, reference track management
├── PluginEditor.cpp/h        — Main editor layout
├── DSP/
│   ├── SpectralAnalyzer       — FFT analysis, band magnitudes
│   ├── LUFSMeter              — BS.1770-4 loudness metering
│   ├── ReferenceCurve          — Genre preset curves
│   ├── EQMatcher               — Spectral delta / EQ suggestions
│   └── SystemSimulator         — Room response modeling
└── Components/
    ├── SpectrumDisplay         — Main spectrum visualizer
    ├── WaveformDisplay         — Overview waveform
    ├── WaveformMicroscope      — CDJ-style zoomed waveform
    ├── ReferenceGrid           — 16-slot track grid
    ├── MeterPanel              — LUFS + true peak meters
    ├── SystemSimPanel          — Room sim controls
    ├── TopBar                  — Mode/color/curve selectors
    ├── ABButton                — A/B toggle
    └── RefBoxLookAndFeel       — Custom dark theme
```

## License

All rights reserved.
