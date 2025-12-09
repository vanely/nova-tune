#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * ParameterIDs.h
 *
 * Contains all parameter identifiers (string IDs) and enumerations for NovaTune.
 *
 * WHY STRING IDs?
 * DAWs (Digital Audio Workstations) need to identify parameters for:
 * - Automation (recording parameter changes over time)
 * - Preset saving/loading
 * - Host parameter display
 *
 * Using constexpr strings in a namespace prevents typos and enables autocomplete.
 *
 * ANALOGY: Think of these like route paths in a web app - they need to be
 * consistent everywhere they're used.
 */

namespace ParamIDs {
  //==========================================================================
  // GLOBAL / LEAD VOICE PARAMETERS
  //==========================================================================

  /** Musical key (C, C#, D, etc.) */
  static constexpr const char *key = "key";

  /** Musical scale (Major, Minor, Chromatic, etc.) */
  static constexpr const char *scale = "scale";

  /** Input voice type - affects pitch detection range */
  static constexpr const char *inputType = "inputType";

  /**
   * Retune Speed (0-100)
   * 0 = Slow, natural correction (jazz, ballads)
   * 100 = Instant snap to pitch (T-Pain effect)
   */
  static constexpr const char *retuneSpeed = "retuneSpeed";

  /**
   * Humanize (0-100)
   * Preserves natural pitch variation to avoid robotic sound
   */
  static constexpr const char *humanize = "humanize";

  /**
   * Vibrato Amount (0-100)
   * Controls how much natural vibrato to preserve vs. flatten
   */
  static constexpr const char *vibratoAmount = "vibratoAmount";

  /**
   * Mix / Dry-Wet (0-100%)
   * 0% = Original signal (dry)
   * 100% = Fully processed signal (wet)
   */
  static constexpr const char *mix = "mix";

  /** Global bypass - when true, plugin passes audio through unchanged */
  static constexpr const char *bypass = "bypass";

  /**
   * Quality Mode (Live vs Mix)
   * Live = Lower latency, optimized for real-time monitoring
   * Mix = Higher quality, accepts more latency
   */
  static constexpr const char *qualityMode = "qualityMode";

  /**
   * Harmony Preset dropdown
   * Quick way to set up common harmony configurations
   */
  static constexpr const char *harmonyPreset = "harmonyPreset";

  //==========================================================================
  // HARMONY VOICE A PARAMETERS
  //==========================================================================

  static constexpr const char *A_enabled = "A_enabled";
  static constexpr const char *A_mode = "A_mode";
  static constexpr const char *A_intervalDiatonic = "A_intervalDiatonic";
  static constexpr const char *A_intervalSemi = "A_intervalSemi";
  static constexpr const char *A_level = "A_level";
  static constexpr const char *A_pan = "A_pan";
  static constexpr const char *A_formantShift = "A_formantShift";
  static constexpr const char *A_humTiming = "A_humTiming";
  static constexpr const char *A_humPitch = "A_humPitch";

  //==========================================================================
  // HARMONY VOICE B PARAMETERS
  //==========================================================================

  static constexpr const char *B_enabled = "B_enabled";
  static constexpr const char *B_mode = "B_mode";
  static constexpr const char *B_intervalDiatonic = "B_intervalDiatonic";
  static constexpr const char *B_intervalSemi = "B_intervalSemi";
  static constexpr const char *B_level = "B_level";
  static constexpr const char *B_pan = "B_pan";
  static constexpr const char *B_formantShift = "B_formantShift";
  static constexpr const char *B_humTiming = "B_humTiming";
  static constexpr const char *B_humPitch = "B_humPitch";

  //==========================================================================
  // HARMONY VOICE C PARAMETERS
  //==========================================================================

  static constexpr const char *C_enabled = "C_enabled";
  static constexpr const char *C_mode = "C_mode";
  static constexpr const char *C_intervalDiatonic = "C_intervalDiatonic";
  static constexpr const char *C_intervalSemi = "C_intervalSemi";
  static constexpr const char *C_level = "C_level";
  static constexpr const char *C_pan = "C_pan";
  static constexpr const char *C_formantShift = "C_formantShift";
  static constexpr const char *C_humTiming = "C_humTiming";
  static constexpr const char *C_humPitch = "C_humPitch";

} // namespace ParamIDs

/**
 * NovaTuneEnums
 *
 * Enumerations for discrete parameter choices.
 * Each enum also has a corresponding StringArray function for UI display.
 */
namespace NovaTuneEnums {
  //==========================================================================
  // MUSICAL KEY ENUM
  //==========================================================================

  /**
   * Musical keys - the 12 notes of the chromatic scale.
   * The root note that defines the scale.
   */
  enum class Key
  {
    C,
    Cs,
    D,
    Ds,
    E,
    F,
    Fs,
    G,
    Gs,
    A,
    As,
    B,
    numKeys
  };

