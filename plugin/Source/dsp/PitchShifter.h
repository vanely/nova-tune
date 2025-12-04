#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include "../DSPConfig.h"
#include "../Utilities.h"

/**
 * PitchShifter.h
 *
 * Real-time pitch shifting using WSOLA (Waveform Similarity Overlap-Add).
 *
 * WHAT IS PITCH SHIFTING?
 * Changing the pitch (frequency) of audio without changing the speed.
 * - Pitch up: Makes voice higher (like speeding up a record, but without the speed change)
 * - Pitch down: Makes voice lower
 *
 * WHY IS THIS HARD?
 * If you just speed up audio, pitch goes up but it also plays faster.
 * If you just change playback rate and resample, you get the right speed
 * but the pitch changes. We need to change pitch WITHOUT changing speed.
 *
 * HOW WSOLA WORKS:
 *
 * 1. ANALYSIS: Cut the input into overlapping chunks called "grains"
 *    Think of it like cutting a long ribbon into overlapping pieces
 *
 * 2. TIME SCALING: Adjust how far apart we place the grains
 *    - To pitch UP: Place grains closer together (more overlap)
 *    - To pitch DOWN: Place grains further apart (less overlap)
 *    This changes the playback rate without losing any audio
 *
 * 3. WAVEFORM SIMILARITY: When placing each grain, we search for the
 *    best position where it aligns with the previous grain.
 *    This prevents phase discontinuities (which sound like buzzing)
 *
 * 4. OVERLAP-ADD: Crossfade between grains using a window function
 *    This creates smooth transitions and prevents clicks
 *
 * 5. RESAMPLING: After time-scaling, resample to get original length
 *    The time-scale + resample combo = pitch shift with original duration
 *
 * ANALOGY: Imagine you have a slinky (spring). To make it "higher pitch":
 * - Compress the slinky (time-scale shorter)
 * - Stretch each coil back out (resample to original length)
 * Now you have the same length but more coils = higher frequency!
 */
class PitchShifter {
public:
  PitchShifter();
  ~PitchShifter() = default;

  /**
   * Prepare the pitch shifter for processing.
   *
   * @param sampleRate Audio sample rate
   * @param maxBlockSize Maximum samples per processing block
   */
  void prepare(double sampleRate, int maxBlockSize);

  /**
   * Reset all internal state.
   */
  void reset();

  /**
   * Set the pitch shift ratio.
   *
   * @param ratio Pitch ratio (1.0 = no change, 2.0 = octave up, 0.5 = octave down)
   */
  void setPitchRatio(float ratio);

  /**
   * Set pitch shift in semitones.
   *
   * @param semitones Semitone shift (-12 to +12 is one octave)
   */
  void setPitchSemitones(float semitones);

  /**
   * Get the current pitch ratio.
   */
  float getPitchRatio() const noexcept { return targetPitchRatio; }

  /**
   * Get the latency introduced by the pitch shifter in samples.
   */
  int getLatencySamples() const noexcept { return latencySamples; }

  /**
   * Process a mono audio buffer.
   *
   * @param inputOutput Buffer to process (modified in place)
   * @param numSamples Number of samples to process
   */
  void process(float *inputOutput, int numSamples);

  /**
   * Process a mono audio buffer with separate input and output.
   *
   * @param input Input buffer (read only)
   * @param output Output buffer (written to)
   * @param numSamples Number of samples to process
   */
  void process(const float *input, float *output, int numSamples);

private:
  //==========================================================================
  // CONFIGURATION
  //==========================================================================

  double sampleRate = 44100.0;
  int maxBlockSize = 512;

  // WSOLA window/grain size in samples (calculated from windowMs)
  int windowSize = 1024;

  // Analysis hop size (how far we move between grains)
  int analysisHopSize = 256;

  // Current pitch ratio (1.0 = no shift)
  float targetPitchRatio = 1.0f;
  float currentPitchRatio = 1.0f;    // Smoothed version
  float pitchRatioSmoothing = 0.01f; // Smoothing coefficient

  // Latency introduced by the algorithm
  int latencySamples = 0;

  //==========================================================================
  // INTERNAL BUFFERS
  //==========================================================================

  // Input ring buffer - stores incoming audio
  std::vector<float> inputBuffer;
  int inputWritePos = 0;
  int inputSamplesAvailable = 0;

  // Output ring buffer - stores processed audio ready for output
  std::vector<float> outputBuffer;
  int outputReadPos = 0;
  int outputWritePos = 0;

  // Grain buffer - current grain being processed
  std::vector<float> grainBuffer;

  // Window function (Hann window)
  std::vector<float> windowFunction;

  // Overlap-add buffer for smooth transitions
  std::vector<float> overlapBuffer;

  // Position tracking
  float inputPhase = 0.0f;     // Where we are in the input (fractional)
  float outputPhase = 0.0f;    // Where we are in the output
  int lastInputGrainStart = 0; // Start of last extracted grain

  //==========================================================================
  // INTERNAL METHODS
  //==========================================================================

  /**
   * Calculate synthesis hop size based on pitch ratio.
   *
   * synthesis_hop = analysis_hop / pitch_ratio
   *
   * To pitch UP (ratio > 1): synthesis_hop < analysis_hop (grains closer)
   * To pitch DOWN (ratio < 1): synthesis_hop > analysis_hop (grains further)
   */
  float getSynthesisHopSize() const;

  /**
   * Extract a grain from the input buffer.
   *
   * @param startPos Starting position in input buffer
   */
  void extractGrain(int startPos);

  /**
   * Find the best position to place a grain using waveform similarity.
   * This reduces phase discontinuities by finding where the new grain
   * best aligns with what's already in the output.
   *
   * @param nominalPos The "ideal" position based on hop size
   * @param searchRange How many samples to search around nominalPos
   * @return Best position for the grain
   */
  int findBestGrainPosition(int nominalPos, int searchRange);

  /**
   * Add a grain to the output buffer using overlap-add.
   *
   * @param startPos Position to add the grain
   */
  void addGrainToOutput(int startPos);

  /**
   * Process internal buffers - called when we have enough input.
   */
  void processGrains();

  /**
   * Cubic interpolation for reading from input buffer at fractional positions.
   */
  float readInputInterpolated(float position) const;
};
