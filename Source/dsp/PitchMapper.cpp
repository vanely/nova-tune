#include "PitchMapper.h"
#include <cmath>
#include <algorithm>

/**
 * PitchMapper.cpp
 *
 * Implementation of pitch-to-scale mapping and harmony interval calculations.
 */

PitchMapper::PitchMapper() {
  // Initialize with C Major scale
  scaleIntervals = NovaTuneEnums::getScaleIntervals(NovaTuneEnums::Scale::Major);
  keyRootNote = 0; // C
}

void PitchMapper::prepare(double sr) {
  sampleRate = sr;
}

void PitchMapper::reset() {
  lastResult = PitchMappingResult();
}

void PitchMapper::updateFromParameters(juce::AudioProcessorValueTreeState &apvts) {
  using namespace ParamIDs;

  // Get key and scale
  int keyIndex = static_cast<int>(apvts.getRawParameterValue(key)->load());
  int scaleIndex = static_cast<int>(apvts.getRawParameterValue(scale)->load());

  currentKey = static_cast<NovaTuneEnums::Key>(keyIndex);
  currentScale = static_cast<NovaTuneEnums::Scale>(scaleIndex);

  // Update cached scale data
  keyRootNote = keyIndex; // 0=C, 1=C#, etc.
  scaleIntervals = NovaTuneEnums::getScaleIntervals(currentScale);

  // Update harmony settings for each voice
  const char *enabledIds[] = {A_enabled, B_enabled, C_enabled};
  const char *modeIds[] = {A_mode, B_mode, C_mode};
  const char *diaIds[] = {A_intervalDiatonic, B_intervalDiatonic, C_intervalDiatonic};
  const char *semiIds[] = {A_intervalSemi, B_intervalSemi, C_intervalSemi};

  for (int i = 0; i < DSPConfig::maxHarmonyVoices; ++i) {
    harmonySettings[i].enabled = apvts.getRawParameterValue(enabledIds[i])->load() > 0.5f;
    harmonySettings[i].mode = static_cast<NovaTuneEnums::HarmonyMode>(
        static_cast<int>(apvts.getRawParameterValue(modeIds[i])->load()));
    harmonySettings[i].diatonicIntervalIndex =
        static_cast<int>(apvts.getRawParameterValue(diaIds[i])->load());
    harmonySettings[i].semitoneOffset =
        static_cast<int>(apvts.getRawParameterValue(semiIds[i])->load());
  }
}

PitchMappingResult PitchMapper::map(const PitchDetector &detector) {
  PitchMappingResult result;

  // Copy detection results
  result.detectedMidiNote = detector.getMidiNote();
  result.detectedFrequencyHz = detector.getFrequencyHz();
  result.isVoiced = detector.isVoiced();

  if (!result.isVoiced) {
    // No pitch detected - copy zeros for everything
    result.leadTargetMidiNote = 0.0f;
    result.leadTargetFrequencyHz = 0.0f;
    result.centsOffTarget = 0.0f;

    for (int i = 0; i < DSPConfig::maxHarmonyVoices; ++i) {
      result.harmonyTargetMidiNotes[i] = 0.0f;
    }

    lastResult = result;
    return result;
  }

  // Quantize detected note to scale
  result.leadTargetMidiNote = quantizeToScale(result.detectedMidiNote);
  result.leadTargetFrequencyHz = NovaTuneUtils::midiNoteToFrequency(result.leadTargetMidiNote);

  // Calculate how far off the singer was (for UI display)
  result.centsOffTarget = (result.detectedMidiNote - result.leadTargetMidiNote) * 100.0f;

  // Calculate harmony targets
  for (int i = 0; i < DSPConfig::maxHarmonyVoices; ++i) {
    if (harmonySettings[i].enabled) {
      result.harmonyTargetMidiNotes[i] = calculateHarmonyTarget(i, result.leadTargetMidiNote);
    } else {
      result.harmonyTargetMidiNotes[i] = 0.0f;
    }
  }

  lastResult = result;
  return result;
}

float PitchMapper::calculateHarmonyTarget(int voiceIndex, float baseMidiNote) const {
  if (voiceIndex < 0 || voiceIndex >= DSPConfig::maxHarmonyVoices) {
    return baseMidiNote;
  }

  const HarmonySettings &settings = harmonySettings[voiceIndex];

  if (!settings.enabled) {
    return 0.0f;
  }

  float targetMidi = baseMidiNote;

  switch (settings.mode) {
    case NovaTuneEnums::HarmonyMode::Diatonic: {
      // Convert diatonic interval index (0-14) to scale degrees (-7 to +7)
      int scaleDegrees = NovaTuneEnums::diatonicIndexToScaleDegree(settings.diatonicIntervalIndex);

      // Convert scale degrees to semitones
      int semitones = diatonicToSemitones(scaleDegrees, baseMidiNote);

      targetMidi = baseMidiNote + static_cast<float>(semitones);
      break;
    }

    case NovaTuneEnums::HarmonyMode::Semitone: {
      // Simple: just add the semitone offset
      targetMidi = baseMidiNote + static_cast<float>(settings.semitoneOffset);
      break;
    }

    default:
      break;
  }

  return targetMidi;
}

