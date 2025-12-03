#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.h"
#include "DSPConfig.h"

/**
 * PluginProcessor.cpp
 *
 * Implementation of the main audio plugin processor.
 */

//==============================================================================
// PARAMETER LAYOUT CREATION
//==============================================================================

/**
 * Create all parameters for the plugin.
 *
 * This is like defining your API schema - these are all the values
 * that can be automated, saved in presets, and controlled by the UI.
 */
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  using namespace ParamIDs;
  using namespace NovaTuneEnums;

  //==========================================================================
  // GLOBAL PARAMETERS
  //==========================================================================

  // Key selection (C, C#, D, etc.)
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID(key, 1),
      "Key",
      getKeyNames(),
      0 // Default: C
      ));

  // Scale selection
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID(scale, 1),
      "Scale",
      getScaleNames(),
      0 // Default: Major
      ));

  // Input type (affects pitch detection range)
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID(inputType, 1),
      "Input Type",
      getInputTypeNames(),
      1 // Default: Alto/Tenor
      ));

  // Retune Speed (the main "Auto-Tune" control)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID(retuneSpeed, 1),
      "Retune Speed",
      juce::NormalisableRange<float>(DSPConfig::retuneSpeedMin, DSPConfig::retuneSpeedMax, 0.1f),
      50.0f // Default: Medium speed
      ));

  // Humanize
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID(humanize, 1),
      "Humanize",
      juce::NormalisableRange<float>(DSPConfig::humanizeMin, DSPConfig::humanizeMax, 0.1f),
      25.0f));

  // Vibrato Amount
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID(vibratoAmount, 1),
      "Vibrato Amount",
      juce::NormalisableRange<float>(DSPConfig::vibratoMin, DSPConfig::vibratoMax, 0.1f),
      0.0f));

  // Mix (Dry/Wet)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID(mix, 1),
      "Mix",
      juce::NormalisableRange<float>(DSPConfig::mixMin, DSPConfig::mixMax, 0.1f),
      100.0f // Default: 100% wet
      ));

  // Bypass
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID(bypass, 1),
      "Bypass",
      false));

  // Quality Mode
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID(qualityMode, 1),
      "Quality Mode",
      getQualityModeNames(),
      0 // Default: Live
      ));

  // Harmony Preset
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID(harmonyPreset, 1),
      "Harmony Preset",
      getHarmonyPresetNames(),
      0 // Default: None
      ));

  //==========================================================================
  // HARMONY VOICE PARAMETERS
  //==========================================================================

  // Lambda to add parameters for a single harmony voice
  auto addHarmonyVoiceParams = [&params](
                                   const char *enabledId,
                                   const char *modeId,
                                   const char *intervalDiatonicId,
                                   const char *intervalSemiId,
                                   const char *levelId,
                                   const char *panId,
                                   const char *formantId,
                                   const char *humTimingId,
                                   const char *humPitchId,
                                   const juce::String &voiceName) {
    // Enabled toggle
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(enabledId, 1),
        voiceName + " Enabled",
        false));

    // Harmony mode (Diatonic vs Semitone)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(modeId, 1),
        voiceName + " Mode",
        NovaTuneEnums::getHarmonyModeNames(),
        0 // Default: Diatonic
        ));

    // Diatonic interval (-7 to +7 as indices 0-14)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(intervalDiatonicId, 1),
        voiceName + " Diatonic Interval",
        NovaTuneEnums::getDiatonicIntervalNames(),
        7 // Default: Unison (center)
        ));

    // Semitone interval (-12 to +12)
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID(intervalSemiId, 1),
        voiceName + " Semitone Interval",
        -12, 12,
        0 // Default: No shift
        ));

    // Level (volume in dB)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(levelId, 1),
        voiceName + " Level",
        juce::NormalisableRange<float>(DSPConfig::levelMinDb, DSPConfig::levelMaxDb, 0.1f),
        -12.0f // Default: -12 dB
        ));

    // Pan
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(panId, 1),
        voiceName + " Pan",
        juce::NormalisableRange<float>(DSPConfig::panMin, DSPConfig::panMax, 0.01f),
        0.0f // Default: Center
        ));

    // Formant shift
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(formantId, 1),
        voiceName + " Formant Shift",
        juce::NormalisableRange<float>(DSPConfig::formantMin, DSPConfig::formantMax, 0.1f),
        0.0f));

    // Humanize timing
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(humTimingId, 1),
        voiceName + " Humanize Timing",
        juce::NormalisableRange<float>(DSPConfig::humTimingMinMs, DSPConfig::humTimingMaxMs, 0.1f),
        5.0f));

    // Humanize pitch
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(humPitchId, 1),
        voiceName + " Humanize Pitch",
        juce::NormalisableRange<float>(DSPConfig::humPitchMinCents, DSPConfig::humPitchMaxCents, 0.1f),
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

  return {params.begin(), params.end()};
}

