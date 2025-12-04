#include "PitchShifter.h"
#include <cmath>
#include <algorithm>
#include <numeric>

/**
 * PitchShifter.cpp
 *
 * Implementation of WSOLA (Waveform Similarity Overlap-Add) pitch shifting.
 *
 * This is a time-domain approach that works well for monophonic signals
 * like vocals. It provides low latency and good quality for real-time use.
 */

PitchShifter::PitchShifter() {
  // Default buffer sizes - will be properly sized in prepare()
  inputBuffer.resize(DSPConfig::ringBufferSize, 0.0f);
  outputBuffer.resize(DSPConfig::ringBufferSize, 0.0f);
}

void PitchShifter::prepare(double sr, int maxBlock) {
  sampleRate = sr;
  maxBlockSize = maxBlock;

  // Calculate window size from milliseconds
  // 25ms is a good compromise between quality and latency for vocals
  windowSize = static_cast<int>(DSPConfig::wsolaWindowMs * sampleRate / 1000.0);
  // Round to nearest power of 2 for efficiency (optional but helps)
  windowSize = std::max(256, std::min(windowSize, 2048));

  // Analysis hop size: typically 1/4 of window size for 75% overlap
  analysisHopSize = windowSize / 4;

  // Latency is approximately the window size
  // We need to buffer at least one window before we can output
  latencySamples = windowSize;

  // Size input buffer to hold enough for analysis
  // Need at least 2x window size + search range
  int inputBufferSize = windowSize * 4 + maxBlockSize;
  inputBufferSize = juce::nextPowerOfTwo(inputBufferSize);
  inputBuffer.resize(static_cast<size_t>(inputBufferSize), 0.0f);

  // Output buffer sized similarly
  int outputBufferSize = windowSize * 4 + maxBlockSize;
  outputBufferSize = juce::nextPowerOfTwo(outputBufferSize);
  outputBuffer.resize(static_cast<size_t>(outputBufferSize), 0.0f);

  // Grain buffer holds one grain
  grainBuffer.resize(static_cast<size_t>(windowSize), 0.0f);

  // Overlap buffer for overlap-add (needs to be as wide as window)
  overlapBuffer.resize(static_cast<size_t>(windowSize), 0.0f);

  // Pre-compute Hann window
  NovaTuneUtils::fillHannWindow(windowFunction, windowSize);

  // Calculate smoothing coefficient for pitch ratio changes
  // We want smooth transitions to avoid clicks
  pitchRatioSmoothing = NovaTuneUtils::calculateSmoothingCoeff(
      DSPConfig::pitchSmoothingMs, sampleRate);

  reset();
}

void PitchShifter::reset() {
  std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
  std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
  std::fill(grainBuffer.begin(), grainBuffer.end(), 0.0f);
  std::fill(overlapBuffer.begin(), overlapBuffer.end(), 0.0f);

  inputWritePos = 0;
  inputSamplesAvailable = 0;
  outputReadPos = 0;
  outputWritePos = windowSize; // Start with latency offset
  inputPhase = 0.0f;
  outputPhase = 0.0f;
  lastInputGrainStart = 0;

  currentPitchRatio = targetPitchRatio;
}

void PitchShifter::setPitchRatio(float ratio) {
  // Clamp to safe range
  targetPitchRatio = std::clamp(ratio,
                                DSPConfig::minPitchShiftRatio,
                                DSPConfig::maxPitchShiftRatio);
}

void PitchShifter::setPitchSemitones(float semitones) {
  setPitchRatio(NovaTuneUtils::semitonesToRatio(semitones));
}

float PitchShifter::getSynthesisHopSize() const {
  /**
   * The synthesis hop determines where we place grains in the output.
   *
   * synthesis_hop = analysis_hop / pitch_ratio
   *
   * Why? Think about it:
   * - To pitch UP (ratio > 1): We need MORE cycles in the same time
   *   → Place grains CLOSER together (smaller synthesis hop)
   *   → More overlap means more repetition of waveform cycles
   *
   * - To pitch DOWN (ratio < 1): We need FEWER cycles
   *   → Place grains FURTHER apart (larger synthesis hop)
   *   → Less overlap means stretching out the waveform cycles
   */

  if (currentPitchRatio <= 0.0f) {
    return static_cast<float>(analysisHopSize);
  }

  return static_cast<float>(analysisHopSize) / currentPitchRatio;
}