  /** Display names for keys (shown in UI dropdown) */
  inline const juce::StringArray getKeyNames()
  {
    return {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  }

  //==========================================================================
  // SCALE ENUM
  //==========================================================================

  /**
   * Musical scales - determine which notes are "allowed"
   *
   * Major: Happy sound (C D E F G A B)
   * Natural Minor: Sad sound (C D Eb F G Ab Bb)
   * Harmonic Minor: Middle Eastern feel (minor with raised 7th)
   * Melodic Minor: Jazz sound (different ascending/descending)
   * Chromatic: All 12 notes allowed - no scale restriction
   */
  enum class Scale
  {
    Major,
    NaturalMinor,
    HarmonicMinor,
    MelodicMinor,
    Chromatic,
    numScales
  };

  inline const juce::StringArray getScaleNames()
  {
    return {"Major", "Natural Minor", "Harmonic Minor", "Melodic Minor", "Chromatic"};
  }

  /**
   * Scale intervals in semitones from root.
   * These define which notes belong to each scale.
   * A note is "in scale" if its semitone offset from the root is in this array.
   */
  inline const std::vector<int> &getScaleIntervals(Scale scale)
  {
    // Major scale: W W H W W W H (W=whole step=2 semitones, H=half=1)
    static const std::vector<int> major = {0, 2, 4, 5, 7, 9, 11};

    // Natural minor: W H W W H W W
    static const std::vector<int> naturalMinor = {0, 2, 3, 5, 7, 8, 10};

    // Harmonic minor: Natural minor with raised 7th
    static const std::vector<int> harmonicMinor = {0, 2, 3, 5, 7, 8, 11};

    // Melodic minor: Minor with raised 6th and 7th
    static const std::vector<int> melodicMinor = {0, 2, 3, 5, 7, 9, 11};

    // Chromatic: All 12 semitones
    static const std::vector<int> chromatic = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

    switch (scale)
    {
    case Scale::Major:
      return major;
    case Scale::NaturalMinor:
      return naturalMinor;
    case Scale::HarmonicMinor:
      return harmonicMinor;
    case Scale::MelodicMinor:
      return melodicMinor;
    case Scale::Chromatic:
      return chromatic;
    case Scale::numScales:
      return chromatic; // Sentinel value, shouldn't occur in practice
    default:
      return chromatic;
    }
  }

  //==========================================================================
  // INPUT TYPE ENUM
  //==========================================================================

  /**
   * Input type - helps the pitch detector by limiting search range.
   *
   * Why does this matter?
   * Pitch detection algorithms can get confused by octaves.
   * By knowing the approximate vocal range, we avoid "octave errors"
   * where the algorithm detects 2x or 0.5x the actual frequency.
   */
  enum class InputType
  {
    Soprano,    // High female voice: ~250Hz - 1100Hz
    AltoTenor,  // Medium voice: ~130Hz - 700Hz
    LowMale,    // Low male voice: ~80Hz - 400Hz
    Instrument, // Wide range for instruments
    numInputTypes
  };

  inline const juce::StringArray getInputTypeNames()
  {
    return {"Soprano", "Alto/Tenor", "Low Male", "Instrument"};
  }

  //==========================================================================
  // QUALITY MODE ENUM
  //==========================================================================

  /**
   * Quality mode - trade-off between latency and quality.
   *
   * Live Mode: For real-time monitoring while recording
   * - Minimal latency (< 10ms total)
   * - Slightly lower quality pitch estimation
   *
   * Mix Mode: For mixing/mastering
   * - Higher latency is acceptable
   * - Maximum quality processing
   */
  enum class QualityMode
  {
    Live,
    Mix,
    numQualityModes
  };

  inline const juce::StringArray getQualityModeNames()
  {
    return {"Live", "Mix"};
  }

  //==========================================================================
  // HARMONY MODE ENUM
  //==========================================================================

  /**
   * Harmony voice mode - how intervals are calculated.
   *
   * Diatonic: Interval follows the scale
   *   - "+3rd" means 3rd scale degree above (could be 3 or 4 semitones depending on scale)
   *   - Sounds more musical/natural
   *
   * Semitone: Fixed semitone offset
   *   - "+4 semitones" is always exactly 4 semitones (a major 3rd)
   *   - More predictable but may go outside the scale
   */
  enum class HarmonyMode
  {
    Diatonic,
    Semitone,
    numHarmonyModes
  };

  inline const juce::StringArray getHarmonyModeNames()
  {
    return {"Diatonic", "Semitone"};
  }

  //==========================================================================
  // DIATONIC INTERVAL NAMES
  //==========================================================================

  /**
   * Diatonic interval display names.
   *
   * We use 15 values (indices 0-14) where:
   * - Index 0 = -7th (octave below)
   * - Index 7 = Unison (same note)
   * - Index 14 = +Octave
   *
   * The actual mapping to semitones depends on the current scale.
   */
  inline const juce::StringArray getDiatonicIntervalNames() {
    return {
        "-Octave", "-7th", "-6th", "-5th", "-4th", "-3rd", "-2nd",
        "Unison",
        "+2nd", "+3rd", "+4th", "+5th", "+6th", "+7th", "+Octave"};
  }

  /** Convert diatonic interval index (0-14) to scale degree offset (-7 to +7) */
  inline int diatonicIndexToScaleDegree(int index) {
    return index - 7; // Index 7 = 0 (unison)
  }

  //==========================================================================
  // HARMONY PRESET ENUM
  //==========================================================================

  /**
   * Quick-access harmony presets.
   * These configure all three harmony voices with a single selection.
   */
  enum class HarmonyPreset {
    None,             // All harmonies disabled
    Pop3rdUp,         // A = +3rd above
    Pop3rdAnd5th,     // A = +3rd, B = +5th
    ThirdsAboveBelow, // A = +3rd, B = -3rd
    FifthsWide,       // A = +5th left, B = -5th right (stereo)
    OctaveDouble,     // A = +octave
    OctavePlus3rd,    // A = +octave, B = +3rd
    ChoirStack,       // A = +3rd, B = -3rd, C = +5th (full choir)
    numPresets
  };

  inline const juce::StringArray getHarmonyPresetNames() {
    return {
        "None",
        "Pop 3rd Up",
        "Pop 3rd & 5th",
        "Thirds Above & Below",
        "Fifths Wide",
        "Octave Double",
        "Octave + 3rd",
        "Choir Stack"};
  }

} // namespace NovaTuneEnums
