#include "HarmonyVoice.h"
#include <cmath>
#include <algorithm>

/**
 * HarmonyVoice.cpp
 *
 * Implementation of a single harmony voice.
 */

HarmonyVoice::HarmonyVoice() {
  // Will be properly initialized in prepare()
}

void HarmonyVoice::prepare(double sr, int maxBlock, int channels) {
  sampleRate = sr;
  maxBlockSize = maxBlock;
  numChannels = channels;

  // Create pitch shifters for each channel
  pitchShifters.resize(static_cast<size_t>(numChannels));
  for (auto &shifter : pitchShifters) {
    shifter.prepare(sampleRate, maxBlockSize);
  }

  // Prepare formant processor
  formantProcessor.prepare(sampleRate, maxBlockSize, numChannels);

  // Set up delay lines for timing humanization
  // Maximum delay of 50ms should be plenty
  maxDelaySamples = static_cast<int>(0.05 * sampleRate);
  delayLines.resize(static_cast<size_t>(numChannels));
  delayWritePositions.resize(static_cast<size_t>(numChannels), 0);

  for (auto &line : delayLines) {
    line.resize(static_cast<size_t>(maxDelaySamples), 0.0f);
  }

  // Voice buffer
  voiceBuffer.setSize(numChannels, maxBlockSize);

  // Calculate smoothing coefficients
  pitchRatioSmoothing = NovaTuneUtils::calculateSmoothingCoeff(5.0f, sampleRate);
  gainSmoothing = NovaTuneUtils::calculateSmoothingCoeff(10.0f, sampleRate);

  // Humanize update interval (~100ms)
  humanizeUpdateIntervalSamples = static_cast<int>(0.1 * sampleRate);

  reset();
}

void HarmonyVoice::reset() {
  for (auto &shifter : pitchShifters) {
    shifter.reset();
  }

  formantProcessor.reset();

  for (auto &line : delayLines) {
    std::fill(line.begin(), line.end(), 0.0f);
  }
  std::fill(delayWritePositions.begin(), delayWritePositions.end(), 0);

  currentDelaySamples = 0.0f;
  currentHarmonyMidi = 0.0f;
  targetPitchRatio = 1.0f;
  currentPitchRatio = 1.0f;
  targetGain = 0.0f;
  currentGain = 0.0f;
  pitchHumanizeOffset = 0.0f;
  timingHumanizeTarget = 0.0f;
  samplesSinceHumanizeUpdate = 0;

  voiceBuffer.clear();
}

void HarmonyVoice::updateFromParameters(int voiceIndex, juce::AudioProcessorValueTreeState &apvts) {
  using namespace ParamIDs;

  // Select parameter IDs based on voice index
  const char *enabledId = nullptr;
  const char *modeId = nullptr;
  const char *diaId = nullptr;
  const char *semiId = nullptr;
  const char *levelId = nullptr;
  const char *panId = nullptr;
  const char *formId = nullptr;
  const char *humTId = nullptr;
  const char *humPId = nullptr;

  switch (voiceIndex) {
  case 0: // Voice A
    enabledId = A_enabled;
    modeId = A_mode;
    diaId = A_intervalDiatonic;
    semiId = A_intervalSemi;
    levelId = A_level;
    panId = A_pan;
    formId = A_formantShift;
    humTId = A_humTiming;
    humPId = A_humPitch;
    break;

  case 1: // Voice B
    enabledId = B_enabled;
    modeId = B_mode;
    diaId = B_intervalDiatonic;
    semiId = B_intervalSemi;
    levelId = B_level;
    panId = B_pan;
    formId = B_formantShift;
    humTId = B_humTiming;
    humPId = B_humPitch;
    break;

  case 2: // Voice C
    enabledId = C_enabled;
    modeId = C_mode;
    diaId = C_intervalDiatonic;
    semiId = C_intervalSemi;
    levelId = C_level;
    panId = C_pan;
    formId = C_formantShift;
    humTId = C_humTiming;
    humPId = C_humPitch;
    break;

  default:
    return;
  }

  // Read parameters
  enabled = apvts.getRawParameterValue(enabledId)->load() > 0.5f;
  mode = static_cast<NovaTuneEnums::HarmonyMode>(
      static_cast<int>(apvts.getRawParameterValue(modeId)->load()));
  diatonicIntervalIndex = static_cast<int>(apvts.getRawParameterValue(diaId)->load());
  semitoneOffset = static_cast<int>(apvts.getRawParameterValue(semiId)->load());
  levelDb = apvts.getRawParameterValue(levelId)->load();
  pan = apvts.getRawParameterValue(panId)->load();
  formantShift = apvts.getRawParameterValue(formId)->load();
  humanizeTimingMs = apvts.getRawParameterValue(humTId)->load();
  humanizePitchCents = apvts.getRawParameterValue(humPId)->load();

  // Calculate gain from dB
  targetGain = enabled ? NovaTuneUtils::dbToGain(levelDb) : 0.0f;

  // Calculate pan gains
  NovaTuneUtils::constantPowerPan(pan, panGainL, panGainR);

  // Update formant processor
  formantProcessor.setFormantShift(formantShift);
}

