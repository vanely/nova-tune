#include "PitchDetector.h"
#include <cmath>
#include <algorithm>

/**
 * PitchDetector.cpp
 *
 * Implementation of the YIN algorithm for fundamental frequency detection.
 *
 * Reference: "YIN, a fundamental frequency estimator for speech and music"
 * de Cheveigné & Kawahara, Journal of the Acoustical Society of America, 2002
 */

PitchDetector::PitchDetector() {
  // Pre-allocate buffers with reasonable defaults
  // These will be resized properly in prepare()
  inputRingBuffer.resize(DSPConfig::ringBufferSize, 0.0f);
  yinBuffer.resize(DSPConfig::pitchDetectionFrameSize / 2, 0.0f);
}

void PitchDetector::prepare(double sr, int maxBlockSize) {
  sampleRate = sr;

  // Calculate frame size that works well at this sample rate
  // We want roughly the same time duration regardless of sample rate
  // ~46ms at 44.1kHz = 2048 samples
  frameSize = static_cast<int>(0.046 * sampleRate);
  // Round up to nearest power of 2 for efficiency
  frameSize = juce::nextPowerOfTwo(frameSize);
  frameSize = std::min(frameSize, 4096); // Cap to prevent excessive latency

  // Hop size: how often we analyze (smaller = more responsive, more CPU)
  hopSize = frameSize / 8; // Analyze every ~6ms

  // Resize internal buffers
  monoBuffer.setSize(1, maxBlockSize);
  analysisFrame.setSize(1, frameSize);
  yinBuffer.resize(static_cast<size_t>(frameSize / 2));

  // Ring buffer must be at least frameSize
  int ringSize = std::max(DSPConfig::ringBufferSize, frameSize * 2);
  // Ensure power of 2 for efficient modulo
  ringSize = juce::nextPowerOfTwo(ringSize);
  inputRingBuffer.resize(static_cast<size_t>(ringSize), 0.0f);

  updateFrequencyRange();
  reset();
}

void PitchDetector::reset() {
  // Clear all state
  std::fill(inputRingBuffer.begin(), inputRingBuffer.end(), 0.0f);
  ringBufferWritePos = 0;
  samplesUntilNextAnalysis = 0;

  detectedFrequencyHz = 0.0f;
  detectedMidiNote = 0.0f;
  detectedPeriod = 0.0f;
  voiced = false;
  confidence = 0.0f;
}

void PitchDetector::setInputType(NovaTuneEnums::InputType type) {
  inputType = type;
  updateFrequencyRange();
}

void PitchDetector::updateFrequencyRange() {
  // Set frequency search range based on voice type
  // This prevents octave errors by limiting where we look for the pitch
  switch (inputType) {
    case NovaTuneEnums::InputType::Soprano:
      minFreqHz = DSPConfig::sopranoMinHz;
      maxFreqHz = DSPConfig::sopranoMaxHz;
      break;
    case NovaTuneEnums::InputType::AltoTenor:
      minFreqHz = DSPConfig::altoTenorMinHz;
      maxFreqHz = DSPConfig::altoTenorMaxHz;
      break;
    case NovaTuneEnums::InputType::LowMale:
      minFreqHz = DSPConfig::lowMaleMinHz;
      maxFreqHz = DSPConfig::lowMaleMaxHz;
      break;
    case NovaTuneEnums::InputType::Instrument:
    default:
      minFreqHz = DSPConfig::instrumentMinHz;
      maxFreqHz = DSPConfig::instrumentMaxHz;
      break;
  }
}