//==============================================================================
// CONSTRUCTOR / DESTRUCTOR
//==============================================================================

NovaTuneAudioProcessor::NovaTuneAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout()) {
  // Plugin is constructed but not yet ready for audio processing
  // Audio setup happens in prepareToPlay()
}

NovaTuneAudioProcessor::~NovaTuneAudioProcessor() {
  // Destructor - clean up any resources
}

//==============================================================================
// PLUGIN INFORMATION
//==============================================================================

const juce::String NovaTuneAudioProcessor::getName() const {
  return "NovaTune";
}

bool NovaTuneAudioProcessor::acceptsMidi() const {
  return false; // We don't use MIDI input (yet - could add for key detection)
}

bool NovaTuneAudioProcessor::producesMidi() const {
  return false;
}

bool NovaTuneAudioProcessor::isMidiEffect() const {
  return false;
}

double NovaTuneAudioProcessor::getTailLengthSeconds() const {
  return 0.0; // No reverb tail
}

//==============================================================================
// PRESET / PROGRAM MANAGEMENT
//==============================================================================

int NovaTuneAudioProcessor::getNumPrograms() {
  return 1; // At least 1 program required by some hosts
}

int NovaTuneAudioProcessor::getCurrentProgram() {
  return 0;
}

void NovaTuneAudioProcessor::setCurrentProgram(int /*index*/) {
  // Not implemented - presets handled through state management
}

const juce::String NovaTuneAudioProcessor::getProgramName(int /*index*/) {
  return "Default";
}

void NovaTuneAudioProcessor::changeProgramName(int /*index*/, const juce::String & /*newName*/) {
  // Not implemented
}

//==============================================================================
// AUDIO PROCESSING SETUP
//==============================================================================

bool NovaTuneAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const {
  // Support mono or stereo, with matching input/output
  const auto &mainInput = layouts.getMainInputChannelSet();
  const auto &mainOutput = layouts.getMainOutputChannelSet();

  // Must have same number of input and output channels
  if (mainInput != mainOutput)
    return false;

  // Support mono or stereo
  if (mainInput.isDisabled())
    return false;

  if (mainInput != juce::AudioChannelSet::mono() &&
      mainInput != juce::AudioChannelSet::stereo())
    return false;

  return true;
}

void NovaTuneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  // Prepare the DSP engine
  tunerEngine.prepare(sampleRate, samplesPerBlock, getTotalNumInputChannels());

  // Report latency to the host
  setLatencySamples(tunerEngine.getLatencySamples());
}

void NovaTuneAudioProcessor::releaseResources() {
  // Reset the DSP engine
  tunerEngine.reset();
}

//==============================================================================
// AUDIO PROCESSING
//==============================================================================

void NovaTuneAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                          juce::MidiBuffer &midiMessages) {
  // Prevent denormals (very small floating point numbers that slow down CPU)
  juce::ScopedNoDenormals noDenormals;

  // Get channel counts
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // Clear any extra output channels
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
    buffer.clear(i, 0, buffer.getNumSamples());
  }

  // Check bypass
  bool isBypassed = apvts.getRawParameterValue(ParamIDs::bypass)->load() > 0.5f;

  if (isBypassed) {
    // Bypass: pass audio through unchanged
    return;
  }

  // Process through the tuner engine
  tunerEngine.process(buffer, midiMessages, apvts);
}

//==============================================================================
// EDITOR
//==============================================================================

bool NovaTuneAudioProcessor::hasEditor() const {
  return true;
}

juce::AudioProcessorEditor *NovaTuneAudioProcessor::createEditor() {
  return new NovaTuneAudioProcessorEditor(*this);
}

//==============================================================================
// STATE SERIALIZATION
//==============================================================================

void NovaTuneAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
  // Serialize the parameter state to XML, then to binary
  if (auto xml = apvts.copyState().createXml()) {
    copyXmlToBinary(*xml, destData);
  }
}

void NovaTuneAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
  // Deserialize from binary to XML, then to parameter state
  if (auto xmlState = getXmlFromBinary(data, sizeInBytes)) {
    if (xmlState->hasTagName(apvts.state.getType())) {
      apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
  }
}

//==============================================================================
// PLUGIN CREATION
//==============================================================================

/**
 * This creates new instances of the plugin.
 * Required by the JUCE plugin framework.
 */
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new NovaTuneAudioProcessor();
}
