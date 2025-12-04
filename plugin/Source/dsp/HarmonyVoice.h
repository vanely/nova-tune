#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PitchShifter.h"
#include "FormantProcessor.h"
#include "PitchDetector.h"
#include "PitchMapper.h"
#include "../ParameterIDs.h"
#include "../DSPConfig.h"
#include "../Utilities.h"

/**
 * HarmonyVoice.h
 *
 * Represents a single harmony voice that generates a pitched copy
 * of the lead vocal at a specified interval.
 *
 * WHAT IS A HARMONY VOICE?
 *
 * In vocal music, harmonies are additional vocal lines that complement
 * the main melody. Common harmony patterns include:
 *
 * - Third above: The harmony sings 3 scale notes above the melody
 * - Third below: 3 scale notes below
 * - Fifth above: 5 scale notes above (very common in gospel/country)
 * - Octave: Same note but higher or lower octave
 *
 * NovaTune generates these automatically by:
 * 1. Taking the corrected lead vocal as a source
 * 2. Pitch shifting it to the harmony interval
 * 3. Applying formant correction (so it doesn't sound chipmunk-y)
 * 4. Adding humanization (so it sounds like a real singer)
 *
 * HUMANIZATION FOR HARMONIES:
 *
 * Real backing vocalists don't sing in perfect sync with the lead.
 * There are subtle variations in:
 * - Timing: Slightly behind or ahead (usually 5-30ms behind)
 * - Pitch: Small random variations (±5-15 cents)
 * - Vibrato: May have different vibrato patterns
 *
 * We simulate these to make the harmonies sound more natural.
 */
class HarmonyVoice {
public:
  HarmonyVoice();
  ~HarmonyVoice() = default;

  /**
   * Prepare the harmony voice for processing.
   *
   * @param sampleRate Audio sample rate
   * @param maxBlockSize Maximum samples per block
   * @param numChannels Number of audio channels
   */
  void prepare(double sampleRate, int maxBlockSize, int numChannels);

  /**
   * Reset internal state.
   */
  void reset();

  /**
   * Update voice parameters from the plugin state.
   *
   * @param voiceIndex Which voice this is (0=A, 1=B, 2=C)
   * @param apvts The parameter state
   */
  void updateFromParameters(int voiceIndex, juce::AudioProcessorValueTreeState &apvts);

  /**
   * Process audio and add this voice's output to the harmony buffer.
   *
   * @param harmonyBuffer Output buffer to ADD this voice's signal to
   * @param leadBuffer The corrected lead vocal to base harmony on
   * @param detector Pitch detector with current detection
   * @param mapper Pitch mapper with target notes
   */
  void process(juce::AudioBuffer<float> &harmonyBuffer,
               const juce::AudioBuffer<float> &leadBuffer,
               const PitchDetector &detector,
               const PitchMapper &mapper);

  /**
   * Check if this voice is enabled.
   */
  bool isEnabled() const noexcept { return enabled; }

  /**
   * Get the current harmony pitch in MIDI note number.
   */
  float getCurrentHarmonyMidi() const noexcept { return currentHarmonyMidi; }

  /**
   * Get the latency introduced by this voice in samples.
   */
  int getLatencySamples() const;

private:
  //==========================================================================
  // CONFIGURATION
  //==========================================================================

  double sampleRate = 44100.0;
  int maxBlockSize = 512;
  int numChannels = 2;

  // Voice parameters
  bool enabled = false;
  NovaTuneEnums::HarmonyMode mode = NovaTuneEnums::HarmonyMode::Diatonic;
  int diatonicIntervalIndex = 7; // 7 = unison (center of -7 to +7 range)
  int semitoneOffset = 0;        // For semitone mode: -12 to +12

  float levelDb = -12.0f;          // Output level in dB
  float pan = 0.0f;                // Stereo pan (-1 to +1)
  float formantShift = 0.0f;       // Formant shift in semitones
  float humanizeTimingMs = 5.0f;   // Random timing variation
  float humanizePitchCents = 3.0f; // Random pitch variation

  //==========================================================================
  // DSP COMPONENTS
  //==========================================================================

  // Pitch shifter for each channel
  std::vector<PitchShifter> pitchShifters;

  // Formant processor
  FormantProcessor formantProcessor;

  // Delay line for timing humanization (per channel)
  std::vector<std::vector<float>> delayLines;
  std::vector<int> delayWritePositions;
  int maxDelaySamples = 0;
  float currentDelaySamples = 0.0f;

  //==========================================================================
  // INTERNAL STATE
  //==========================================================================

  // Current harmony target (for UI/debugging)
  float currentHarmonyMidi = 0.0f;

  // Pitch ratio smoothing
  float targetPitchRatio = 1.0f;
  float currentPitchRatio = 1.0f;
  float pitchRatioSmoothing = 0.01f;

  // Gain smoothing
  float targetGain = 0.0f;
  float currentGain = 0.0f;
  float gainSmoothing = 0.01f;

  // Pan gains
  float panGainL = 1.0f;
  float panGainR = 1.0f;

  // Humanization state
  float pitchHumanizeOffset = 0.0f;
  float timingHumanizeTarget = 0.0f;
  juce::Random randomGenerator;
  int samplesSinceHumanizeUpdate = 0;
  int humanizeUpdateIntervalSamples = 4410; // ~100ms at 44.1kHz

  // Internal buffers
  juce::AudioBuffer<float> voiceBuffer;

  //==========================================================================
  // HELPER METHODS
  //==========================================================================

  /**
   * Calculate the pitch ratio for this harmony voice.
   */
  float calculateHarmonyPitchRatio(const PitchDetector &detector,
                                   const PitchMapper &mapper);

  /**
   * Apply timing humanization (delay) to the buffer.
   */
  void applyTimingHumanization(juce::AudioBuffer<float> &buffer);

  /**
   * Update humanization parameters (called periodically).
   */
  void updateHumanization();

  /**
   * Apply gain and panning to the voice buffer.
   */
  void applyGainAndPan(juce::AudioBuffer<float> &buffer);
};
