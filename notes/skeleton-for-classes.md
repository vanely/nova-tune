Awesome, let’s turn NovaTune into something you can literally drop into a JUCE project.

I’ll give you:

1. **Directory / file-level breakdown**
2. **Parameter layout (JUCE `AudioProcessorValueTreeState`)**
3. **Skeleton C++ classes** for the main pieces (processor, editor, DSP core, pitch detector, harmonies)

You can copy-paste these and then iteratively fill in DSP and UI.

---

## 1. Suggested directory & file structure

```text
NovaTune/
  Source/
    PluginProcessor.h
    PluginProcessor.cpp
    PluginEditor.h
    PluginEditor.cpp

    ParameterIDs.h
    DSPConfig.h
    Utilities.h
    Utilities.cpp

    dsp/
      TunerEngine.h
      TunerEngine.cpp

      PitchDetector.h
      PitchDetector.cpp

      PitchMapper.h
      PitchMapper.cpp

      LeadCorrection.h
      LeadCorrection.cpp

      HarmonyVoice.h
      HarmonyVoice.cpp

      FormantProcessor.h
      FormantProcessor.cpp
```

You can merge/split files later, but this separation keeps things clean:

* `PluginProcessor` / `PluginEditor` – JUCE boilerplate, host glue, UI.
* `ParameterIDs` – all parameter IDs and enums in one place.
* `DSPConfig` – constants (max voices, ranges, etc.).
* `TunerEngine` – central DSP core orchestrating everything.
* `PitchDetector` – F0 estimation.
* `PitchMapper` – key/scale mapping + harmony interval logic.
* `LeadCorrection` – main pitch shifting for lead voice.
* `HarmonyVoice` – per-voice harmonizer DSP.
* `FormantProcessor` – spectral tweaks for formant shifting (can be stubbed initially).

---

## 2. Parameter IDs & layout

### 2.1 `ParameterIDs.h`

```cpp
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs
{
    // Global / lead
    static constexpr const char* key            = "key";
    static constexpr const char* scale          = "scale";
    static constexpr const char* inputType      = "inputType";
    static constexpr const char* retuneSpeed    = "retuneSpeed";
    static constexpr const char* humanize       = "humanize";
    static constexpr const char* vibratoAmount  = "vibratoAmount";
    static constexpr const char* mix            = "mix";
    static constexpr const char* bypass         = "bypass";

    static constexpr const char* qualityMode    = "qualityMode";

    // Harmony presets (optional exposed parameter; can be UI-only too)
    static constexpr const char* harmonyPreset  = "harmonyPreset";

    // Harmony voice common naming scheme: v[A|B|C]_<param>
    // Voice A
    static constexpr const char* A_enabled          = "A_enabled";
    static constexpr const char* A_mode             = "A_mode";
    static constexpr const char* A_intervalDiatonic = "A_intervalDiatonic";
    static constexpr const char* A_intervalSemi     = "A_intervalSemi";
    static constexpr const char* A_level            = "A_level";
    static constexpr const char* A_pan              = "A_pan";
    static constexpr const char* A_formantShift     = "A_formantShift";
    static constexpr const char* A_humTiming        = "A_humTiming";
    static constexpr const char* A_humPitch         = "A_humPitch";

    // Voice B
    static constexpr const char* B_enabled          = "B_enabled";
    static constexpr const char* B_mode             = "B_mode";
    static constexpr const char* B_intervalDiatonic = "B_intervalDiatonic";
    static constexpr const char* B_intervalSemi     = "B_intervalSemi";
    static constexpr const char* B_level            = "B_level";
    static constexpr const char* B_pan              = "B_pan";
    static constexpr const char* B_formantShift     = "B_formantShift";
    static constexpr const char* B_humTiming        = "B_humTiming";
    static constexpr const char* B_humPitch         = "B_humPitch";

    // Voice C
    static constexpr const char* C_enabled          = "C_enabled";
    static constexpr const char* C_mode             = "C_mode";
    static constexpr const char* C_intervalDiatonic = "C_intervalDiatonic";
    static constexpr const char* C_intervalSemi     = "C_intervalSemi";
    static constexpr const char* C_level            = "C_level";
    static constexpr const char* C_pan              = "C_pan";
    static constexpr const char* C_formantShift     = "C_formantShift";
    static constexpr const char* C_humTiming        = "C_humTiming";
    static constexpr const char* C_humPitch         = "C_humPitch";
} // namespace ParamIDs

namespace NovaTuneEnums
{
    enum class Key
    {
        C, Cs, D, Ds, E, F, Fs, G, Gs, A, As, B,
        numKeys
    };

    enum class Scale
    {
        Major,
        NaturalMinor,
        HarmonicMinor,
        MelodicMinor,
        Chromatic,
        numScales
    };

    enum class InputType
    {
        Soprano,
        AltoTenor,
        LowMale,
        Instrument,
        numInputTypes
    };

    enum class QualityMode
    {
        Live,
        Mix,
        numQualityModes
    };

    enum class HarmonyMode
    {
        Diatonic,
        Semitone,
        numHarmonyModes
    };

    // For diatonic intervals: -7..+7 scale degrees (we’ll encode them as 0..14 in a choice param)
    inline const juce::StringArray getDiatonicIntervalNames()
    {
        // Internal mapping: 0 = -7, 7 = 0, 14 = +7 for example
        return {
            "-7th", "-6th", "-5th", "-4th", "-3rd", "-2nd", "-Unison",
            "Unison",
            "+2nd", "+3rd", "+4th", "+5th", "+6th", "+7th", "+Oct (scale)"
        };
    }

    inline const juce::StringArray getKeyNames()
    {
        return { "C", "C#", "D", "D#", "E", "F", "F#", "G",
                 "G#", "A", "A#", "B" };
    }

    inline const juce::StringArray getScaleNames()
    {
        return { "Major", "Natural Minor", "Harmonic Minor", "Melodic Minor", "Chromatic" };
    }

    inline const juce::StringArray getInputTypeNames()
    {
        return { "Soprano", "Alto/Tenor", "Low Male", "Instrument" };
    }

    inline const juce::StringArray getQualityModeNames()
    {
        return { "Live", "Mix" };
    }

    inline const juce::StringArray getHarmonyModeNames()
    {
        return { "Diatonic", "Semitone" };
    }

    inline const juce::StringArray getHarmonyPresetNames()
    {
        return {
            "None",
            "Pop 3rd Up",
            "Pop 3rd & 5th",
            "Thirds Above & Below",
            "Fifths Wide",
            "Octave Double",
            "Octave + 3rd",
            "Choir Stack"
        };
    }
} // namespace NovaTuneEnums
```