void PitchDetector::process(const juce::AudioBuffer<float> &buffer) {
  const int numSamples = buffer.getNumSamples();
  const int numChannels = buffer.getNumChannels();

  if (numSamples == 0)
    return;

  //==========================================================================
  // Step 1: Convert stereo to mono
  // We only need one channel for pitch detection
  //==========================================================================

  monoBuffer.setSize(1, numSamples, false, false, true);
  auto *mono = monoBuffer.getWritePointer(0);

  // Copy first channel
  std::copy(buffer.getReadPointer(0),
            buffer.getReadPointer(0) + numSamples,
            mono);

  // Add other channels and average
  for (int ch = 1; ch < numChannels; ++ch) {
    const auto *channelData = buffer.getReadPointer(ch);
    for (int i = 0; i < numSamples; ++i) {
      mono[i] += channelData[i];
    }
  }

  if (numChannels > 1) {
    const float invChannels = 1.0f / static_cast<float>(numChannels);
    for (int i = 0; i < numSamples; ++i) {
      mono[i] *= invChannels;
    }
  }

  //==========================================================================
  // Step 2: Feed samples into ring buffer
  //==========================================================================

  const int ringSize = static_cast<int>(inputRingBuffer.size());

  for (int i = 0; i < numSamples; ++i) {
    inputRingBuffer[static_cast<size_t>(ringBufferWritePos)] = mono[i];
    ringBufferWritePos = (ringBufferWritePos + 1) % ringSize;

    // Decrement counter and check if it's time to analyze
    if (--samplesUntilNextAnalysis <= 0) {
      samplesUntilNextAnalysis = hopSize;

      //==================================================================
      // Step 3: Extract analysis frame from ring buffer
      //==================================================================

      auto *frame = analysisFrame.getWritePointer(0);
      int readPos = (ringBufferWritePos - frameSize + ringSize) % ringSize;

      for (int j = 0; j < frameSize; ++j) {
        frame[j] = inputRingBuffer[static_cast<size_t>((readPos + j) % ringSize)];
      }

      //==================================================================
      // Step 4: Run YIN algorithm
      //==================================================================

      // 4a: Compute difference function
      computeDifferenceFunction(frame, frameSize);

      // 4b: Cumulative mean normalized difference
      computeCumulativeMeanNormalizedDifference();

      // 4c: Absolute threshold to find period
      float rawPeriod = absoluteThreshold();

      if (rawPeriod > 0.0f) {
        // 4d: Refine with parabolic interpolation
        detectedPeriod = parabolicInterpolation(static_cast<int>(rawPeriod));
        detectedFrequencyHz = periodToFrequency(detectedPeriod);

        // Validate frequency is in expected range
        if (detectedFrequencyHz >= minFreqHz && detectedFrequencyHz <= maxFreqHz) {
          voiced = true;
          detectedMidiNote = NovaTuneUtils::frequencyToMidiNote(detectedFrequencyHz);

          // Confidence is inverse of the YIN value at the detected period
          int tauInt = static_cast<int>(rawPeriod);
          if (tauInt < static_cast<int>(yinBuffer.size())) {
            confidence = 1.0f - yinBuffer[static_cast<size_t>(tauInt)];
            confidence = std::clamp(confidence, 0.0f, 1.0f);
          }
        } else {
          // Frequency out of range - likely an octave error
          voiced = false;
          confidence = 0.0f;
        }
      } else {
        // No pitch detected (silence or noise)
        voiced = false;
        detectedFrequencyHz = 0.0f;
        detectedMidiNote = 0.0f;
        detectedPeriod = 0.0f;
        confidence = 0.0f;
      }
    }
  }
}

void PitchDetector::computeDifferenceFunction(const float *input, int numSamples) {
  /**
   * Difference function d(τ):
   * d(τ) = Σ (x[j] - x[j+τ])² for j = 0 to W-τ-1
   *
   * Where:
   * - τ (tau) is the lag (potential period)
   * - W is the window size (half of frameSize for YIN)
   * - x[j] is the input signal
   *
   * This measures how different the signal is from a shifted copy.
   * When τ equals the true pitch period, d(τ) will be minimal.
   */

  const int yinSize = numSamples / 2;

  // τ = 0 is always 0 (signal is identical to itself with no shift)
  yinBuffer[0] = 0.0f;

  // For each lag τ from 1 to yinSize-1
  for (int tau = 1; tau < yinSize; ++tau) {
    float sum = 0.0f;

    // Sum of squared differences
    for (int j = 0; j < yinSize; ++j) {
      float diff = input[j] - input[j + tau];
      sum += diff * diff;
    }

    yinBuffer[static_cast<size_t>(tau)] = sum;
  }
}

