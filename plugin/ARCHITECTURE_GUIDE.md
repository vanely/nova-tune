# NovaTune Architecture & Learning Guide

**A comprehensive guide for understanding and navigating the NovaTune codebase**

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Directory Structure](#directory-structure)
3. [Architecture Layers](#architecture-layers)
4. [Component Relationships](#component-relationships)
5. [DSP Algorithms Explained](#dsp-algorithms-explained)
6. [Import Sources & Dependencies](#import-sources--dependencies)
7. [Code Navigation Guide](#code-navigation-guide)
8. [Learning Resources](#learning-resources)

---

## Project Overview

NovaTune is a real-time vocal pitch correction and harmonization plugin built with JUCE. It processes audio in real-time to:
- Detect the pitch of incoming vocals
- Correct pitch to match a musical key/scale
- Generate up to 3 harmony voices
- Preserve natural vocal characteristics (formants, vibrato)

**Key Technologies:**
- **JUCE Framework**: Audio plugin framework (C++)
- **YIN Algorithm**: Pitch detection
- **WSOLA**: Pitch shifting without time stretching
- **Formant Processing**: Preserve vocal character

---

## Directory Structure

```
plugin/
├── Source/                    # All project source code
│   ├── PluginProcessor.*     # Main plugin interface (host communication)
│   ├── PluginEditor.*        # User interface
│   ├── ParameterIDs.h        # Parameter definitions & enums
│   ├── DSPConfig.h           # DSP constants & configuration
│   ├── Utilities.h            # Pure utility functions
│   └── dsp/                   # Digital Signal Processing modules
│       ├── TunerEngine.*     # Main DSP orchestrator
│       ├── PitchDetector.*   # YIN pitch detection
│       ├── PitchMapper.*      # Key/scale mapping
│       ├── LeadCorrection.*   # Lead vocal pitch correction
│       ├── PitchShifter.*     # WSOLA pitch shifting
│       ├── HarmonyVoice.*     # Harmony voice generation
│       └── FormantProcessor.* # Formant preservation/shifting
├── JUCE/                      # JUCE framework (external dependency)
└── build/                     # Build output directory
```

---

## Architecture Layers

NovaTune follows a **layered architecture** with clear separation of concerns:

### Layer 1: Host Interface (PluginProcessor)
**Files:** `PluginProcessor.h`, `PluginProcessor.cpp`

**Purpose:** Communication layer between the DAW (host) and the plugin.

**Responsibilities:**
- Implements JUCE's `AudioProcessor` interface
- Manages plugin lifecycle (construction, preparation, processing, destruction)
- Handles parameter state via `AudioProcessorValueTreeState` (APVTS)
- Serializes/deserializes plugin state for presets
- Delegates audio processing to `TunerEngine`

**Key Concepts:**
- **Audio Thread**: `processBlock()` runs on a real-time audio thread (must be fast, no blocking)
- **Message Thread**: UI runs on a separate thread (can block for user interaction)
- **Thread Safety**: Parameters use atomic operations for safe cross-thread access

**Imports:**
- `juce_audio_processors/juce_audio_processors.h` - JUCE plugin framework
- `ParameterIDs.h` - Parameter ID strings
- `dsp/TunerEngine.h` - DSP engine

---

### Layer 2: User Interface (PluginEditor)
**Files:** `PluginEditor.h`, `PluginEditor.cpp`

**Purpose:** Graphical user interface for controlling the plugin.

**Responsibilities:**
- Renders the plugin UI (knobs, dropdowns, displays)
- Connects UI controls to parameters via `*Attachment` classes
- Displays real-time pitch information
- Custom look-and-feel styling

**Key Components:**
- `NovaTuneLookAndFeel`: Custom visual styling
- `PitchDisplayComponent`: Shows detected/target pitch
- `HarmonyVoicePanel`: Controls for each harmony voice
- `NovaTuneAudioProcessorEditor`: Main editor window

**Imports:**
- `juce_gui_extra/juce_gui_extra.h` - Extended GUI components
- `juce_audio_processors/juce_audio_processors.h` - Parameter attachments
- `PluginProcessor.h` - Access to processor and parameters

---

### Layer 3: DSP Orchestration (TunerEngine)
**Files:** `dsp/TunerEngine.h`, `dsp/TunerEngine.cpp`

**Purpose:** Coordinates all DSP components in the correct order.

**Signal Flow:**
```
Input Audio
    ↓
[PitchDetector] → Detected F0
    ↓
[PitchMapper] → Target Notes
    ↓
    ├─→ [LeadCorrection] → Corrected Lead
    ├─→ [HarmonyVoice A] → Harmony A
    ├─→ [HarmonyVoice B] → Harmony B
    └─→ [HarmonyVoice C] → Harmony C
    ↓
[Mixer] → Output Audio
```

**Responsibilities:**
- Orchestrates the processing pipeline
- Manages internal buffers (lead, harmony, dry)
- Updates all DSP components from parameters
- Calculates total latency

**Imports:**
- `juce_audio_processors/juce_audio_processors.h` - Parameter access
- `PitchDetector.h`, `PitchMapper.h`, `LeadCorrection.h`, `HarmonyVoice.h` - DSP components
- `DSPConfig.h` - Configuration constants
- `ParameterIDs.h` - Parameter IDs

---

### Layer 4: DSP Components

#### 4.1 Pitch Detection (PitchDetector)
**Files:** `dsp/PitchDetector.h`, `dsp/PitchDetector.cpp`

**Purpose:** Detects the fundamental frequency (F0) of incoming audio.

**Algorithm:** YIN (de Cheveigné & Kawahara, 2002)

**How It Works:**
1. **Difference Function**: For each possible period τ, compute how different the signal is from a shifted copy
2. **Cumulative Mean Normalization**: Normalize to reduce octave errors
3. **Absolute Threshold**: Find the smallest period below threshold
4. **Parabolic Interpolation**: Refine to sub-sample accuracy

**Key Methods:**
- `process()`: Analyze audio buffer
- `getFrequencyHz()`: Get detected frequency
- `getMidiNote()`: Get detected MIDI note
- `isVoiced()`: Is signal currently singing?

**Imports:**
- `juce_audio_basics/juce_audio_basics.h` - Audio buffer types
- `DSPConfig.h` - Configuration (frame size, thresholds)
- `Utilities.h` - Frequency conversion functions
- `ParameterIDs.h` - Input type enum

---

#### 4.2 Pitch Mapping (PitchMapper)
**Files:** `dsp/PitchMapper.h`, `dsp/PitchMapper.cpp`

**Purpose:** Maps detected pitch to target notes based on key/scale.

**Key Concepts:**
- **Scale Quantization**: Snaps detected notes to nearest scale note
- **Diatonic Intervals**: Intervals that follow the scale (musical)
- **Semitone Intervals**: Fixed semitone offsets (chromatic)

**Example:**
- Key: C Major, Detected: E♭ (slightly flat)
- Maps to: E (nearest scale note in C Major)

**Key Methods:**
- `map()`: Map detected pitch to target notes
- `quantizeToScale()`: Snap to nearest scale note
- `calculateHarmonyTarget()`: Calculate harmony voice target

**Imports:**
- `juce_audio_processors/juce_audio_processors.h` - Parameter access
- `ParameterIDs.h` - Key, scale, harmony enums
- `Utilities.h` - Musical interval calculations
- `PitchDetector.h` - Detection results

---

#### 4.3 Lead Correction (LeadCorrection)
**Files:** `dsp/LeadCorrection.h`, `dsp/LeadCorrection.cpp`

**Purpose:** Applies pitch correction to the lead vocal.

**Key Parameters:**
- **Retune Speed** (0-100): How fast to correct (0=slow/natural, 100=instant/robotic)
- **Humanize** (0-100): Preserves natural pitch variation
- **Vibrato** (0-100): Preserves natural vibrato
- **Mix** (0-100%): Dry/wet balance

**How It Works:**
1. Calculate target pitch ratio from detected → target note
2. Smooth the ratio based on retune speed
3. Apply humanization (adds variation)
4. Use `PitchShifter` to shift pitch
5. Mix with dry signal

**Imports:**
- `juce_audio_basics/juce_audio_basics.h` - Audio buffers
- `juce_audio_processors/juce_audio_processors.h` - Parameters
- `PitchDetector.h`, `PitchMapper.h` - Detection/mapping results
- `PitchShifter.h` - Pitch shifting engine
- `DSPConfig.h`, `Utilities.h` - Configuration & utilities

---

#### 4.4 Pitch Shifting (PitchShifter)
**Files:** `dsp/PitchShifter.h`, `dsp/PitchShifter.cpp`

**Purpose:** Shifts pitch without changing playback speed.

**Algorithm:** WSOLA (Waveform Similarity Overlap-Add)

**How WSOLA Works:**
1. **Analysis**: Cut input into overlapping "grains" (chunks)
2. **Time Scaling**: Adjust grain spacing based on pitch ratio
   - Pitch UP: Grains closer together (more overlap)
   - Pitch DOWN: Grains further apart (less overlap)
3. **Waveform Similarity**: Find best alignment position for each grain
4. **Overlap-Add**: Crossfade grains using window function (Hann window)
5. **Resampling**: Resample to restore original duration

**Key Methods:**
- `setPitchRatio()`: Set pitch shift ratio (1.0 = no change, 2.0 = octave up)
- `setPitchSemitones()`: Set shift in semitones
- `process()`: Process audio buffer

**Imports:**
- `juce_audio_basics/juce_audio_basics.h` - Audio buffers
- `juce_dsp/juce_dsp.h` - DSP utilities
- `DSPConfig.h` - Window size, overlap configuration
- `Utilities.h` - Pitch conversion functions

---

#### 4.5 Harmony Voice (HarmonyVoice)
**Files:** `dsp/HarmonyVoice.h`, `dsp/HarmonyVoice.cpp`

**Purpose:** Generates a single harmony voice from the lead.

**How It Works:**
1. Calculate harmony target note (from `PitchMapper`)
2. Use `PitchShifter` to shift lead audio to harmony pitch
3. Apply `FormantProcessor` for formant control
4. Apply level, pan, and humanization

**Key Parameters:**
- Enabled, Mode (Diatonic/Semitone), Interval, Level, Pan, Formant Shift
- Humanize Timing, Humanize Pitch

**Imports:**
- `juce_audio_basics/juce_audio_basics.h` - Audio buffers
- `juce_audio_processors/juce_audio_processors.h` - Parameters
- `PitchDetector.h`, `PitchMapper.h` - Detection/mapping
- `PitchShifter.h` - Pitch shifting
- `FormantProcessor.h` - Formant processing
- `DSPConfig.h`, `Utilities.h` - Configuration

---

#### 4.6 Formant Processing (FormantProcessor)
**Files:** `dsp/FormantProcessor.h`, `dsp/FormantProcessor.cpp`

**Purpose:** Preserves or shifts vocal formants (vocal character).

**What Are Formants?**
- Frequency regions boosted by the vocal tract (throat, mouth, nasal cavity)
- F1 (200-1000 Hz): Jaw openness
- F2 (700-2500 Hz): Tongue position
- Different vowels have different formant patterns

**Why It Matters:**
- Without formant preservation: Pitch up → "chipmunk" sound, Pitch down → "giant" sound
- With formant preservation: Voice sounds like the same person, just different pitch

**How It Works:**
- Uses a bank of bandpass filters to analyze spectral envelope
- Shifts filter frequencies to preserve or modify formants
- Recombines to maintain vocal character

**Imports:**
- `juce_audio_basics/juce_audio_basics.h` - Audio buffers
- `juce_dsp/juce_dsp.h` - IIR filters
- `DSPConfig.h` - Configuration

---

### Layer 5: Configuration & Utilities

#### 5.1 DSP Configuration (DSPConfig.h)
**Purpose:** Central location for all DSP constants.

**Contains:**
- Parameter ranges (retune speed, humanize, etc.)
- Pitch detection settings (frame size, thresholds)
- WSOLA configuration (window size, overlap)
- Voice type frequency ranges
- Musical constants (A4 = 440 Hz, etc.)

**Why Centralized?**
- Easy to tune algorithm parameters
- No "magic numbers" scattered through code
- Single source of truth

---

#### 5.2 Parameter IDs (ParameterIDs.h)
**Purpose:** String identifiers for all plugin parameters.

**Contains:**
- Parameter ID strings (e.g., `"retuneSpeed"`, `"key"`)
- Enumerations (Key, Scale, InputType, HarmonyMode, etc.)
- Helper functions (getKeyNames(), getScaleNames(), etc.)

**Why String IDs?**
- DAWs need consistent IDs for automation, presets, host display
- Prevents typos (compile-time checked)
- Enables autocomplete

**Namespaces:**
- `ParamIDs`: Parameter ID strings
- `NovaTuneEnums`: Enumerations and display name functions

---

#### 5.3 Utilities (Utilities.h)
**Purpose:** Pure utility functions (no state, no side effects).

**Contains:**
- Frequency ↔ MIDI note conversions
- Pitch ratio calculations
- Musical interval calculations (diatonic to semitones)
- Smoothing/filtering utilities
- Window functions

**All functions are `inline`** for performance (no function call overhead).

---

## Component Relationships

### Dependency Graph

```
PluginProcessor
    ├─→ TunerEngine
    │       ├─→ PitchDetector
    │       │       └─→ Utilities, DSPConfig, ParameterIDs
    │       ├─→ PitchMapper
    │       │       └─→ Utilities, ParameterIDs, PitchDetector
    │       ├─→ LeadCorrection
    │       │       ├─→ PitchShifter
    │       │       │       └─→ Utilities, DSPConfig
    │       │       └─→ PitchDetector, PitchMapper
    │       └─→ HarmonyVoice[3]
    │               ├─→ PitchShifter
    │               ├─→ FormantProcessor
    │               │       └─→ DSPConfig
    │               └─→ PitchDetector, PitchMapper
    └─→ PluginEditor
            └─→ PluginProcessor (for parameter access)
```

### Data Flow

1. **Host → PluginProcessor**: Audio buffer arrives via `processBlock()`
2. **PluginProcessor → TunerEngine**: Delegates processing
3. **TunerEngine → PitchDetector**: Analyzes pitch
4. **TunerEngine → PitchMapper**: Maps to target notes
5. **TunerEngine → LeadCorrection**: Corrects lead vocal
6. **TunerEngine → HarmonyVoice**: Generates harmonies
7. **TunerEngine → Mixer**: Combines all voices
8. **PluginProcessor → Host**: Returns processed buffer

---

## DSP Algorithms Explained

### 1. YIN Pitch Detection

**What It Does:** Finds the fundamental frequency (F0) of a periodic signal.

**Why It's Needed:** We need to know what note the singer is singing before we can correct it.

**How It Works:**

**Step 1: Difference Function**
```
d(τ) = Σ (x[j] - x[j+τ])²
```
For each possible period τ, measure how different the signal is from a shifted copy. When τ equals the pitch period, the signals align and d(τ) is small.

**Step 2: Cumulative Mean Normalization**
```
d'(τ) = d(τ) / [(1/τ) * Σ(d(j), j=1 to τ)]
```
Normalizes to reduce octave errors (prevents detecting 2x or 0.5x the actual frequency).

**Step 3: Absolute Threshold**
Find the smallest τ where d'(τ) < threshold. This is the pitch period estimate.

**Step 4: Parabolic Interpolation**
Fit a parabola to refine the estimate to sub-sample accuracy.

**Learning Resources:**
- **Paper**: "YIN, a fundamental frequency estimator for speech and music" (de Cheveigné & Kawahara, 2002)
- **Wikipedia**: https://en.wikipedia.org/wiki/YIN_(algorithm)
- **Implementation Guide**: See `PitchDetector.cpp` for detailed comments

---

### 2. WSOLA Pitch Shifting

**What It Does:** Changes pitch without changing playback speed.

**Why It's Needed:** We need to shift the singer's pitch to the target note while keeping the same timing.

**How It Works:**

**Concept:** Time-scale the audio, then resample to restore original duration.

**Step 1: Grain Extraction**
Cut the input into overlapping chunks called "grains" (typically 20-50ms).

**Step 2: Time Scaling**
- To pitch UP: Place grains closer together (more overlap)
- To pitch DOWN: Place grains further apart (less overlap)

**Step 3: Waveform Similarity**
When placing each grain, search for the best alignment position to prevent phase discontinuities (which cause buzzing/artifacts).

**Step 4: Overlap-Add**
Crossfade grains using a window function (Hann window) to create smooth transitions.

**Step 5: Resampling**
After time-scaling, resample to restore the original audio duration.

**Result:** Pitch changed, duration unchanged.

**Learning Resources:**
- **Paper**: "An Overlap-Add Technique Based on Waveform Similarity (WSOLA)" (Verhelst & Roelands, 1993)
- **Tutorial**: https://www.dsprelated.com/freebooks/sasp/Overlap_Add_OLA_Processing.html
- **Implementation**: See `PitchShifter.cpp` for detailed comments

---

### 3. Formant Preservation

**What It Does:** Preserves vocal character when pitch shifting.

**Why It's Needed:** Without formant preservation, pitch-shifted vocals sound unnatural (chipmunk or giant effect).

**What Are Formants?**
- Frequency regions boosted by the vocal tract
- F1 (200-1000 Hz): Related to jaw openness
- F2 (700-2500 Hz): Related to tongue position
- Different vowels have different formant patterns

**How It Works (Simplified Approach):**

**Method 1: LPC Analysis** (Advanced)
1. Analyze spectral envelope using Linear Predictive Coding
2. Separate excitation (pitch) from envelope (formants)
3. Shift pitch, keep envelope
4. Recombine

**Method 2: Filter Bank** (Current Implementation)
1. Use a bank of bandpass filters to analyze spectral envelope
2. Shift filter frequencies in opposite direction of pitch shift
3. Recombine to maintain formant positions

**Learning Resources:**
- **Book**: "Digital Signal Processing of Speech Signals" (Rabiner & Schafer)
- **Tutorial**: https://ccrma.stanford.edu/~jos/filters/Formant_Preservation.html
- **Wikipedia**: https://en.wikipedia.org/wiki/Formant

---

### 4. Scale Quantization

**What It Does:** Snaps detected notes to the nearest note in the selected scale.

**Why It's Needed:** Ensures corrected pitch always matches the musical key/scale.

**How It Works:**
1. Get detected MIDI note (e.g., 64.3 = E♭ slightly flat)
2. Find nearest scale note in current key/scale
3. Return quantized note (e.g., 64.0 = E)

**Example:**
- Key: C Major (notes: C, D, E, F, G, A, B)
- Detected: E♭ (63)
- Nearest scale note: E (64) or D (62)
- Choose E (closer)

**Learning Resources:**
- **Music Theory**: Any basic music theory book on scales
- **Implementation**: See `PitchMapper::quantizeToScale()` in `PitchMapper.cpp`

---

## Import Sources & Dependencies

### JUCE Framework Imports

**Location:** `plugin/JUCE/` (submodule or local copy)

**Main Modules Used:**

1. **`juce_audio_processors`**
   - `AudioProcessor`: Base class for plugins
   - `AudioProcessorValueTreeState`: Parameter management
   - `AudioProcessorEditor`: Base class for UI
   - **Used in:** `PluginProcessor.*`, `PluginEditor.*`, all DSP components

2. **`juce_audio_basics`**
   - `AudioBuffer<float>`: Audio buffer container
   - `MidiBuffer`: MIDI message container
   - **Used in:** All DSP components

3. **`juce_dsp`**
   - `IIR::Filter`: IIR filters for formant processing
   - DSP utilities
   - **Used in:** `FormantProcessor.*`

4. **`juce_gui_extra`**
   - Extended GUI components
   - **Used in:** `PluginEditor.*`

**How to Find JUCE Documentation:**
- Online: https://juce.com/learn/documentation
- Local: `plugin/JUCE/docs/` (if included)
- Examples: `plugin/JUCE/examples/`

---

### Project-Specific Imports

All project-specific headers are in `plugin/Source/`:

- **`ParameterIDs.h`**: Parameter IDs and enums
- **`DSPConfig.h`**: DSP constants
- **`Utilities.h`**: Utility functions
- **`dsp/*.h`**: DSP component headers

**Import Pattern:**
```cpp
// External (JUCE)
#include <juce_audio_processors/juce_audio_processors.h>

// Project-specific (relative to Source/)
#include "ParameterIDs.h"
#include "dsp/TunerEngine.h"
```

---

## Code Navigation Guide

### Where to Start

**For Understanding the Overall Flow:**
1. Read `PluginProcessor.cpp::processBlock()` - Entry point
2. Read `TunerEngine.cpp::process()` - Main DSP flow
3. Follow the signal path through each component

**For Understanding Pitch Detection:**
1. Start: `PitchDetector.h` - Interface
2. Read: `PitchDetector.cpp` - Implementation
3. Reference: YIN algorithm paper (see Learning Resources)

**For Understanding Pitch Correction:**
1. Start: `LeadCorrection.h` - Interface
2. Read: `LeadCorrection.cpp::process()` - Correction logic
3. Read: `PitchShifter.*` - WSOLA implementation

**For Understanding Harmony Generation:**
1. Start: `HarmonyVoice.h` - Interface
2. Read: `HarmonyVoice.cpp::process()` - Harmony generation
3. Read: `PitchMapper::calculateHarmonyTarget()` - Interval calculation

**For Understanding the UI:**
1. Start: `PluginEditor.h` - Component structure
2. Read: `PluginEditor.cpp` - Implementation
3. Look for `*Attachment` classes - Parameter connections

---

### Key Files to Understand

**Entry Points:**
- `PluginProcessor.cpp::processBlock()` - Audio processing entry
- `PluginProcessor.cpp::createParameterLayout()` - Parameter definitions

**Core DSP:**
- `TunerEngine.cpp::process()` - Main processing pipeline
- `PitchDetector.cpp::process()` - Pitch detection
- `PitchMapper.cpp::map()` - Pitch mapping
- `LeadCorrection.cpp::process()` - Lead correction
- `PitchShifter.cpp::process()` - Pitch shifting

**Configuration:**
- `DSPConfig.h` - All DSP constants
- `ParameterIDs.h` - All parameters and enums

**Utilities:**
- `Utilities.h` - Conversion functions (frequency ↔ MIDI, etc.)

---

### Common Patterns

**1. Prepare/Reset Pattern**
All DSP components follow this pattern:
```cpp
void prepare(double sampleRate, int maxBlockSize, ...);
void reset();
```
- `prepare()`: Called when audio starts or settings change
- `reset()`: Called when playback stops

**2. Parameter Update Pattern**
```cpp
void updateFromParameters(juce::AudioProcessorValueTreeState &apvts);
```
- Called at start of each `process()` call
- Reads thread-safe parameter values
- Updates internal state

**3. Processing Pattern**
```cpp
void process(juce::AudioBuffer<float> &buffer, ...);
```
- Modifies buffer in place (or uses separate input/output)
- Must be real-time safe (no allocation, no blocking)

---

## Learning Resources

### DSP Fundamentals

**Books:**
- **"The Scientist and Engineer's Guide to Digital Signal Processing"** by Steven W. Smith
  - Free online: https://www.dspguide.com/
  - Covers: FFT, filters, convolution, etc.

- **"Understanding Digital Signal Processing"** by Richard G. Lyons
  - Comprehensive DSP textbook

**Online Courses:**
- **Coursera**: "Audio Signal Processing for Music Applications"
- **YouTube**: "The Audio Programmer" channel

---

### Pitch Detection

**Papers:**
- **YIN Algorithm**: "YIN, a fundamental frequency estimator for speech and music" (de Cheveigné & Kawahara, 2002)
  - Original paper describing the algorithm

**Resources:**
- Wikipedia: https://en.wikipedia.org/wiki/YIN_(algorithm)
- Implementation: See `PitchDetector.cpp` for detailed inline comments

---

### Pitch Shifting

**Papers:**
- **WSOLA**: "An Overlap-Add Technique Based on Waveform Similarity (WSOLA)" (Verhelst & Roelands, 1993)

**Tutorials:**
- DSP Related: https://www.dsprelated.com/freebooks/sasp/
- "Time Stretching and Pitch Shifting of Audio Signals" by Laroche & Dolson

**Alternative Algorithms:**
- **PSOLA** (Pitch Synchronous Overlap-Add)
- **Phase Vocoder** (frequency-domain approach)
- **PSOLA vs WSOLA**: WSOLA is more robust to pitch variations

---

### Formant Processing

**Books:**
- **"Digital Signal Processing of Speech Signals"** by Rabiner & Schafer
  - Comprehensive speech processing textbook

**Tutorials:**
- CCRMA Stanford: https://ccrma.stanford.edu/~jos/filters/
- Formant preservation techniques

**Concepts:**
- **LPC** (Linear Predictive Coding): Advanced formant analysis
- **Cepstral Analysis**: Alternative formant extraction method

---

### JUCE Framework

**Official Documentation:**
- https://juce.com/learn/documentation
- **Tutorials**: https://juce.com/learn/tutorials
- **API Reference**: https://docs.juce.com/

**Key JUCE Concepts:**
- **AudioProcessor**: Base class for plugins
- **AudioProcessorValueTreeState**: Thread-safe parameter management
- **AudioBuffer**: Audio data container
- **AudioProcessorEditor**: UI base class

**JUCE Examples:**
- Located in: `plugin/JUCE/examples/`
- **AudioPlugin**: Basic plugin template
- **DSP**: DSP examples (filters, effects)

---

### Music Theory (for Scale Mapping)

**Resources:**
- **"Music Theory for Computer Musicians"** by Michael Hewitt
- **Online**: https://www.musictheory.net/
- **Scales**: Understanding major, minor, harmonic minor, etc.

**Key Concepts:**
- **Semitones**: 12 semitones per octave
- **Scale Degrees**: Position within a scale (1st, 2nd, 3rd, etc.)
- **Intervals**: Distance between notes (3rd, 5th, octave, etc.)

---

### Real-Time Audio Programming

**Books:**
- **"Real-Time Digital Signal Processing"** by Sen M. Kuo
- **"Designing Audio Effect Plugins in C++"** by Will Pirkle

**Key Concepts:**
- **Audio Thread**: Real-time thread (must not block)
- **Message Thread**: UI thread (can block)
- **Lock-Free Programming**: Atomic operations for thread safety
- **Buffer Management**: Pre-allocate buffers, avoid runtime allocation

---

## Quick Reference

### File Purpose Summary

| File | Purpose |
|------|---------|
| `PluginProcessor.*` | Host interface, plugin lifecycle |
| `PluginEditor.*` | User interface |
| `TunerEngine.*` | DSP orchestration |
| `PitchDetector.*` | YIN pitch detection |
| `PitchMapper.*` | Key/scale mapping |
| `LeadCorrection.*` | Lead vocal correction |
| `PitchShifter.*` | WSOLA pitch shifting |
| `HarmonyVoice.*` | Harmony voice generation |
| `FormantProcessor.*` | Formant preservation |
| `ParameterIDs.h` | Parameter definitions |
| `DSPConfig.h` | DSP constants |
| `Utilities.h` | Utility functions |

### Common Tasks

**Adding a New Parameter:**
1. Add ID to `ParameterIDs.h`
2. Add to `createParameterLayout()` in `PluginProcessor.cpp`
3. Read in component's `updateFromParameters()`
4. Add UI control in `PluginEditor.*`

**Modifying DSP Algorithm:**
1. Find relevant component in `dsp/`
2. Modify algorithm in `.cpp` file
3. Adjust constants in `DSPConfig.h` if needed

**Understanding Signal Flow:**
1. Start at `TunerEngine::process()`
2. Follow the signal path through each component
3. Check buffer allocations in `TunerEngine::prepare()`

---

## Conclusion

This guide provides a comprehensive overview of the NovaTune architecture. For deeper understanding:

1. **Read the code** - Each file has extensive inline documentation
2. **Follow the signal flow** - Start at `processBlock()` and trace through
3. **Experiment** - Modify parameters in `DSPConfig.h` to see effects
4. **Reference papers** - For algorithm details, see the original papers

**Questions?** Check the inline comments in each file - they explain the "why" behind each design decision.

---

*Last Updated: Based on current codebase structure*