### 2.2 `DSPConfig.h` (ranges & constants)

```cpp
#pragma once

namespace DSPConfig
{
    constexpr int maxHarmonyVoices = 3;

    // Parameter ranges
    constexpr float retuneSpeedMin   = 0.0f;
    constexpr float retuneSpeedMax   = 100.0f;
    constexpr float humanizeMin      = 0.0f;
    constexpr float humanizeMax      = 100.0f;
    constexpr float vibratoMin       = 0.0f;
    constexpr float vibratoMax       = 100.0f;
    constexpr float mixMin           = 0.0f;
    constexpr float mixMax           = 100.0f;

    constexpr float levelMinDb       = -48.0f;
    constexpr float levelMaxDb       = 6.0f;

    constexpr float panMin           = -1.0f;
    constexpr float panMax           = 1.0f;

    constexpr float formantMin       = -6.0f;
    constexpr float formantMax       = 6.0f;

    constexpr float humTimingMinMs   = 0.0f;
    constexpr float humTimingMaxMs   = 30.0f;

    constexpr float humPitchMinCents = 0.0f;
    constexpr float humPitchMaxCents = 15.0f;
}
```

### 2.3 Parameter layout function (for `AudioProcessorValueTreeState`)

We’ll define a `createParameterLayout()` function used by the processor.

```cpp
// In PluginProcessor.h (declaration) or a separate Parameters.cpp:
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
```

