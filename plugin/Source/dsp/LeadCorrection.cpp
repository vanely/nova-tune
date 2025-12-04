#include "LeadCorrection.h"
#include "../ParameterIDs.h"
#include <cmath>

/**
 * LeadCorrection.cpp
 *
 * Implementation of the main pitch correction engine.
 */

LeadCorrection::LeadCorrection() {
  // Will be properly initialized in prepare()
}

void LeadCorrection::prepare(double sr, int maxBlockSize, int channels) {
  sampleRate = sr;
  blockSize = maxBlockSize;
  numChannels = channels;

  // Create a pitch shifter for each channel
  pitchShifters.resize(static_cast<size_t>(numChannels));
  for (auto &shifter : pitchShifters) {
    shifter.prepare(sampleRate, maxBlockSize);
  }

  // Dry buffer for mix
  dryBuffer.setSize(numChannels, maxBlockSize);

  // Initialize smoothing coefficient based on default retune speed
  pitchRatioSmoothingCoeff = NovaTuneUtils::calculateSmoothingCoeff(
      retuneSpeedToTimeConstantMs(retuneSpeed), sampleRate);

  reset();
}

void LeadCorrection::reset() {
  for (auto &shifter : pitchShifters) {
    shifter.reset();
  }

  targetPitchRatio = 1.0f;
  currentPitchRatio = 1.0f;
  currentCorrectionAmount = 0.0f;
  humanizeOffset = 0.0f;
  humanizePhase = 0.0f;
  lastDetectedPitch = 0.0f;
  vibratoDepth = 0.0f;

  dryBuffer.clear();
}

void LeadCorrection::updateFromParameters(juce::AudioProcessorValueTreeState &apvts) {
  using namespace ParamIDs;

  setRetuneSpeed(apvts.getRawParameterValue(retuneSpeed)->load());
  setHumanize(apvts.getRawParameterValue(humanize)->load());
  setVibrato(apvts.getRawParameterValue(vibratoAmount)->load());
  setMix(apvts.getRawParameterValue(mix)->load() / 100.0f);
}

void LeadCorrection::setRetuneSpeed(float speed) {
  retuneSpeed = std::clamp(speed, 0.0f, 100.0f);

  // Update smoothing coefficient
  float timeConstantMs = retuneSpeedToTimeConstantMs(retuneSpeed);
  pitchRatioSmoothingCoeff = NovaTuneUtils::calculateSmoothingCoeff(timeConstantMs, sampleRate);
}

void LeadCorrection::setHumanize(float amount) {
  humanizeAmount = std::clamp(amount, 0.0f, 100.0f);
}

void LeadCorrection::setVibrato(float amount) {
  vibratoAmount = std::clamp(amount, 0.0f, 100.0f);
}

void LeadCorrection::setMix(float wetAmount) {
  mix = std::clamp(wetAmount, 0.0f, 1.0f);
}

float LeadCorrection::retuneSpeedToTimeConstantMs(float speed) const {
  /**
   * Convert retune speed (0-100) to time constant in milliseconds.
   *
   * We use an exponential mapping because human perception of
   * speed is logarithmic - the difference between 10ms and 20ms
   * is more noticeable than between 200ms and 210ms.
   *
   * speed 0   → 400ms (very slow, natural)
   * speed 50  → ~25ms (medium, modern pop)
   * speed 100 → ~0.5ms (instant, robotic)
   */

  // Map 0-100 to 400ms-0.5ms using exponential curve
  const float maxTime = 400.0f; // ms at speed 0
  const float minTime = 0.5f;   // ms at speed 100

  // Exponential interpolation: time = maxTime * (minTime/maxTime)^(speed/100)
  float normalizedSpeed = speed / 100.0f;
  float timeMs = maxTime * std::pow(minTime / maxTime, normalizedSpeed);

  return timeMs;
}

float LeadCorrection::calculateTargetPitchRatio(const PitchDetector &detector,
                                                const PitchMapper &mapper) {
  /**
   * Calculate what pitch ratio we need to correct the vocal.
   *
   * If singer is at 420 Hz and target is 440 Hz:
   * ratio = 440 / 420 = 1.0476 (shift up by ~4.76%)
   */

  if (!detector.isVoiced()) {
    // No pitch detected - no correction
    return 1.0f;
  }

  const auto &mappingResult = mapper.getLastResult();

  if (mappingResult.leadTargetFrequencyHz <= 0.0f ||
      mappingResult.detectedFrequencyHz <= 0.0f) {
    return 1.0f;
  }

  // Basic pitch ratio
  float ratio = mappingResult.leadTargetFrequencyHz / mappingResult.detectedFrequencyHz;

  // Clamp to safe range
  ratio = std::clamp(ratio, DSPConfig::minPitchShiftRatio, DSPConfig::maxPitchShiftRatio);

  return ratio;
}

