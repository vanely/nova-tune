#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include "../DSPConfig.h"

/**
 * FormantProcessor.h
 *
 * Handles formant preservation and shifting for vocal processing.
 *
 * WHAT ARE FORMANTS?
 *
 * When you speak or sing, your vocal cords create a buzzing sound
 * (like a reed in a clarinet). This "source" sound then resonates
 * through your throat, mouth, and nasal cavity - your "vocal tract".
 *
 * The vocal tract acts like a filter, boosting certain frequencies
 * and cutting others. These boosted frequency regions are called
 * FORMANTS. The first two formants (F1 and F2) are most important:
 *
 * - F1: Related to jaw openness (200-1000 Hz)
 * - F2: Related to tongue position (700-2500 Hz)
 *
 * Different vowels have different formant patterns:
 * - "EE" (beat): F1 low (~300 Hz), F2 high (~2500 Hz)
 * - "AH" (father): F1 high (~700 Hz), F2 mid (~1100 Hz)
 * - "OO" (boot): F1 low (~300 Hz), F2 low (~870 Hz)
 *
 * WHY FORMANT PRESERVATION MATTERS:
 *
 * When we pitch shift without formant preservation:
 * - Pitch up → Formants shift up → "Chipmunk" sound
 * - Pitch down → Formants shift down → "Giant/demon" sound
 *
 * With formant preservation:
 * - We shift the pitch but keep formants where they were
 * - Voice sounds like the same person, just singing a different note
 *
 * HOW WE DO IT:
 *
 * 1. ANALYSIS: Estimate the spectral envelope (which includes formants)
 *    - Use LPC (Linear Predictive Coding) or Cepstral analysis
 *
 * 2. SEPARATION: Separate the excitation (pitch) from the envelope (formants)
 *
 * 3. MODIFICATION: Shift the pitch, optionally shift formants independently
 *
 * 4. SYNTHESIS: Recombine the modified excitation with the envelope
 *
 * For MVP, we'll use a simplified approach: a dynamic EQ that
 * counteracts the formant shift caused by pitch shifting.
 */
class FormantProcessor {
public:
  FormantProcessor();
  ~FormantProcessor() = default;

  /**
   * Prepare for processing.
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
   * Set the amount of formant shift in semitones.
   *
   * @param semitones Formant shift amount (-6 to +6)
   *                  Positive = higher formants (more feminine)
   *                  Negative = lower formants (more masculine)
   */
  void setFormantShift(float semitones);

  /**
   * Set the pitch shift ratio to compensate for.
   * This is used for formant PRESERVATION - we shift formants
   * in the opposite direction of the pitch shift to maintain
   * the original formant positions.
   *
   * @param pitchRatio The pitch shift ratio (1.0 = no shift)
   */
  void setPitchCompensation(float pitchRatio);

  /**
   * Process audio buffer.
   *
   * @param buffer Audio buffer to process (modified in place)
   */
  void process(juce::AudioBuffer<float> &buffer);

  /**
   * Get the latency introduced by formant processing in samples.
   */
  int getLatencySamples() const noexcept { return latencySamples; }

private:
  //==========================================================================
  // CONFIGURATION
  //==========================================================================

  double sampleRate = 44100.0;
  int maxBlockSize = 512;
  int numChannels = 2;
  int latencySamples = 0;

  // Formant shift amount in semitones (-6 to +6)
  float formantShiftSemitones = 0.0f;

  // Pitch compensation ratio (for formant preservation)
  float pitchCompensationRatio = 1.0f;

  // Combined shift ratio for the filters
  float currentShiftRatio = 1.0f;
  float targetShiftRatio = 1.0f;
  float shiftSmoothingCoeff = 0.01f;

  //==========================================================================
  // FILTER BANK FOR FORMANT SHIFTING
  // We use a bank of bandpass filters to create a "vocoder-like" effect
  // that shifts formant frequencies
  //==========================================================================

  static constexpr int numBands = 8;

  // Analysis filter bank (fixed frequencies)
  std::vector<juce::dsp::IIR::Filter<float>> analysisFiltersL;
  std::vector<juce::dsp::IIR::Filter<float>> analysisFiltersR;

  // Synthesis filter bank (shifted frequencies)
  std::vector<juce::dsp::IIR::Filter<float>> synthesisFiltersL;
  std::vector<juce::dsp::IIR::Filter<float>> synthesisFiltersR;

  // Filter center frequencies (in Hz)
  std::array<float, numBands> bandCenterFreqs;

  // Band envelope followers
  std::array<float, numBands> bandEnvelopes;
  float envelopeSmoothingCoeff = 0.01f;

  // Temporary buffers
  juce::AudioBuffer<float> analysisBuffer;
  juce::AudioBuffer<float> synthesisBuffer;

  //==========================================================================
  // HELPER METHODS
  //==========================================================================

  /**
   * Update filter coefficients based on current shift ratio.
   */
  void updateFilters();

  /**
   * Calculate the effective shift ratio from formant shift and pitch compensation.
   */
  float calculateEffectiveShiftRatio() const;
};