```cpp
// In PluginProcessor.cpp
#include "ParameterIDs.h"
#include "DSPConfig.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    using namespace ParamIDs;
    using namespace NovaTuneEnums;

    // Key (choice)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        key, "Key",
        getKeyNames(), 0));

    // Scale (choice)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        scale, "Scale",
        getScaleNames(), 0));

    // Input type
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        inputType, "Input Type",
        getInputTypeNames(), 1 /* Alto/Tenor as default */));

    // Retune Speed
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        retuneSpeed, "Retune Speed",
        juce::NormalisableRange<float>(DSPConfig::retuneSpeedMin, DSPConfig::retuneSpeedMax),
        50.0f));

    // Humanize
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        humanize, "Humanize",
        juce::NormalisableRange<float>(DSPConfig::humanizeMin, DSPConfig::humanizeMax),
        25.0f));

    // Vibrato Amount
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        vibratoAmount, "Vibrato Amount",
        juce::NormalisableRange<float>(DSPConfig::vibratoMin, DSPConfig::vibratoMax),
        0.0f));

    // Mix
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        mix, "Mix",
        juce::NormalisableRange<float>(DSPConfig::mixMin, DSPConfig::mixMax),
        100.0f));

    // Bypass
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        bypass, "Bypass", false));

    // Quality Mode
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        qualityMode, "Quality Mode",
        getQualityModeNames(), 0 /* Live */));

    // Harmony preset
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        harmonyPreset, "Harmony Preset",
        getHarmonyPresetNames(), 0));

    auto addHarmonyVoiceParams = [&params](const char* enabledId,
                                           const char* modeId,
                                           const char* intervalDiatonicId,
                                           const char* intervalSemiId,
                                           const char* levelId,
                                           const char* panId,
                                           const char* formantId,
                                           const char* humTimingId,
                                           const char* humPitchId,
                                           const juce::String& voiceName)
    {
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            enabledId, voiceName + " Enabled", false));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            modeId, voiceName + " Mode",
            NovaTuneEnums::getHarmonyModeNames(), 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            intervalDiatonicId, voiceName + " Diatonic Interval",
            NovaTuneEnums::getDiatonicIntervalNames(), 7 /* Unison-ish */));

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            intervalSemiId, voiceName + " Semitone Interval",
            -12, 12, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            levelId, voiceName + " Level (dB)",
            juce::NormalisableRange<float>(DSPConfig::levelMinDb, DSPConfig::levelMaxDb),
            -12.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            panId, voiceName + " Pan",
            juce::NormalisableRange<float>(DSPConfig::panMin, DSPConfig::panMax),
            0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            formantId, voiceName + " Formant Shift",
            juce::NormalisableRange<float>(DSPConfig::formantMin, DSPConfig::formantMax),
            0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            humTimingId, voiceName + " Humanize Timing (ms)",
            juce::NormalisableRange<float>(DSPConfig::humTimingMinMs, DSPConfig::humTimingMaxMs),
            5.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            humPitchId, voiceName + " Humanize Pitch (cents)",
            juce::NormalisableRange<float>(DSPConfig::humPitchMinCents, DSPConfig::humPitchMaxCents),
            3.0f));
    };

    // Voice A
    addHarmonyVoiceParams(
        A_enabled, A_mode, A_intervalDiatonic, A_intervalSemi,
        A_level, A_pan, A_formantShift, A_humTiming, A_humPitch,
        "Harmony A");

    // Voice B
    addHarmonyVoiceParams(
        B_enabled, B_mode, B_intervalDiatonic, B_intervalSemi,
        B_level, B_pan, B_formantShift, B_humTiming, B_humPitch,
        "Harmony B");

    // Voice C
    addHarmonyVoiceParams(
        C_enabled, C_mode, C_intervalDiatonic, C_intervalSemi,
        C_level, C_pan, C_formantShift, C_humTiming, C_humPitch,
        "Harmony C");

    return { params.begin(), params.end() };
}
```

---

## 3. PluginProcessor skeleton

### 3.1 `PluginProcessor.h`

```cpp
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterIDs.h"
#include "dsp/TunerEngine.h"

class NovaTuneAudioProcessor : public juce::AudioProcessor
{
public:
    using APVTS = juce::AudioProcessorValueTreeState;

    NovaTuneAudioProcessor();
    ~NovaTuneAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    APVTS& getValueTreeState() { return apvts; }
    const APVTS& getValueTreeState() const { return apvts; }

private:
    APVTS apvts;
    TunerEngine tunerEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NovaTuneAudioProcessor)
};
```

### 3.2 `PluginProcessor.cpp`

```cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.h"

extern juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

NovaTuneAudioProcessor::NovaTuneAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

NovaTuneAudioProcessor::~NovaTuneAudioProcessor() = default;

const juce::String NovaTuneAudioProcessor::getName() const
{
    return "NovaTune";
}

bool NovaTuneAudioProcessor::acceptsMidi() const          { return false; }
bool NovaTuneAudioProcessor::producesMidi() const         { return false; }
bool NovaTuneAudioProcessor::isMidiEffect() const         { return false; }
double NovaTuneAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int NovaTuneAudioProcessor::getNumPrograms()              { return 1; }
int NovaTuneAudioProcessor::getCurrentProgram()           { return 0; }
void NovaTuneAudioProcessor::setCurrentProgram (int)      {}
const juce::String NovaTuneAudioProcessor::getProgramName (int) { return {}; }
void NovaTuneAudioProcessor::changeProgramName (int, const juce::String&) {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NovaTuneAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Only allow stereo in/out for now
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}
#endif

void NovaTuneAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    tunerEngine.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels());
}

void NovaTuneAudioProcessor::releaseResources()
{
    tunerEngine.reset();
}

void NovaTuneAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const bool isBypassed = apvts.getRawParameterValue (ParamIDs::bypass)->load() > 0.5f;

    if (isBypassed)
        return;

    tunerEngine.process (buffer, midi, apvts);
}

bool NovaTuneAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* NovaTuneAudioProcessor::createEditor()
{
    return new NovaTuneAudioProcessorEditor (*this);
}

void NovaTuneAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void NovaTuneAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}
```

---

## 4. TunerEngine & DSP skeletons

### 4.1 `dsp/TunerEngine.h`

```cpp
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PitchDetector.h"
#include "PitchMapper.h"
#include "LeadCorrection.h"
#include "HarmonyVoice.h"
#include "FormantProcessor.h"
#include "../DSPConfig.h"

class TunerEngine
{
public:
    TunerEngine() = default;
    ~TunerEngine() = default;

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void process (juce::AudioBuffer<float>& buffer,
                  juce::MidiBuffer& midi,
                  juce::AudioProcessorValueTreeState& apvts);

private:
    double sampleRate { 44100.0 };
    int    samplesPerBlock { 512 };
    int    numChannels { 2 };

    PitchDetector  pitchDetector;
    PitchMapper    pitchMapper;
    LeadCorrection leadCorrection;

    std::array<HarmonyVoice, DSPConfig::maxHarmonyVoices> harmonyVoices;

    // Temporary buffers
    juce::AudioBuffer<float> leadBuffer;      // corrected lead
    juce::AudioBuffer<float> harmonyBuffer;   // sum of harmonies

    void updateFromParameters (juce::AudioProcessorValueTreeState& apvts);
};
```

### 4.2 `dsp/TunerEngine.cpp`

```cpp
#include "TunerEngine.h"
#include "../ParameterIDs.h"

void TunerEngine::prepare (double sr, int blockSize, int channels)
{
    sampleRate     = sr;
    samplesPerBlock = blockSize;
    numChannels     = channels;

    leadBuffer.setSize (numChannels, samplesPerBlock);
    harmonyBuffer.setSize (numChannels, samplesPerBlock);

    pitchDetector.prepare (sampleRate, samplesPerBlock);
    pitchMapper.prepare (sampleRate);
    leadCorrection.prepare (sampleRate, samplesPerBlock, numChannels);

    for (auto& v : harmonyVoices)
        v.prepare (sampleRate, samplesPerBlock, numChannels);
}

void TunerEngine::reset()
{
    pitchDetector.reset();
    pitchMapper.reset();
    leadCorrection.reset();

    for (auto& v : harmonyVoices)
        v.reset();
}

void TunerEngine::updateFromParameters (juce::AudioProcessorValueTreeState& apvts)
{
    using namespace ParamIDs;

    // Example: update lead correction parameters
    const auto retune  = apvts.getRawParameterValue (retuneSpeed)->load();
    const auto human   = apvts.getRawParameterValue (humanize)->load();
    const auto vibrato = apvts.getRawParameterValue (vibratoAmount)->load();
    const auto mix     = apvts.getRawParameterValue (mix)->load();

    leadCorrection.setRetuneSpeed (retune);
    leadCorrection.setHumanize (human);
    leadCorrection.setVibrato (vibrato);
    leadCorrection.setMix (mix / 100.0f);

    // Key / scale / input type / quality etc passed to mapper / detector
    // (You’ll convert choice indexes to internal enums.)
}

void TunerEngine::process (juce::AudioBuffer<float>& buffer,
                           juce::MidiBuffer& /*midi*/,
                           juce::AudioProcessorValueTreeState& apvts)
{
    const int numSamples = buffer.getNumSamples();

    if (numSamples != samplesPerBlock)
    {
        // simple safety: resize temp buffers if DAW changes block size at runtime
        samplesPerBlock = numSamples;
        leadBuffer.setSize (numChannels, samplesPerBlock, false, false, true);
        harmonyBuffer.setSize (numChannels, samplesPerBlock, false, false, true);

        leadCorrection.prepare (sampleRate, samplesPerBlock, numChannels);
        for (auto& v : harmonyVoices)
            v.prepare (sampleRate, samplesPerBlock, numChannels);
    }

    updateFromParameters (apvts);

    // 1) Analyze pitch
    pitchDetector.process (buffer); // You’ll implement analysis-only in here

    // 2) Map pitch -> target notes/harmonies
    pitchMapper.process (pitchDetector);

    // 3) Lead correction
    leadBuffer.makeCopyOf (buffer, true);
    leadCorrection.process (leadBuffer, pitchDetector, pitchMapper);

    // 4) Harmonies
    harmonyBuffer.clear();
    for (int i = 0; i < DSPConfig::maxHarmonyVoices; ++i)
    {
        harmonyVoices[i].updateFromParameters (i, apvts); // i = 0->A, 1->B, 2->C
        harmonyVoices[i].process (harmonyBuffer, leadBuffer, pitchDetector, pitchMapper);
    }

    // 5) Combine lead + harmony into output
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* out = buffer.getWritePointer (ch);
        auto* lead = leadBuffer.getReadPointer (ch);
        auto* harm = harmonyBuffer.getReadPointer (ch);

        for (int n = 0; n < numSamples; ++n)
        {
            out[n] = lead[n] + harm[n]; // You may want wet/dry or separate mix controls later
        }
    }
}
```

