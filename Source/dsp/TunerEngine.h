#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PitchDetector.h"
#include "PitchMapper.h"
#include "LeadCorrection.h"
#include "HarmonyVoice.h"
#include "../DSPConfig.h"
#include "../ParameterIDs.h"

/**
 * TunerEngine.h
 *
 * The main DSP orchestrator for NovaTune.
 *
 * This class coordinates all the DSP components:
 * - Pitch detection (what note is the singer singing?)
 * - Pitch mapping (what note should they be singing?)
 * - Lead correction (move them to the right note)
 * - Harmony generation (create additional voices)
 *
 * SIGNAL FLOW:
 *
 *                         Input Audio
 *                              │
 *                              ▼
 *                    ┌─────────────────┐
 *                    │ Pitch Detector  │
 *                    │   (Analysis)    │
 *                    └────────┬────────┘
 *                             │ Detected F0
 *                             ▼
 *                    ┌─────────────────┐
 *                    │  Pitch Mapper   │
 *                    │  (Key/Scale)    │
 *                    └────────┬────────┘
 *                             │ Target Notes
 *              ┌──────────────┼──────────────┐
 *              │              │              │
 *              ▼              ▼              ▼
 *       ┌───────────┐  ┌───────────┐  ┌───────────┐
 *       │   Lead    │  │ Harmony A │  │ Harmony B │ ...
 *       │Correction │  │   Voice   │  │   Voice   │
 *       └─────┬─────┘  └─────┬─────┘  └─────┬─────┘
 *             │              │              │
 *             └──────────────┼──────────────┘
 *                            │
 *                            ▼
 *                    ┌─────────────────┐
 *                    │    Mixer        │
 *                    │ (Lead+Harmony)  │
 *                    └────────┬────────┘
 *                             │
 *                             ▼
 *                        Output Audio
 *
 * THREADING MODEL:
 *
 * This class is designed to be called from the audio thread.
 * All processing must be:
 * - Deterministic (no locks, no memory allocation)
 * - Fast (avoid branches, use SIMD where possible)
 * - Real-time safe
 *
 * Parameter updates come from the message thread via atomic values
 * in the AudioProcessorValueTreeState.
 */
class TunerEngine {
public:
  TunerEngine();
  ~TunerEngine() = default;

  /**
   * Prepare the engine for processing.
   * Called when the audio device starts or settings change.
   *
   * @param sampleRate Audio sample rate (e.g., 44100, 48000)
   * @param samplesPerBlock Maximum samples per process call
   * @param numChannels Number of audio channels (1=mono, 2=stereo)
   */
  void prepare(double sampleRate, int samplesPerBlock, int numChannels);

  /**
   * Reset all internal state.
   * Called when playback stops or seeks.
   */
  void reset();

  /**
   * Process a block of audio.
   *
   * @param buffer Audio buffer to process (modified in place)
   * @param midi MIDI buffer (unused in current implementation)
   * @param apvts Parameter state for reading current values
   */
  void process(juce::AudioBuffer<float> &buffer,
               juce::MidiBuffer &midi,
               juce::AudioProcessorValueTreeState &apvts);

  /**
   * Get the total latency introduced by the engine in samples.
   */
  int getLatencySamples() const;

  //==========================================================================
  // ACCESSORS FOR UI / METERING
  //==========================================================================

  /** Get the pitch detector for UI visualization */
  const PitchDetector &getPitchDetector() const { return pitchDetector; }

  /** Get the pitch mapper for UI visualization */
  const PitchMapper &getPitchMapper() const { return pitchMapper; }

  /** Get the lead correction for UI visualization */
  const LeadCorrection &getLeadCorrection() const { return leadCorrection; }

  /** Get a harmony voice for UI visualization */
  const HarmonyVoice &getHarmonyVoice(int index) const {
    return harmonyVoices[static_cast<size_t>(std::clamp(index, 0, DSPConfig::maxHarmonyVoices - 1))];
  }

private:
  //==========================================================================
  // CONFIGURATION
  //==========================================================================

  double sampleRate = 44100.0;
  int samplesPerBlock = 512;
  int numChannels = 2;

  //==========================================================================
  // DSP COMPONENTS
  //==========================================================================

  PitchDetector pitchDetector;
  PitchMapper pitchMapper;
  LeadCorrection leadCorrection;
  std::array<HarmonyVoice, DSPConfig::maxHarmonyVoices> harmonyVoices;

  //==========================================================================
  // INTERNAL BUFFERS
  //==========================================================================

  /** Buffer for corrected lead vocal */
  juce::AudioBuffer<float> leadBuffer;

  /** Buffer for summed harmony voices */
  juce::AudioBuffer<float> harmonyBuffer;

  /** Dry signal for mix control */
  juce::AudioBuffer<float> dryBuffer;

  //==========================================================================
  // HELPER METHODS
  //==========================================================================

  /**
   * Update all DSP components from the parameter state.
   * Called at the start of each process block.
   */
  void updateFromParameters(juce::AudioProcessorValueTreeState &apvts);
};
