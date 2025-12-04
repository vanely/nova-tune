#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PitchDetector.h"
#include "PitchMapper.h"
#include "PitchShifter.h"
#include "../DSPConfig.h"
#include "../Utilities.h"

/**
 * LeadCorrection.h
 *
 * The main pitch correction engine for the lead vocal.
 *
 * HOW PITCH CORRECTION WORKS:
 *
 * 1. DETECTION: PitchDetector tells us what note the singer is singing
 * 2. MAPPING: PitchMapper tells us what note they SHOULD be singing
 * 3. CORRECTION: LeadCorrection uses PitchShifter to move them to the target
 *
 * THE RETUNE SPEED PARAMETER:
 * This is the most important creative control. It determines HOW FAST
 * we pull the singer to the target note:
 *
 * - Retune Speed 0 (Slow): Gentle correction over ~400ms
 *   → Natural sound, preserves expression
 *   → Good for jazz, classical, acoustic music
 *
 * - Retune Speed 50 (Medium): Correction over ~50ms
 *   → Modern pop sound
 *   → Tight but still human
 *
 * - Retune Speed 100 (Instant): Immediate snap to pitch
 *   → The "T-Pain effect" / "Cher effect"
 *   → Robotic, obviously processed
 *   → Trap, EDM, hip-hop vocals
 *
 * THE HUMANIZE PARAMETER:
 * Adds variation to prevent the "too perfect" robotic sound:
 * - Preserves natural pitch drift on sustained notes
 * - Allows for expressive bends between notes
 * - Makes the correction less obvious
 */
class LeadCorrection {
public:
  LeadCorrection();
  ~LeadCorrection() = default;

  /**
   * Prepare the correction engine.
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
   * Update parameters from the plugin state.
   * Call this at the start of each process block.
   */
  void updateFromParameters(juce::AudioProcessorValueTreeState &apvts);

  /**
   * Set retune speed (0-100).
   * 0 = very slow/natural, 100 = instant/robotic
   */
  void setRetuneSpeed(float speed);

  /**
   * Set humanize amount (0-100).
   * Higher values preserve more natural variation.
   */
  void setHumanize(float amount);

  /**
   * Set vibrato amount (0-100).
   * Controls how much natural vibrato to preserve.
   */
  void setVibrato(float amount);

  /**
   * Set wet/dry mix (0.0 to 1.0).
   */
  void setMix(float wetAmount);

  /**
   * Process audio buffer.
   *
   * @param buffer Audio buffer to process (modified in place)
   * @param detector Pitch detector with current detection results
   * @param mapper Pitch mapper with target note information
   */
  void process(juce::AudioBuffer<float> &buffer,
               const PitchDetector &detector,
               const PitchMapper &mapper);

  /**
   * Get the current pitch correction amount in semitones.
   * Useful for UI visualization.
   */
  float getCurrentCorrectionSemitones() const noexcept { return currentCorrectionAmount; }

  /**
   * Get the latency introduced by correction in samples.
   */
  int getLatencySamples() const noexcept;

private:
  //==========================================================================
  // CONFIGURATION
  //==========================================================================

  double sampleRate = 44100.0;
  int blockSize = 512;
  int numChannels = 2;

  // User parameters
  float retuneSpeed = 50.0f;    // 0-100
  float humanizeAmount = 25.0f; // 0-100
  float vibratoAmount = 0.0f;   // 0-100
  float mix = 1.0f;             // 0.0-1.0 (wet amount)

  //==========================================================================
  // INTERNAL STATE
  //==========================================================================

  // Pitch shifter for each channel
  std::vector<PitchShifter> pitchShifters;

  // Smoothed pitch ratio (to avoid sudden jumps)
  float targetPitchRatio = 1.0f;
  float currentPitchRatio = 1.0f;
  float pitchRatioSmoothingCoeff = 0.1f;

  // Current correction amount (for UI)
  float currentCorrectionAmount = 0.0f;

  // Dry signal buffer for mix
  juce::AudioBuffer<float> dryBuffer;

  // Humanization state
  float humanizeOffset = 0.0f; // Current humanize pitch offset
  float humanizePhase = 0.0f;  // LFO phase for subtle drift
  juce::Random randomGenerator;

  // Vibrato detection and preservation
  float lastDetectedPitch = 0.0f;
  float vibratoDepth = 0.0f; // Estimated vibrato depth

  //==========================================================================
  // HELPER METHODS
  //==========================================================================

  /**
   * Convert retune speed (0-100) to smoothing time constant in ms.
   *
   * 0 → ~400ms (very slow)
   * 50 → ~25ms (medium)
   * 100 → ~0.5ms (instant)
   */
  float retuneSpeedToTimeConstantMs(float speed) const;

  /**
   * Calculate the target pitch ratio based on detected and target notes.
   */
  float calculateTargetPitchRatio(const PitchDetector &detector,
                                  const PitchMapper &mapper);

  /**
   * Apply humanization to the target pitch ratio.
   */
  float applyHumanization(float targetRatio, float detectedMidi, float targetMidi);
};