---

## 5. Supporting DSP class skeletons

### 5.1 `dsp/PitchDetector.h`

```cpp
#pragma once

#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>

class PitchDetector
{
public:
    PitchDetector() = default;

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    void process (const juce::AudioBuffer<float>& buffer);

    // Accessors for latest analysis results
    float getLastFrequencyHz() const noexcept { return lastFrequencyHz; }
    float getLastMidiNote() const noexcept    { return lastMidiNote; }
    bool  isVoiced() const noexcept           { return voiced; }

private:
    double sampleRate { 44100.0 };
    int blockSize { 512 };

    float lastFrequencyHz { 0.0f };
    float lastMidiNote    { 0.0f };
    bool  voiced          { false };

    juce::AudioBuffer<float> monoBuffer;

    // Internal methods for YIN/autocorrelation/etc.
    float estimateFrequency (const float* samples, int numSamples);
};
```

### 5.2 `dsp/PitchDetector.cpp`

```cpp
#include "PitchDetector.h"
#include <cmath>

void PitchDetector::prepare (double sr, int bs)
{
    sampleRate = sr;
    blockSize  = bs;
    monoBuffer.setSize (1, blockSize);
}

void PitchDetector::reset()
{
    lastFrequencyHz = 0.0f;
    lastMidiNote    = 0.0f;
    voiced          = false;
}

void PitchDetector::process (const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples <= 0)
        return;

    // Downmix to mono
    monoBuffer.setSize (1, numSamples, false, false, true);
    auto* mono = monoBuffer.getWritePointer (0);
    buffer.copyFrom (0, 0, buffer, 0, 0, numSamples);
    for (int ch = 1; ch < numChannels; ++ch)
        monoBuffer.addFrom (0, 0, buffer, ch, 0, numSamples);

    // Average if multi-channel
    if (numChannels > 1)
        for (int n = 0; n < numSamples; ++n)
            mono[n] /= static_cast<float>(numChannels);

    // TODO: apply window, pre-filtering, etc.
    const float freq = estimateFrequency (mono, numSamples);

    lastFrequencyHz = freq;
    if (freq > 0.0f)
    {
        lastMidiNote = 69.0f + 12.0f * std::log2 (freq / 440.0f);
        voiced = true;
    }
    else
    {
        lastMidiNote = 0.0f;
        voiced = false;
    }
}

float PitchDetector::estimateFrequency (const float* samples, int numSamples)
{
    // TODO: Implement YIN / autocorrelation-based pitch detection.
    // For now, stub out & return 0.
    juce::ignoreUnused (samples, numSamples);
    return 0.0f;
}
```

### 5.3 `dsp/PitchMapper.h`

```cpp
#pragma once

#include "PitchDetector.h"
#include "../ParameterIDs.h"

struct PitchTarget
{
    float leadMidiNote   { 0.0f };
    float targetMidiNote { 0.0f }; // corrected note
};

class PitchMapper
{
public:
    PitchMapper() = default;

    void prepare (double sampleRate) { juce::ignoreUnused (sampleRate); }
    void reset() {}

    void updateFromParameters (juce::AudioProcessorValueTreeState& apvts);
    void process (const PitchDetector& detector);

    const PitchTarget& getTarget() const noexcept { return target; }

    // For harmonies, you’ll add methods that map diatonic/semitone intervals.

private:
    PitchTarget target;

    int keyIndex   { 0 };
    int scaleIndex { 0 };
};
```