float LeadCorrection::applyHumanization(float targetRatio, float detectedMidi, float targetMidi) {
  /**
   * Apply humanization to make the correction sound more natural.
   *
   * WHAT HUMANIZATION DOES:
   * 1. Reduces correction amount (doesn't fully snap to target)
   * 2. Adds subtle random pitch drift (like a real singer)
   * 3. Preserves some of the original pitch variation
   */

  if (humanizeAmount <= 0.0f) {
    return targetRatio;
  }

  float humanizeFactor = humanizeAmount / 100.0f;

  // 1. CORRECTION REDUCTION
  // Instead of fully correcting, only partially correct based on humanize
  // humanizeFactor 1.0 = 50% correction, 0.0 = full correction
  float correctionReduction = humanizeFactor * 0.5f;

  // Blend between target ratio and 1.0 (no correction)
  float blendedRatio = NovaTuneUtils::lerp(targetRatio, 1.0f, correctionReduction);

  // 2. PITCH DRIFT
  // Add slow, subtle random drift to the target
  // This prevents the "too perfect" sound
  humanizePhase += 0.00005f; // Very slow drift
  if (humanizePhase > juce::MathConstants<float>::twoPi) {
    humanizePhase -= juce::MathConstants<float>::twoPi;
  }

  // Use a combination of sine wave and noise for natural drift
  float driftLfo = std::sin(humanizePhase) * 0.5f +
                   std::sin(humanizePhase * 2.7f) * 0.3f +
                   std::sin(humanizePhase * 4.1f) * 0.2f;

  // Drift amount in cents, scaled by humanize
  float maxDriftCents = 8.0f * humanizeFactor;
  float driftCents = driftLfo * maxDriftCents;

  // Convert cents drift to ratio
  float driftRatio = NovaTuneUtils::centsToRatio(driftCents);
  blendedRatio *= driftRatio;

  // 3. PRESERVE EXPRESSION
  // On longer notes, gradually relax correction to preserve expression
  // This is a simplified version - a more sophisticated version would
  // track note duration and apply different amounts

  return blendedRatio;
}

void LeadCorrection::process(juce::AudioBuffer<float> &buffer,
                             const PitchDetector &detector,
                             const PitchMapper &mapper) {
  const int numSamples = buffer.getNumSamples();
  const int channels = buffer.getNumChannels();

  // Store dry signal for mix
  dryBuffer.makeCopyOf(buffer, true);

  //==========================================================================
  // CALCULATE TARGET PITCH RATIO
  //==========================================================================

  targetPitchRatio = calculateTargetPitchRatio(detector, mapper);

  // Apply humanization if enabled
  const auto &mappingResult = mapper.getLastResult();
  if (humanizeAmount > 0.0f && detector.isVoiced()) {
    targetPitchRatio = applyHumanization(
        targetPitchRatio,
        mappingResult.detectedMidiNote,
        mappingResult.leadTargetMidiNote);
  }

  //==========================================================================
  // SMOOTH THE PITCH RATIO
  // This is where the retune speed actually takes effect
  //==========================================================================

  // Per-sample smoothing would be more accurate, but per-block is good enough
  // and more efficient
  float smoothedRatio = currentPitchRatio;

  for (int i = 0; i < numSamples; ++i) {
    smoothedRatio += pitchRatioSmoothingCoeff * (targetPitchRatio - smoothedRatio);
  }

  currentPitchRatio = smoothedRatio;

  // Update correction amount for UI (in semitones)
  currentCorrectionAmount = NovaTuneUtils::ratioToSemitones(currentPitchRatio);

  //==========================================================================
  // APPLY PITCH SHIFTING
  //==========================================================================

  // Set the pitch ratio on all channel shifters
  for (auto &shifter : pitchShifters) {
    shifter.setPitchRatio(currentPitchRatio);
  }

  // Process each channel
  for (int ch = 0; ch < channels && ch < static_cast<int>(pitchShifters.size()); ++ch) {
    float *channelData = buffer.getWritePointer(ch);
    pitchShifters[static_cast<size_t>(ch)].process(channelData, numSamples);
  }

  //==========================================================================
  // APPLY DRY/WET MIX
  //==========================================================================

  if (mix < 1.0f) {
    float wetGain = mix;
    float dryGain = 1.0f - mix;

    for (int ch = 0; ch < channels; ++ch) {
      float *wet = buffer.getWritePointer(ch);
      const float *dry = dryBuffer.getReadPointer(ch);

      for (int i = 0; i < numSamples; ++i)
      {
        wet[i] = wet[i] * wetGain + dry[i] * dryGain;
      }
    }
  }
}

int LeadCorrection::getLatencySamples() const noexcept {
  // Return the latency of one shifter (they're all the same)
  if (!pitchShifters.empty()) {
    return pitchShifters[0].getLatencySamples();
  }
  return 0;
}