float HarmonyVoice::calculateHarmonyPitchRatio(const PitchDetector &detector,
                                               const PitchMapper &mapper) {
  if (!detector.isVoiced()) {
    return 1.0f;
  }

  const auto &mappingResult = mapper.getLastResult();
  float leadMidi = mappingResult.leadTargetMidiNote;

  if (leadMidi <= 0.0f) {
    return 1.0f;
  }

  // Calculate harmony target note
  float harmonyMidi = leadMidi;

  switch (mode) {
    case NovaTuneEnums::HarmonyMode::Diatonic: {
      // Use the pitch mapper's calculation for diatonic intervals
      // voiceIndex doesn't matter here since we have our own settings
      // We need to calculate it ourselves based on our interval setting

      // Convert diatonic interval index (0-14) to scale degrees (-7 to +7)
      int scaleDegrees = NovaTuneEnums::diatonicIndexToScaleDegree(diatonicIntervalIndex);

      // Get scale intervals
      const auto &scaleIntervals = NovaTuneEnums::getScaleIntervals(mapper.getScale());

      // Convert scale degrees to semitones
      int semitones = NovaTuneUtils::diatonicToSemitones(scaleDegrees, scaleIntervals);

      harmonyMidi = leadMidi + static_cast<float>(semitones);
      break;
    }

    case NovaTuneEnums::HarmonyMode::Semitone: {
      harmonyMidi = leadMidi + static_cast<float>(semitoneOffset);
      break;
    }
  }

  // Apply pitch humanization
  harmonyMidi += pitchHumanizeOffset / 100.0f; // Convert cents to semitones

  // Store for UI
  currentHarmonyMidi = harmonyMidi;

  // Calculate pitch ratio
  // We're shifting FROM the detected pitch TO the harmony pitch
  float detectedMidi = detector.getMidiNote();
  if (detectedMidi <= 0.0f) {
    return 1.0f;
  }

  float ratio = NovaTuneUtils::getPitchRatio(harmonyMidi, detectedMidi);

  // Clamp to safe range
  return std::clamp(ratio, DSPConfig::minPitchShiftRatio, DSPConfig::maxPitchShiftRatio);
}

void HarmonyVoice::updateHumanization() {
  /**
   * Update humanization parameters.
   * This is called periodically (not every sample) to create
   * slowly-varying random offsets that make the harmony sound
   * more like a real singer.
   */

  // Pitch humanization: random offset within specified range
  float pitchRange = humanizePitchCents;
  float newPitchOffset = NovaTuneUtils::randomFloat(-pitchRange, pitchRange);

  // Smooth transition to new offset (don't jump suddenly)
  pitchHumanizeOffset += 0.1f * (newPitchOffset - pitchHumanizeOffset);

  // Timing humanization: random delay within specified range
  float timingRange = humanizeTimingMs;
  float newTimingMs = NovaTuneUtils::randomFloat(0.0f, timingRange);
  float newTimingSamples = (newTimingMs / 1000.0f) * static_cast<float>(sampleRate);

  // Smooth transition
  timingHumanizeTarget = newTimingSamples;
}

void HarmonyVoice::applyTimingHumanization(juce::AudioBuffer<float> &buffer) {
  /**
   * Apply timing humanization using a variable delay line.
   * The delay smoothly varies to create the illusion of a
   * slightly imperfect backing vocalist.
   */

  if (humanizeTimingMs <= 0.0f) {
    return;
  }

  const int numSamples = buffer.getNumSamples();
  const int channels = buffer.getNumChannels();

  // Smoothing coefficient for delay changes
  float delaySmoothing = 0.001f;

  for (int ch = 0; ch < channels && ch < static_cast<int>(delayLines.size()); ++ch) {
    auto &delayLine = delayLines[static_cast<size_t>(ch)];
    int &writePos = delayWritePositions[static_cast<size_t>(ch)];

    float *data = buffer.getWritePointer(ch);

    for (int i = 0; i < numSamples; ++i) {
      // Smooth delay changes
      currentDelaySamples += delaySmoothing * (timingHumanizeTarget - currentDelaySamples);

      // Write to delay line
      delayLine[static_cast<size_t>(writePos)] = data[i];

      // Read from delay line with interpolation
      float readPos = static_cast<float>(writePos) - currentDelaySamples;
      if (readPos < 0.0f) {
        readPos += static_cast<float>(maxDelaySamples);
      }

      int readPosInt = static_cast<int>(readPos);
      float frac = readPos - static_cast<float>(readPosInt);

      int idx0 = readPosInt % maxDelaySamples;
      int idx1 = (readPosInt + 1) % maxDelaySamples;

      data[i] = NovaTuneUtils::lerp(delayLine[static_cast<size_t>(idx0)],
                                    delayLine[static_cast<size_t>(idx1)],
                                    frac);

      // Advance write position
      writePos = (writePos + 1) % maxDelaySamples;
    }
  }
}