void PitchShifter::process(float *inputOutput, int numSamples) {
  process(inputOutput, inputOutput, numSamples);
}

void PitchShifter::process(const float *input, float *output, int numSamples) {
  const int inputBufferSize = static_cast<int>(inputBuffer.size());
  const int outputBufferSize = static_cast<int>(outputBuffer.size());

  // Process sample by sample (or in small chunks)
  for (int i = 0; i < numSamples; ++i) {
    // Smooth pitch ratio changes to avoid clicks
    currentPitchRatio += pitchRatioSmoothing * (targetPitchRatio - currentPitchRatio);

    //======================================================================
    // INPUT: Write incoming sample to input ring buffer
    //======================================================================

    inputBuffer[static_cast<size_t>(inputWritePos)] = input[i];
    inputWritePos = (inputWritePos + 1) % inputBufferSize;
    inputSamplesAvailable++;

    //======================================================================
    // PROCESSING: Generate grains when we have enough input
    //======================================================================

    // Check if we should generate a new grain
    // We generate grains at the synthesis hop rate
    float synthHop = getSynthesisHopSize();

    while (inputSamplesAvailable >= windowSize && outputPhase >= synthHop) {
      processGrains();
      outputPhase -= synthHop;
    }

    outputPhase += 1.0f;

    //======================================================================
    // OUTPUT: Read from output ring buffer
    //======================================================================

    output[i] = outputBuffer[static_cast<size_t>(outputReadPos)];

    // Clear the sample we just read (for the overlap-add accumulator)
    outputBuffer[static_cast<size_t>(outputReadPos)] = 0.0f;

    outputReadPos = (outputReadPos + 1) % outputBufferSize;
  }
}

void PitchShifter::processGrains() {
  const int inputBufferSize = static_cast<int>(inputBuffer.size());

  // Calculate where to read the next grain from
  // We advance by analysisHopSize in the input for each grain
  int grainStartPos = lastInputGrainStart + analysisHopSize;

  // Ensure we have enough samples
  int availableFromStart = inputWritePos - grainStartPos;
  if (availableFromStart < 0) {
    availableFromStart += inputBufferSize;
  }

  if (availableFromStart < windowSize) {
    return; // Not enough input yet
  }

  //==========================================================================
  // STEP 1: Extract grain from input
  //==========================================================================

  extractGrain(grainStartPos);
  lastInputGrainStart = grainStartPos;
  inputSamplesAvailable -= analysisHopSize;

  //==========================================================================
  // STEP 2: Find optimal position using waveform similarity
  //==========================================================================

  // Nominal position is where we'd place it based purely on synthesis hop
  // Search range is how far we look for a better position
  int searchRange = analysisHopSize / 2;
  int nominalPos = outputWritePos;

  int bestPos = findBestGrainPosition(nominalPos, searchRange);

  //==========================================================================
  // STEP 3: Add grain to output using overlap-add
  //==========================================================================

  addGrainToOutput(bestPos);

  // Advance output write position by synthesis hop
  float synthHop = getSynthesisHopSize();
  outputWritePos = (outputWritePos + static_cast<int>(synthHop)) %
                   static_cast<int>(outputBuffer.size());
}

void PitchShifter::extractGrain(int startPos) {
  /**
   * Extract a windowed grain from the input buffer.
   *
   * The window (Hann window) tapers the grain at both ends,
   * which allows smooth overlap-add without discontinuities.
   */

  const int inputBufferSize = static_cast<int>(inputBuffer.size());

  for (int i = 0; i < windowSize; ++i) {
    int readPos = (startPos + i) % inputBufferSize;
    if (readPos < 0)
      readPos += inputBufferSize;

    // Apply window function while extracting
    grainBuffer[static_cast<size_t>(i)] =
        inputBuffer[static_cast<size_t>(readPos)] *
        windowFunction[static_cast<size_t>(i)];
  }
}