### 5.4 `dsp/PitchMapper.cpp`

```cpp
#include "PitchMapper.h"

using APVTS = juce::AudioProcessorValueTreeState;
using namespace ParamIDs;
using namespace NovaTuneEnums;

void PitchMapper::updateFromParameters (APVTS& apvts)
{
    keyIndex   = static_cast<int> (apvts.getRawParameterValue (key)->load());
    scaleIndex = static_cast<int> (apvts.getRawParameterValue (scale)->load());
}

void PitchMapper::process (const PitchDetector& detector)
{
    target.leadMidiNote = detector.getLastMidiNote();

    if (! detector.isVoiced())
    {
        target.targetMidiNote = target.leadMidiNote;
        return;
    }

    // TODO: Apply key/scale logic.
    // For now, just naive rounding to nearest semitone.
    target.targetMidiNote = std::round (target.leadMidiNote);

    juce::ignoreUnused (keyIndex, scaleIndex);
}
```

### 5.5 `dsp/LeadCorrection.h`

```cpp
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "PitchDetector.h"
#include "PitchMapper.h"

class LeadCorrection
{
public:
    LeadCorrection() = default;

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void setRetuneSpeed (float value);
    void setHumanize (float value);
    void setVibrato (float value);
    void setMix (float wet);

    void process (juce::AudioBuffer<float>& buffer,
                  const PitchDetector& detector,
                  const PitchMapper& mapper);

private:
    double sr { 44100.0 };
    int blockSize { 512 };
    int channels { 2 };

    float retuneSpeed { 50.0f }; // 0–100
    float humanizeAmount { 25.0f };
    float vibratoAmount { 0.0f };
    float mix { 1.0f };

    // TODO: internal WSOLA/PSOLA state
};
```

### 5.6 `dsp/LeadCorrection.cpp`

```cpp
#include "LeadCorrection.h"

void LeadCorrection::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    sr       = sampleRate;
    blockSize = samplesPerBlock;
    channels = numChannels;

    // TODO: allocate / prepare WSOLA/PSOLA buffers.
}

void LeadCorrection::reset()
{
    // TODO: reset internal state
}

void LeadCorrection::setRetuneSpeed (float value)
{
    retuneSpeed = value;
}

void LeadCorrection::setHumanize (float value)
{
    humanizeAmount = value;
}

void LeadCorrection::setVibrato (float value)
{
    vibratoAmount = value;
}

void LeadCorrection::setMix (float wet)
{
    mix = juce::jlimit (0.0f, 1.0f, wet);
}

void LeadCorrection::process (juce::AudioBuffer<float>& buffer,
                              const PitchDetector& detector,
                              const PitchMapper& mapper)
{
    juce::ignoreUnused (detector);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // TODO:
    //  - compute pitch ratio from detector/mapper
    //  - smooth ratio based on retuneSpeed/humanize
    //  - apply WSOLA/PSOLA or other pitch-shift

    // For now, pass-through (so you can compile and wire UI first)
    juce::ignoreUnused (mapper, numSamples, numChannels);
}
```

### 5.7 `dsp/HarmonyVoice.h`

```cpp
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "../ParameterIDs.h"
#include "../DSPConfig.h"
#include "PitchDetector.h"
#include "PitchMapper.h"
#include "FormantProcessor.h"

class HarmonyVoice
{
public:
    HarmonyVoice() = default;

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    // voiceIndex: 0=A, 1=B, 2=C
    void updateFromParameters (int voiceIndex,
                               juce::AudioProcessorValueTreeState& apvts);

    // Writes this voice into harmonyBuffer (additive), based on leadBuffer as source
    void process (juce::AudioBuffer<float>& harmonyBuffer,
                  const juce::AudioBuffer<float>& leadBuffer,
                  const PitchDetector& detector,
                  const PitchMapper& mapper);

private:
    double sr { 44100.0 };
    int blockSize { 512 };
    int channels { 2 };

    bool enabled { false };
    NovaTuneEnums::HarmonyMode mode { NovaTuneEnums::HarmonyMode::Diatonic };
    int diatonicIntervalIndex { 7 }; // center = unison
    int semitoneOffset { 0 };

    float levelDb { -12.0f };
    float pan { 0.0f };
    float formant { 0.0f };
    float humTimingMs { 5.0f };
    float humPitchCents { 3.0f };

    juce::AudioBuffer<float> voiceBuffer;
    FormantProcessor formantProcessor;

    // TODO: per-voice pitch-shift state, delay line for timing humanization, etc.
};
```