void HarmonyVoice::applyGainAndPan(juce::AudioBuffer<float> &buffer) {
  const int numSamples = buffer.getNumSamples();
  const int channels = buffer.getNumChannels();

  for (int i = 0; i < numSamples; ++i) {
    // Smooth gain changes
    currentGain += gainSmoothing * (targetGain - currentGain);

    if (channels >= 1) {
      float *left = buffer.getWritePointer(0);
      left[i] *= currentGain * panGainL;
    }

    if (channels >= 2) {
      float *right = buffer.getWritePointer(1);
      right[i] *= currentGain * panGainR;
    }
  }
}

void HarmonyVoice::process(juce::AudioBuffer<float> &harmonyBuffer,
                           const juce::AudioBuffer<float> &leadBuffer,
                           const PitchDetector &detector,
                           const PitchMapper &mapper) {
  // Early exit if voice is disabled
  if (!enabled) {
    // Fade out if we were previously on
    if (currentGain > 0.001f) {
      targetGain = 0.0f;
      // Process one buffer to fade out
    } else {
      return;
    }
  }

  const int numSamples = leadBuffer.getNumSamples();
  const int channels = leadBuffer.getNumChannels();

  // Ensure voice buffer is correct size
  voiceBuffer.setSize(channels, numSamples, false, false, true);

  // Copy lead buffer as our source
  voiceBuffer.makeCopyOf(leadBuffer, true);

  //==========================================================================
  // UPDATE HUMANIZATION
  //==========================================================================

  samplesSinceHumanizeUpdate += numSamples;
  if (samplesSinceHumanizeUpdate >= humanizeUpdateIntervalSamples) {
    updateHumanization();
    samplesSinceHumanizeUpdate = 0;
  }

  //==========================================================================
  // CALCULATE AND APPLY PITCH SHIFT
  //==========================================================================

  targetPitchRatio = calculateHarmonyPitchRatio(detector, mapper);

  // Smooth pitch ratio changes
  for (int i = 0; i < numSamples; ++i) {
    currentPitchRatio += pitchRatioSmoothing * (targetPitchRatio - currentPitchRatio);
  }

  // Set pitch ratio on all shifters
  for (auto &shifter : pitchShifters) {
    shifter.setPitchRatio(currentPitchRatio);
  }

  // Process each channel through pitch shifter
  for (int ch = 0; ch < channels && ch < static_cast<int>(pitchShifters.size()); ++ch) {
    float *channelData = voiceBuffer.getWritePointer(ch);
    pitchShifters[static_cast<size_t>(ch)].process(channelData, numSamples);
  }

  //==========================================================================
  // APPLY FORMANT PROCESSING
  //==========================================================================

  // Set formant compensation based on pitch shift
  formantProcessor.setPitchCompensation(currentPitchRatio);
  formantProcessor.process(voiceBuffer);

  //==========================================================================
  // APPLY TIMING HUMANIZATION
  //==========================================================================

  applyTimingHumanization(voiceBuffer);

  //==========================================================================
  // APPLY GAIN AND PAN
  //==========================================================================

  applyGainAndPan(voiceBuffer);

  //==========================================================================
  // ADD TO HARMONY OUTPUT
  //==========================================================================

  for (int ch = 0; ch < channels; ++ch) {
    harmonyBuffer.addFrom(ch, 0, voiceBuffer, ch, 0, numSamples);
  }
}

int HarmonyVoice::getLatencySamples() const {
  int latency = 0;

  // Pitch shifter latency
  if (!pitchShifters.empty()) {
    latency += pitchShifters[0].getLatencySamples();
  }

  // Formant processor latency
  latency += formantProcessor.getLatencySamples();

  // Note: Timing humanization delay is intentional, not latency

  return latency;
}
