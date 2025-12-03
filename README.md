# NovaTune - Real-Time Vocal Pitch Correction & Harmonizer

<p align="center">
  <strong>Auto-Tune Style Pitch Correction + Intelligent Harmonization</strong>
</p>

## Overview

NovaTune is a professional-grade audio plugin that provides:
- **Real-time pitch correction** (from subtle tuning to "hard-tune" robotic effects)
- **Intelligent harmonization** (up to 3 harmony voices)
- **Ultra-low latency** (< 10ms for live performance)
- **Low CPU usage** (multiple instances per project)

## Supported Formats

| Format | Platform | Status |
|--------|----------|--------|
| VST3   | Windows/macOS | ✅ MVP |
| AU     | macOS | ✅ MVP |
| AAX    | Windows/macOS | 🔜 Phase 2 |

## Building from Source

### Prerequisites

1. **CMake** (3.22 or higher)
2. **JUCE Framework** (7.0 or higher)
3. **C++ Compiler** with C++17 support:
   - Windows: Visual Studio 2019+ or MinGW
   - macOS: Xcode 12+ or Clang
   - Linux: GCC 9+ or Clang 10+

### Build Steps

```bash
# Clone the repository
git clone https://github.com/yourusername/NovaTune.git
cd NovaTune

# Initialize JUCE submodule (if using submodule approach)
git submodule update --init --recursive

# Create build directory
mkdir build && cd build

# Configure (Release build for best performance)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# The plugin will be in:
# - VST3: build/NovaTune_artefacts/Release/VST3/
# - AU: build/NovaTune_artefacts/Release/AU/
```

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        NovaTune Plugin                          │
├─────────────────────────────────────────────────────────────────┤
│  PluginProcessor (Host Communication & State Management)        │
│  ├── Parameter Management (APVTS)                               │
│  ├── Preset System                                              │
│  └── TunerEngine (Main DSP Orchestrator)                        │
│       ├── PitchDetector (YIN Algorithm)                         │
│       ├── PitchMapper (Key/Scale Mapping)                       │
│       ├── LeadCorrection (WSOLA Pitch Shift)                    │
│       └── HarmonyVoice[3] (Parallel Pitch Shifters)             │
│            └── FormantProcessor (Spectral Envelope)             │
├─────────────────────────────────────────────────────────────────┤
│  PluginEditor (User Interface)                                  │
│  ├── Key/Scale Selection                                        │
│  ├── Retune Speed Knob                                          │
│  ├── Harmony Voice Panels                                       │
│  └── Pitch Visualization                                        │
└─────────────────────────────────────────────────────────────────┘
```

## DSP Signal Flow

```
Input Audio
    │
    ▼
┌─────────────────┐
│ Pitch Detection │ ──► Detected F0 (Fundamental Frequency)
│   (YIN Algo)    │
└─────────────────┘
    │
    ▼
┌─────────────────┐
│  Pitch Mapper   │ ──► Target Note (quantized to key/scale)
│  (Key/Scale)    │
└─────────────────┘
    │
    ├────────────────────┬────────────────────┐
    ▼                    ▼                    ▼
┌──────────┐      ┌────────────┐      ┌────────────┐
│   Lead   │      │ Harmony A  │      │ Harmony B  │ ...
│Correction│      │   Voice    │      │   Voice    │
│ (WSOLA)  │      │  (WSOLA)   │      │  (WSOLA)   │
└──────────┘      └────────────┘      └────────────┘
    │                    │                    │
    └────────────────────┴────────────────────┘
                         │
                         ▼
                   ┌──────────┐
                   │   Mix    │
                   │  Output  │
                   └──────────┘
                         │
                         ▼
                   Output Audio
```

## Key DSP Algorithms

### YIN Pitch Detection
The YIN algorithm estimates fundamental frequency (F0) by computing a 
cumulative mean normalized difference function. It's more accurate than 
simple autocorrelation and handles:
- Octave errors (common in pitch detection)
- Noise in the signal
- Fast pitch changes

### WSOLA Pitch Shifting
Waveform Similarity Overlap-Add allows pitch shifting without time stretching:
1. Analysis: Segment audio into overlapping frames
2. Synthesis: Resample frames at new rate
3. Overlap-Add: Cross-fade overlapping regions

### Formant Preservation
Formants (vocal tract resonances) are preserved by:
1. Extracting spectral envelope via LPC or cepstral analysis
2. Applying pitch shift to carrier signal
3. Re-imposing original spectral envelope

## Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| Key | C-B | Musical key for scale mapping |
| Scale | Major/Minor/Chromatic/etc. | Scale type |
| Retune Speed | 0-100 | How fast pitch corrects (0=slow, 100=instant) |
| Humanize | 0-100 | Preserves natural variation |
| Mix | 0-100% | Dry/wet balance |

## License

[Your License Here]

## Acknowledgments

- JUCE Framework by ROLI/Focusrite
- YIN Algorithm (de Cheveigné & Kawahara, 2002)
- WSOLA (Verhelst & Roelands, 1993)