### 5.8 `dsp/HarmonyVoice.cpp`

```cpp
#include "HarmonyVoice.h"

using APVTS = juce::AudioProcessorValueTreeState;
using namespace ParamIDs;
using namespace NovaTuneEnums;

void HarmonyVoice::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    sr       = sampleRate;
    blockSize = samplesPerBlock;
    channels = numChannels;

    voiceBuffer.setSize (channels, blockSize);
    formantProcessor.prepare (sampleRate, samplesPerBlock, numChannels);
}

void HarmonyVoice::reset()
{
    formantProcessor.reset();
}

void HarmonyVoice::updateFromParameters (int voiceIndex, APVTS& apvts)
{
    const char* enabledId = nullptr;
    const char* modeId    = nullptr;
    const char* diaId     = nullptr;
    const char* semiId    = nullptr;
    const char* levelId   = nullptr;
    const char* panId     = nullptr;
    const char* formId    = nullptr;
    const char* humTId    = nullptr;
    const char* humPId    = nullptr;

    switch (voiceIndex)
    {
        case 0:
            enabledId = A_enabled;
            modeId    = A_mode;
            diaId     = A_intervalDiatonic;
            semiId    = A_intervalSemi;
            levelId   = A_level;
            panId     = A_pan;
            formId    = A_formantShift;
            humTId    = A_humTiming;
            humPId    = A_humPitch;
            break;
        case 1:
            enabledId = B_enabled;
            modeId    = B_mode;
            diaId     = B_intervalDiatonic;
            semiId    = B_intervalSemi;
            levelId   = B_level;
            panId     = B_pan;
            formId    = B_formantShift;
            humTId    = B_humTiming;
            humPId    = B_humPitch;
            break;
        case 2:
            enabledId = C_enabled;
            modeId    = C_mode;
            diaId     = C_intervalDiatonic;
            semiId    = C_intervalSemi;
            levelId   = C_level;
            panId     = C_pan;
            formId    = C_formantShift;
            humTId    = C_humTiming;
            humPId    = C_humPitch;
            break;
        default:
            jassertfalse;
            return;
    }

    enabled = apvts.getRawParameterValue (enabledId)->load() > 0.5f;
    mode    = static_cast<HarmonyMode> (static_cast<int> (apvts.getRawParameterValue (modeId)->load()));
    diatonicIntervalIndex = static_cast<int> (apvts.getRawParameterValue (diaId)->load());
    semitoneOffset        = static_cast<int> (apvts.getRawParameterValue (semiId)->load());
    levelDb               = apvts.getRawParameterValue (levelId)->load();
    pan                   = apvts.getRawParameterValue (panId)->load();
    formant               = apvts.getRawParameterValue (formId)->load();
    humTimingMs           = apvts.getRawParameterValue (humTId)->load();
    humPitchCents         = apvts.getRawParameterValue (humPId)->load();
}

void HarmonyVoice::process (juce::AudioBuffer<float>& harmonyBuffer,
                            const juce::AudioBuffer<float>& leadBuffer,
                            const PitchDetector& detector,
                            const PitchMapper& mapper)
{
    juce::ignoreUnused (detector, mapper);

    if (! enabled)
        return;

    const int numSamples  = leadBuffer.getNumSamples();
    const int numChannels = leadBuffer.getNumChannels();

    voiceBuffer.setSize (numChannels, numSamples, false, false, true);
    voiceBuffer.makeCopyOf (leadBuffer, true);

    // TODO:
    //  - compute target MIDI note for this harmony (using mapper, mode, diatonicIntervalIndex, semitoneOffset)
    //  - compute pitch ratio and apply pitch shift to voiceBuffer
    //  - apply formantProcessor with 'formant'
    //  - apply timing humanization (delay line) & pitch humanization (LFO/jitter)
    //  - apply gain & pan
    const float gainLin = juce::Decibels::decibelsToGain (levelDb);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* dst = harmonyBuffer.getWritePointer (ch);
        auto* src = voiceBuffer.getReadPointer (ch);

        for (int n = 0; n < numSamples; ++n)
        {
            const float dry = src[n] * gainLin;
            // Simple pan: left = (1 - pan), right = (1 + pan)
            float panL = 1.0f;
            float panR = 1.0f;

            if (channels == 2)
            {
                panL = std::sqrt (0.5f * (1.0f - pan)); // basic constant-power pan
                panR = std::sqrt (0.5f * (1.0f + pan));
            }

            if (channels == 1)
            {
                dst[n] += dry;
            }
            else
            {
                if (ch == 0)
                    dst[n] += dry * panL;
                else
                    dst[n] += dry * panR;
            }
        }
    }
}
```

