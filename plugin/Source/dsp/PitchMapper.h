#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../ParameterIDs.h"
#include "../Utilities.h"
#include "PitchDetector.h"

/**
 * PitchMapper.h
 *
 * Handles the musical intelligence of NovaTune:
 * - Maps detected pitches to target notes in the selected key/scale
 * - Calculates harmony voice intervals
 *
 * WHAT IS SCALE MAPPING?
 * When you set the key to "C Major", only these notes are "allowed":
 * C, D, E, F, G, A, B (the white keys on a piano)
 *
 * If the singer is slightly flat on an E (say, singing E♭), we need to
 * decide whether to pull them up to E or down to D. That's what this does.
 *
 * DIATONIC vs CHROMATIC HARMONIES:
 *
 * Diatonic: "Stay in the scale"
 * - In C Major, a "3rd above C" is E (4 semitones)
 * - But a "3rd above D" is F (3 semitones) - still a 3rd, different distance
 * - Sounds more natural and musical
 *
 * Chromatic/Semitone: "Fixed interval"
 * - Always shift by exactly N semitones
 * - A "+4 semitones" is always +4, whether it's in the scale or not
 * - More predictable but may sound "outside"
 */

/**
 * Holds the result of pitch mapping - the target notes for lead and harmonies.
 */
struct PitchMappingResult {
  // Original detected pitch
  float detectedMidiNote = 0.0f;
  float detectedFrequencyHz = 0.0f;
  bool isVoiced = false;

  // Target for lead voice (after scale quantization)
  float leadTargetMidiNote = 0.0f;
  float leadTargetFrequencyHz = 0.0f;

  // How far off was the singer? (useful for UI)
  float centsOffTarget = 0.0f;

  // For harmony voices (computed when needed)
  float harmonyTargetMidiNotes[DSPConfig::maxHarmonyVoices] = {0.0f, 0.0f, 0.0f};
};

class PitchMapper {
public:
  PitchMapper();
  ~PitchMapper() = default;

  /**
   * Prepare for processing.
   * @param sampleRate Audio sample rate (for any smoothing calculations)
   */
  void prepare(double sampleRate);

  /**
   * Reset state.
   */
  void reset();

  /**
   * Update mapping parameters from the plugin's parameter state.
   * Call this once per process block before calling map().
   *
   * @param apvts The AudioProcessorValueTreeState containing all parameters
   */
  void updateFromParameters(juce::AudioProcessorValueTreeState &apvts);

  /**
   * Map detected pitch to target notes.
   *
   * @param detector The pitch detector with current detection results
   * @return Mapping result with lead and harmony targets
   */
  PitchMappingResult map(const PitchDetector &detector);

  /**
   * Calculate the target MIDI note for a specific harmony voice.
   *
   * @param voiceIndex Which harmony voice (0=A, 1=B, 2=C)
   * @param baseMidiNote The note to calculate harmony from (usually the corrected lead)
   * @return Target MIDI note for this harmony voice
   */
  float calculateHarmonyTarget(int voiceIndex, float baseMidiNote) const;

  //==========================================================================
  // GETTERS
  //==========================================================================

  NovaTuneEnums::Key getKey() const noexcept { return currentKey; }
  NovaTuneEnums::Scale getScale() const noexcept { return currentScale; }

  /** Get the current mapping result (from last map() call) */
  const PitchMappingResult &getLastResult() const noexcept { return lastResult; }

private:
  //==========================================================================
  // PARAMETERS
  //==========================================================================

  double sampleRate = 44100.0;

  NovaTuneEnums::Key currentKey = NovaTuneEnums::Key::C;
  NovaTuneEnums::Scale currentScale = NovaTuneEnums::Scale::Major;

  // Cached scale intervals for current key/scale
  std::vector<int> scaleIntervals;
  int keyRootNote = 0; // 0=C, 1=C#, etc.

  // Harmony voice settings (per voice)
  struct HarmonySettings {
    bool enabled = false;
    NovaTuneEnums::HarmonyMode mode = NovaTuneEnums::HarmonyMode::Diatonic;
    int diatonicIntervalIndex = 7; // 7 = unison
    int semitoneOffset = 0;
  };

  HarmonySettings harmonySettings[DSPConfig::maxHarmonyVoices];

  // Result storage
  PitchMappingResult lastResult;

  //==========================================================================
  // HELPER METHODS
  //==========================================================================

  /**
   * Quantize a MIDI note to the nearest note in the current scale.
   *
   * @param midiNote Input MIDI note (can be fractional)
   * @return Quantized MIDI note (integer, in scale)
   */
  float quantizeToScale(float midiNote) const;

  /**
   * Find the nearest scale note to a given MIDI note.
   * Unlike quantizeToScale, this always snaps to the nearest scale note
   * without considering whether it's above or below.
   *
   * @param midiNote Input MIDI note
   * @return Nearest scale note
   */
  float findNearestScaleNote(float midiNote) const;

  /**
   * Check if a note is in the current scale.
   *
   * @param midiNote MIDI note to check
   * @return True if the note is in the scale
   */
  bool isNoteInScale(int midiNote) const;

  /**
   * Convert a diatonic interval to semitones in the current scale.
   *
   * @param scaleDegrees Number of scale degrees to move (negative = down)
   * @param fromMidiNote Starting MIDI note
   * @return Number of semitones for this interval
   */
  int diatonicToSemitones(int scaleDegrees, float fromMidiNote) const;
};