float PitchMapper::quantizeToScale(float midiNote) const {
  /**
   * Quantize a MIDI note to the nearest note in the current scale.
   *
   * Algorithm:
   * 1. Get the pitch class (0-11) of the input note
   * 2. Find the nearest pitch class that's in the scale
   * 3. Return the input note snapped to that pitch class
   */

  // For chromatic scale, just round to nearest semitone
  if (currentScale == NovaTuneEnums::Scale::Chromatic) {
    return std::round(midiNote);
  }

  int roundedNote = static_cast<int>(std::round(midiNote));

  // Get pitch class relative to C (0-11)
  int pitchClass = roundedNote % 12;
  if (pitchClass < 0)
    pitchClass += 12;

  // Get pitch class relative to key root
  int relativePitchClass = (pitchClass - keyRootNote + 12) % 12;

  // Check if it's already in the scale
  for (int interval : scaleIntervals) {
    if (interval == relativePitchClass) {
      // Already in scale - return rounded note
      return static_cast<float>(roundedNote);
    }
  }

  // Not in scale - find the nearest scale note
  int nearestInterval = 0;
  int minDistance = 12;

  for (int interval : scaleIntervals) {
    // Distance in semitones (considering wrapping)
    int dist = std::abs(relativePitchClass - interval);
    dist = std::min(dist, 12 - dist);

    if (dist < minDistance) {
      minDistance = dist;
      nearestInterval = interval;
    }
  }

  // Calculate the adjustment needed
  int targetRelativePitchClass = nearestInterval;
  int currentRelativePitchClass = relativePitchClass;

  int adjustment = targetRelativePitchClass - currentRelativePitchClass;

  // Choose the shortest path (could go up or down)
  if (adjustment > 6)
    adjustment -= 12;
  if (adjustment < -6)
    adjustment += 12;

  return static_cast<float>(roundedNote + adjustment);
}

float PitchMapper::findNearestScaleNote(float midiNote) const {
  return quantizeToScale(midiNote);
}

bool PitchMapper::isNoteInScale(int midiNote) const {
  // Get pitch class relative to key root
  int pitchClass = midiNote % 12;
  if (pitchClass < 0)
    pitchClass += 12;

  int relativePitchClass = (pitchClass - keyRootNote + 12) % 12;

  for (int interval : scaleIntervals) {
    if (interval == relativePitchClass) {
      return true;
    }
  }

  return false;
}

int PitchMapper::diatonicToSemitones(int scaleDegrees, float fromMidiNote) const {
  /**
   * Convert a diatonic interval (in scale degrees) to semitones.
   *
   * Example in C Major:
   * - +1 degree from C (0) = D (2) = +2 semitones
   * - +1 degree from E (4) = F (5) = +1 semitone (half step E-F)
   * - +2 degrees from C = E = +4 semitones
   *
   * This is more complex than it sounds because different intervals
   * in the scale have different semitone distances!
   */

  if (scaleDegrees == 0) {
    return 0;
  }

  int numScaleNotes = static_cast<int>(scaleIntervals.size());

  // Find the scale degree of the starting note
  int startMidi = static_cast<int>(std::round(fromMidiNote));
  int startPitchClass = startMidi % 12;
  if (startPitchClass < 0)
    startPitchClass += 12;
  int startRelative = (startPitchClass - keyRootNote + 12) % 12;

  // Find which scale degree this is
  int startScaleDegree = 0;
  for (int i = 0; i < numScaleNotes; ++i) {
    if (scaleIntervals[static_cast<size_t>(i)] == startRelative) {
      startScaleDegree = i;
      break;
    }
    // If not exactly on a scale note, find the nearest below
    if (scaleIntervals[static_cast<size_t>(i)] < startRelative) {
      startScaleDegree = i;
    }
  }

  // Calculate target scale degree
  int targetScaleDegree = startScaleDegree + scaleDegrees;

  // Handle octave wrapping
  int octaveShift = 0;
  while (targetScaleDegree >= numScaleNotes) {
    targetScaleDegree -= numScaleNotes;
    octaveShift += 12;
  }
  while (targetScaleDegree < 0) {
    targetScaleDegree += numScaleNotes;
    octaveShift -= 12;
  }

  // Get the semitone interval for the target scale degree
  int targetInterval = scaleIntervals[static_cast<size_t>(targetScaleDegree)];
  int startInterval = scaleIntervals[static_cast<size_t>(startScaleDegree)];

  // Calculate total semitone shift
  int semitones = (targetInterval - startInterval) + octaveShift;

  // Adjust for notes that weren't exactly on a scale degree
  // (the difference between startRelative and startInterval)
  // This keeps harmonies tight even when the lead isn't exactly on pitch

  return semitones;
}