### 5.9 `dsp/FormantProcessor.h` (stub)

```cpp
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class FormantProcessor
{
public:
    FormantProcessor() = default;

    void prepare (double sampleRate, int samplesPerBlock, int numChannels)
    {
        juce::ignoreUnused (sampleRate, samplesPerBlock, numChannels);
    }

    void reset() {}

    // Apply formant shift to buffer in-place
    void process (juce::AudioBuffer<float>& buffer, float shiftSemitones)
    {
        juce::ignoreUnused (buffer, shiftSemitones);
        // TODO: implement spectral warp / envelope shift
    }
};
```

---

## 6. Basic editor skeleton

Just enough to compile and start wiring controls later.

### 6.1 `PluginEditor.h`

```cpp
#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class NovaTuneAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit NovaTuneAudioProcessorEditor (NovaTuneAudioProcessor&);
    ~NovaTuneAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    NovaTuneAudioProcessor& processor;

    // Example controls for main parameters
    juce::ComboBox keyBox;
    juce::ComboBox scaleBox;
    juce::Slider  retuneSlider;
    juce::Slider  humanizeSlider;
    juce::Slider  mixSlider;

    using APVTS = juce::AudioProcessorValueTreeState;
    using ComboAttachment  = APVTS::ComboBoxAttachment;
    using SliderAttachment = APVTS::SliderAttachment;

    std::unique_ptr<ComboAttachment>  keyAttachment;
    std::unique_ptr<ComboAttachment>  scaleAttachment;
    std::unique_ptr<SliderAttachment> retuneAttachment;
    std::unique_ptr<SliderAttachment> humanizeAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NovaTuneAudioProcessorEditor)
};
```

### 6.2 `PluginEditor.cpp`

```cpp
#include "PluginEditor.h"
#include "ParameterIDs.h"

NovaTuneAudioProcessorEditor::NovaTuneAudioProcessorEditor (NovaTuneAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    auto& apvts = processor.getValueTreeState();

    // Key
    addAndMakeVisible (keyBox);
    keyBox.addItemList (NovaTuneEnums::getKeyNames(), 1);
    keyAttachment = std::make_unique<ComboAttachment> (apvts, ParamIDs::key, keyBox);

    // Scale
    addAndMakeVisible (scaleBox);
    scaleBox.addItemList (NovaTuneEnums::getScaleNames(), 1);
    scaleAttachment = std::make_unique<ComboAttachment> (apvts, ParamIDs::scale, scaleBox);

    // Retune Speed
    addAndMakeVisible (retuneSlider);
    retuneSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    retuneSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    retuneAttachment = std::make_unique<SliderAttachment> (apvts, ParamIDs::retuneSpeed, retuneSlider);

    // Humanize
    addAndMakeVisible (humanizeSlider);
    humanizeSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    humanizeSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    humanizeAttachment = std::make_unique<SliderAttachment> (apvts, ParamIDs::humanize, humanizeSlider);

    // Mix
    addAndMakeVisible (mixSlider);
    mixSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    mixAttachment = std::make_unique<SliderAttachment> (apvts, ParamIDs::mix, mixSlider);

    setSize (500, 300);
}

NovaTuneAudioProcessorEditor::~NovaTuneAudioProcessorEditor() = default;

void NovaTuneAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    g.drawFittedText ("NovaTune (MVP Skeleton UI)", getLocalBounds(), juce::Justification::centredTop, 1);
}

void NovaTuneAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    auto topRow = area.removeFromTop (40);

    keyBox.setBounds (topRow.removeFromLeft (120).reduced (5));
    scaleBox.setBounds (topRow.removeFromLeft (140).reduced (5));

    auto knobRow = area.removeFromTop (120);
    retuneSlider.setBounds (knobRow.removeFromLeft (150).reduced (10));
    humanizeSlider.setBounds (knobRow.removeFromLeft (150).reduced (10));

    mixSlider.setBounds (area.removeFromBottom (40).reduced (10));
}
```