void PitchDetector::computeCumulativeMeanNormalizedDifference() {
  /**
   * Cumulative Mean Normalized Difference Function d'(τ):
   *
   * d'(τ) = 1                           if τ = 0
   * d'(τ) = d(τ) / [(1/τ) * Σd(j)]     if τ > 0
   *
   * This normalization is key to YIN's success:
   * - Reduces octave errors (tendency to detect harmonics instead of fundamental)
   * - Makes threshold independent of signal amplitude
   * - d'(τ) starts at 1 and dips below threshold at the pitch period
   */

  yinBuffer[0] = 1.0f; // By definition

  float cumulativeSum = 0.0f;

  for (size_t tau = 1; tau < yinBuffer.size(); ++tau) {

    cumulativeSum += yinBuffer[tau];
    // Avoid division by zero
      yinBuffer[tau] *= static_cast<float>(tau) / cumulativeSum;
    if (cumulativeSum > 0.0f) {
    } else {
      yinBuffer[tau] = 1.0f;
    }
  }
}

float PitchDetector::absoluteThreshold() {
  /**
   * Find the first τ where d'(τ) dips below the threshold,
   * then return the τ of the subsequent local minimum.
   *
   * The threshold (typically 0.1-0.2) determines:
   * - Lower = more strict, may miss quiet notes
   * - Higher = more permissive, may detect false pitches
   *
   * We also respect the frequency range limits by converting
   * minFreq/maxFreq to min/max τ (period in samples).
   */

  // Convert frequency limits to period limits
  // period = sampleRate / frequency
  // Higher frequency = smaller period
  int minTau = static_cast<int>(sampleRate / maxFreqHz);
  int maxTau = static_cast<int>(sampleRate / minFreqHz);

  // Clamp to buffer size
  minTau = std::max(2, minTau);
  maxTau = std::min(maxTau, static_cast<int>(yinBuffer.size()) - 1);

  // Search for the first dip below threshold
  int tau = minTau;
  while (tau < maxTau) {
    if (yinBuffer[static_cast<size_t>(tau)] < DSPConfig::yinThreshold) {
      // Found a candidate - now find the local minimum
      while (tau + 1 < maxTau &&
             yinBuffer[static_cast<size_t>(tau + 1)] < yinBuffer[static_cast<size_t>(tau)]) {
        ++tau;
      }
      return static_cast<float>(tau);
    }
    ++tau;
  }

  // No pitch found below threshold
  // Fall back to global minimum (less reliable)
  float minValue = yinBuffer[static_cast<size_t>(minTau)];
  int minIndex = minTau;

  for (int t = minTau + 1; t < maxTau; ++t) {
    if (yinBuffer[static_cast<size_t>(t)] < minValue) {
      minValue = yinBuffer[static_cast<size_t>(t)];
      minIndex = t;
    }
  }

  // Only return if it's reasonably confident
  if (minValue < 0.5f) {
    return static_cast<float>(minIndex);
  }

  return 0.0f; // Unvoiced
}

float PitchDetector::parabolicInterpolation(int tauEstimate) {
  /**
   * Parabolic (quadratic) interpolation for sub-sample accuracy.
   *
   * The true minimum usually lies between integer sample positions.
   * By fitting a parabola through three points (tau-1, tau, tau+1),
   * we can find a more accurate estimate.
   *
   * The interpolated minimum is at:
   * tau_refined = tau + (y[tau-1] - y[tau+1]) / (2 * (y[tau-1] - 2*y[tau] + y[tau+1]))
   */

  if (tauEstimate < 1 || tauEstimate >= static_cast<int>(yinBuffer.size()) - 1) {
    return static_cast<float>(tauEstimate);
  }

  float y0 = yinBuffer[static_cast<size_t>(tauEstimate - 1)];
  float y1 = yinBuffer[static_cast<size_t>(tauEstimate)];
  float y2 = yinBuffer[static_cast<size_t>(tauEstimate + 1)];

  float denominator = 2.0f * (y0 - 2.0f * y1 + y2);

  if (std::abs(denominator) < 1e-10f) {
    return static_cast<float>(tauEstimate);
  }

  float delta = (y0 - y2) / denominator;

  return static_cast<float>(tauEstimate) + delta;
}

float PitchDetector::periodToFrequency(float periodSamples) const {
  /**
   * Convert period (in samples) to frequency (in Hz).
   *
   * frequency = sampleRate / period
   *
   * Example at 44.1kHz:
   * - Period of 100 samples = 441 Hz (A4)
   * - Period of 200 samples = 220.5 Hz (A3)
   */

  if (periodSamples <= 0.0f) {
    return 0.0f;
  }

  return static_cast<float>(sampleRate) / periodSamples;
}
