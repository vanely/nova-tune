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

#### Linux-Specific Dependencies

On Linux, you need to install additional system libraries for JUCE:

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y \
    libasound2-dev \
    libfreetype-dev \
    libfontconfig1-dev \
    libx11-dev \
    libxcomposite-dev \
    libxext-dev \
    libxinerama-dev \
    libxrandr-dev \
    libxrender-dev \
    libglu1-mesa-dev \
    mesa-common-dev

# Or use the provided setup script:
sudo bash setup_linux_dependencies.sh
```

#### Windows-Specific Prerequisites

On Windows, you need to install the following:

1. **CMake** (3.22 or higher)
   - Download from: https://cmake.org/download/
   - Or via Chocolatey: `choco install cmake`
   - Make sure to add CMake to your PATH during installation

2. **JUCE Framework** (7.0+)
   - Already included as a submodule in `plugin/JUCE/`
   - No additional installation needed if submodule is initialized

3. **C++ Compiler** with C++17 support:
   - **Option A: Visual Studio 2019 or newer** (Recommended)
     - Install "Desktop development with C++" workload
     - Includes MSVC compiler
     - Visual Studio 2022 is recommended for best compatibility
   - **Option B: MinGW-w64**
     - Can be installed via MSYS2 or standalone installer

### Build Steps

#### Linux/macOS

```bash
# Clone the repository
git clone https://github.com/yourusername/NovaTune.git
cd NovaTune/plugin

# Initialize JUCE submodule (if using submodule approach)
git submodule update --init --recursive

# Create build directory
mkdir -p build && cd build

# Configure (Release build for best performance)
# CMAKE_EXPORT_COMPILE_COMMANDS generates compile_commands.json for clangd/LSP tools
# https://clangd.llvm.org/installation#project-setup
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
cmake --build . --config Release

# The plugin will be in:
# - Linux: build/NovaTune_artefacts/Release/VST3/
# - macOS: build/NovaTune_artefacts/Release/VST3/ and build/NovaTune_artefacts/Release/AU/
```

#### Windows

Open **Command Prompt** or **PowerShell** (or "Developer Command Prompt for VS" if using Visual Studio):

```cmd
REM Navigate to the plugin directory
cd NovaTune\plugin

REM Initialize JUCE submodule (if not already done)
git submodule update --init --recursive

REM Create build directory
mkdir build
cd build

REM Configure with CMake
REM For Visual Studio 2022 (64-bit Release):
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

REM OR for Visual Studio 2019:
REM cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

REM OR for MinGW (if using MinGW):
REM cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

REM Build the project
cmake --build . --config Release

REM OR if using Visual Studio, you can open the generated solution:
REM start NovaTune.sln
REM Then build from Visual Studio IDE (Release configuration)
```

**Windows Build Notes:**

- **Visual Studio Generator**: Use `-G "Visual Studio 17 2022"` for VS2022 or `-G "Visual Studio 16 2019"` for VS2019. CMake can auto-detect, but specifying ensures the correct version.
- **Architecture**: Use `-A x64` for 64-bit builds (most common). For 32-bit, use `-A Win32` (rarely needed).
- **Build Configuration**: On Windows with Visual Studio, you can build both Debug and Release from the same build folder:
  ```cmd
  cmake --build . --config Debug    # For debugging
  cmake --build . --config Release  # For release
  ```
- **Plugin Installation**: With `COPY_PLUGIN_AFTER_BUILD TRUE` in CMakeLists.txt, the plugin may be automatically copied to the VST3 directory, but you may need administrator privileges.

**Output Location:**
- VST3: `build\NovaTune_artefacts\Release\VST3\NovaTune.vst3`
- Standalone: `build\NovaTune_artefacts\Release\Standalone\NovaTune.exe`

**Optional Windows Build Script:**

You can create a `build_windows.bat` file in the `plugin` directory for convenience:

```batch
@echo off
cd plugin
git submodule update --init --recursive
if not exist build mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build . --config Release
echo Build complete! Plugin is in: NovaTune_artefacts\Release\VST3\
pause
```

**Note:** The `compile_commands.json` file will be generated in the `build/` directory. This file provides build context for language servers like clangd, enabling better code completion, error checking, and IDE integration. If your IDE doesn't automatically detect it, you may need to create a symlink (Linux/macOS) or junction (Windows) in the project root:

```bash
# Linux/macOS: From the plugin directory (optional, for IDE compatibility)
ln -s build/compile_commands.json compile_commands.json

# Windows: From the plugin directory (optional, for IDE compatibility)
# Using Command Prompt (requires admin privileges):
mklink compile_commands.json build\compile_commands.json
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
