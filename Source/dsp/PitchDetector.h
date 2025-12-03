#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include "../DSPConfig.h"
#include "../Utilities.h"

/**
 * PitchDetector.h
 *
 * Implements the YIN algorithm for fundamental frequency (F0) detection.
 *
 * WHAT IS PITCH DETECTION?
 * When someone sings an "A" note, their vocal cords vibrate 440 times per second.
 * This creates a complex waveform with a fundamental frequency (F0) of 440 Hz
 * plus harmonics (880 Hz, 1320 Hz, etc.). Our job is to find that F0.
 *
 * WHY YIN?
 * The YIN algorithm (de Cheveigné & Kawahara, 2002) is one of the best
 * time-domain pitch detectors. It's:
 * - Accurate: Better than simple autocorrelation
 * - Fast: Real-time capable
 * - Robust: Handles noise and octave errors well
 *
 * HOW YIN WORKS (simplified):
 * 1. Take a chunk of audio (called a "frame")
 * 2. For each possible period τ (tau), compute how different the signal is
 *    from a copy shifted by τ samples
 * 3. The period with the smallest difference is likely the pitch period
 * 4. Convert period (samples) to frequency (Hz): freq = sampleRate / period
 *
 * ANALOGY: Imagine holding two copies of a wavy line. Slide one copy
 * until the waves line up best - the distance you slid is the period.
 */
class PitchDetector {
public:
  PitchDetector();
  ~PitchDetector() = default;

  /**
   * Prepare the detector for processing.
   * Call this when sample rate or buffer size changes.
   *
   * @param sampleRate Audio sample rate (e.g., 44100, 48000)
   * @param maxBlockSize Maximum number of samples per process block
   */
  void prepare(double sampleRate, int maxBlockSize);

  /**
   * Reset internal state.
   * Call this when audio playback stops/starts to clear history.
   */
  void reset();

  /**
   * Process a buffer of audio to detect pitch.
   *
   * @param buffer Input audio buffer (stereo is summed to mono internally)
   */
  void process(const juce::AudioBuffer<float> &buffer);

  /**
   * Set the voice/input type to constrain pitch search range.
   * This helps prevent octave errors.
   *
   * @param type The input type (Soprano, AltoTenor, LowMale, Instrument)
   */
  void setInputType(NovaTuneEnums::InputType type);

  //==========================================================================
  // GETTERS - Call these after process() to get detection results
  //==========================================================================

  /** Get the detected frequency in Hz (0 if unvoiced/silent) */
  float getFrequencyHz() const noexcept { return detectedFrequencyHz; }

  /** Get the detected MIDI note (fractional, e.g., 69.5) */
  float getMidiNote() const noexcept { return detectedMidiNote; }

  /** Is the signal currently voiced (singing) vs unvoiced (silence/noise)? */
  bool isVoiced() const noexcept { return voiced; }

  /** Get the confidence of the pitch estimate (0.0 to 1.0) */
  float getConfidence() const noexcept { return confidence; }

  /** Get the detected period in samples */
  float getPeriodSamples() const noexcept { return detectedPeriod; }

private:
  //==========================================================================
  // INTERNAL STATE
  //==========================================================================

  double sampleRate = 44100.0;
  int frameSize = DSPConfig::pitchDetectionFrameSize;
  int hopSize = DSPConfig::pitchDetectionHopSize;

  // Input type affects the pitch search range
  NovaTuneEnums::InputType inputType = NovaTuneEnums::InputType::AltoTenor;
  float minFreqHz = DSPConfig::altoTenorMinHz;
  float maxFreqHz = DSPConfig::altoTenorMaxHz;

  // Detection results
  float detectedFrequencyHz = 0.0f;
  float detectedMidiNote = 0.0f;
  float detectedPeriod = 0.0f;
  bool voiced = false;
  float confidence = 0.0f;

  // Internal buffers (pre-allocated to avoid runtime allocation)
  juce::AudioBuffer<float> monoBuffer;    // Summed mono input
  juce::AudioBuffer<float> analysisFrame; // Current analysis frame
  std::vector<float> yinBuffer;           // YIN difference function
  std::vector<float> inputRingBuffer;     // Ring buffer for accumulating input
  int ringBufferWritePos = 0;
  int samplesUntilNextAnalysis = 0;

  //==========================================================================
  // YIN ALGORITHM STEPS
  //==========================================================================

  /**
   * Step 1: Compute the difference function d(τ).
   *
   * For each lag τ, compute:
   * d(τ) = Σ (x[j] - x[j+τ])²
   *
   * This measures how different the signal is from a shifted version.
   * When τ equals the pitch period, d(τ) will be small (signals align).
   */
  void computeDifferenceFunction(const float *input, int numSamples);

  /**
   * Step 2: Compute cumulative mean normalized difference function d'(τ).
   *
   * d'(τ) = d(τ) / [(1/τ) * Σ(d(j), j=1 to τ)]
   *
   * This normalization reduces octave errors by making the function
   * less sensitive to the absolute energy of the signal.
   */
  void computeCumulativeMeanNormalizedDifference();

  /**
   * Step 3: Absolute threshold.
   *
   * Find the smallest τ where d'(τ) < threshold.
   * This gives us the pitch period estimate.
   *
   * @return Estimated period in samples, or 0 if unvoiced
   */
  float absoluteThreshold();

  /**
   * Step 4: Parabolic interpolation for sub-sample accuracy.
   *
   * The best τ is usually between two integer sample positions.
   * Fitting a parabola to the three points around the minimum
   * gives us sub-sample precision.
   *
   * @param tauEstimate Integer tau estimate
   * @return Refined tau with sub-sample precision
   */
  float parabolicInterpolation(int tauEstimate);

  /**
   * Convert period in samples to frequency in Hz.
   */
  float periodToFrequency(float periodSamples) const;

  /**
   * Update the min/max frequency search range based on input type.
   */
  void updateFrequencyRange();
};