int PitchShifter::findBestGrainPosition(int nominalPos, int searchRange) {
  /**
   * WAVEFORM SIMILARITY SEARCH
   *
   * This is what makes WSOLA sound better than basic OLA.
   * We search around the nominal position to find where the
   * new grain best matches what's already in the output buffer.
   *
   * This reduces phase discontinuities at grain boundaries,
   * which would otherwise cause a "buzzing" or "flanging" sound.
   *
   * We use cross-correlation to measure similarity:
   * Higher correlation = better alignment = smoother transition
   */

  const int outputBufferSize = static_cast<int>(outputBuffer.size());

  // If this is the first grain, just use nominal position
  if (outputWritePos == windowSize) {
    return nominalPos;
  }

  int bestPos = nominalPos;
  float bestCorrelation = -1.0f;

  // Search around nominal position
  int searchStart = nominalPos - searchRange;
  int searchEnd = nominalPos + searchRange;

  for (int pos = searchStart; pos <= searchEnd; ++pos) {
    float correlation = 0.0f;
    float normGrain = 0.0f;
    float normOutput = 0.0f;

    // Calculate correlation over the overlap region
    // We only need to check the first part of the grain (the overlap)
    int overlapLength = std::min(windowSize / 2, windowSize);

    for (int i = 0; i < overlapLength; ++i) {
      int outputPos = (pos + i) % outputBufferSize;
      if (outputPos < 0)
        outputPos += outputBufferSize;

      float grainSample = grainBuffer[static_cast<size_t>(i)];
      float outputSample = outputBuffer[static_cast<size_t>(outputPos)];

      correlation += grainSample * outputSample;
      normGrain += grainSample * grainSample;
      normOutput += outputSample * outputSample;
    }

    // Normalize correlation
    float norm = std::sqrt(normGrain * normOutput);
    if (norm > 1e-10f) {
      correlation /= norm;
    } else {
      correlation = 0.0f;
    }

    // Keep track of best position
    if (correlation > bestCorrelation) {
      bestCorrelation = correlation;
      bestPos = pos;
    }
  }

  return bestPos;
}

void PitchShifter::addGrainToOutput(int startPos) {
  /**
   * OVERLAP-ADD
   *
   * Add the grain to the output buffer, allowing it to overlap
   * with previously placed grains. The window function ensures
   * that overlapping regions sum to approximately 1.0, so the
   * output level stays consistent.
   *
   * Think of it like crossfading between audio clips:
   * As one grain fades out, the next fades in.
   */

  const int outputBufferSize = static_cast<int>(outputBuffer.size());

  for (int i = 0; i < windowSize; ++i) {
    int writePos = (startPos + i) % outputBufferSize;
    if (writePos < 0)
      writePos += outputBufferSize;

    // Accumulate (overlap-add)
    outputBuffer[static_cast<size_t>(writePos)] += grainBuffer[static_cast<size_t>(i)];
  }
}

float PitchShifter::readInputInterpolated(float position) const {
  /**
   * Read from the input buffer at a fractional position using
   * cubic interpolation for high-quality resampling.
   */

  const int inputBufferSize = static_cast<int>(inputBuffer.size());

  int pos0 = static_cast<int>(position);
  float frac = position - static_cast<float>(pos0);

  // Get four samples for cubic interpolation
  int i0 = (pos0 - 1 + inputBufferSize) % inputBufferSize;
  int i1 = pos0 % inputBufferSize;
  int i2 = (pos0 + 1) % inputBufferSize;
  int i3 = (pos0 + 2) % inputBufferSize;

  float y0 = inputBuffer[static_cast<size_t>(i0)];
  float y1 = inputBuffer[static_cast<size_t>(i1)];
  float y2 = inputBuffer[static_cast<size_t>(i2)];
  float y3 = inputBuffer[static_cast<size_t>(i3)];

  return NovaTuneUtils::cubicInterpolate(y0, y1, y2, y3, frac);
}
