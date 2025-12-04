#pragma once

/**
 * DSPConfig.h
 *
 * Central configuration for all DSP (Digital Signal Processing) constants.
 * Having these in one place makes tuning the algorithm easy - you don't have
 * to hunt through multiple files to find "magic numbers".
 *
 * ANALOGY: Think of this like environment variables or a .env file in web dev.
 */

namespace DSPConfig {
  //==========================================================================
  // HARMONY CONFIGURATION
  //==========================================================================

  /** Maximum number of harmony voices (A, B, C) */
  constexpr int maxHarmonyVoices = 3;

  //==========================================================================
  // PARAMETER RANGES
  // These define min/max values for user-controllable parameters
  //==========================================================================

  // Retune Speed: 0 = very slow (natural), 100 = instant (robotic)
  constexpr float retuneSpeedMin = 0.0f;
  constexpr float retuneSpeedMax = 100.0f;

  // Humanize: Adds natural variation to prevent robotic sound
  constexpr float humanizeMin = 0.0f;
  constexpr float humanizeMax = 100.0f;

  // Vibrato Amount: Controls how much natural vibrato to preserve
  constexpr float vibratoMin = 0.0f;
  constexpr float vibratoMax = 100.0f;

  // Mix: Dry/Wet balance (0% = dry original, 100% = fully processed)
  constexpr float mixMin = 0.0f;
  constexpr float mixMax = 100.0f;

  // Per-voice level in decibels (dB)
  // -48 dB is nearly silent, +6 dB is boosted
  constexpr float levelMinDb = -48.0f;
  constexpr float levelMaxDb = 6.0f;

  // Stereo pan position: -1.0 = full left, +1.0 = full right
  constexpr float panMin = -1.0f;
  constexpr float panMax = 1.0f;

  // Formant shift in semitones
  // Negative = deeper voice, Positive = higher voice character
  constexpr float formantMin = -6.0f;
  constexpr float formantMax = 6.0f;

  // Timing humanization: Random delay in milliseconds per phrase
  constexpr float humTimingMinMs = 0.0f;
  constexpr float humTimingMaxMs = 30.0f;

  // Pitch humanization: Random pitch deviation in cents (1 cent = 1/100 semitone)
  constexpr float humPitchMinCents = 0.0f;
  constexpr float humPitchMaxCents = 15.0f;

  //==========================================================================
  // PITCH DETECTION CONFIGURATION
  // These control the YIN algorithm behavior
  //==========================================================================

  /**
   * YIN threshold: Lower = more accurate but may miss quiet notes
   * Typical values: 0.10 to 0.20
   */
  constexpr float yinThreshold = 0.15f;

  /**
   * Analysis frame size in samples at 44.1kHz
   * Larger = more accurate for low frequencies but higher latency
   * 2048 samples ≈ 46ms at 44.1kHz - good for vocals down to ~80Hz
   */
  constexpr int pitchDetectionFrameSize = 2048;

  /**
   * Hop size: How many samples between each pitch analysis
   * Smaller = more responsive but more CPU
   * 512 samples ≈ 11.6ms at 44.1kHz
   */
  constexpr int pitchDetectionHopSize = 256;

  //==========================================================================
  // PITCH RANGE CONFIGURATION BY VOICE TYPE
  // Frequencies in Hz - used to limit pitch search range
  //==========================================================================

  // Soprano: ~250Hz to 1100Hz (C4 to C6)
  constexpr float sopranoMinHz = 220.0f;
  constexpr float sopranoMaxHz = 1200.0f;

  // Alto/Tenor: ~130Hz to 700Hz (C3 to F5)
  constexpr float altoTenorMinHz = 110.0f;
  constexpr float altoTenorMaxHz = 750.0f;

  // Low Male (Bass/Baritone): ~80Hz to 400Hz (E2 to G4)
  constexpr float lowMaleMinHz = 65.0f;
  constexpr float lowMaleMaxHz = 450.0f;

  // Instrument: Wide range for general use
  constexpr float instrumentMinHz = 50.0f;
  constexpr float instrumentMaxHz = 2000.0f;

  //==========================================================================
  // WSOLA (Pitch Shifting) CONFIGURATION
  //==========================================================================

  /**
   * WSOLA window size in milliseconds
   * Larger = smoother but more latency, smaller = less latency but may have artifacts
   */
  constexpr float wsolaWindowMs = 25.0f;

  /**
   * WSOLA overlap factor (0.0 to 1.0)
   * Higher overlap = smoother crossfades but more CPU
   */
  constexpr float wsolaOverlapFactor = 0.5f;

  /**
   * Maximum pitch shift ratio (for safety)
   * 2.0 = up to one octave shift
   */
  constexpr float maxPitchShiftRatio = 2.0f;
  constexpr float minPitchShiftRatio = 0.5f;

  //==========================================================================
  // LATENCY CONFIGURATION
  //==========================================================================

  /**
   * Live Mode: Prioritizes low latency over quality
   * Target: < 64 samples additional latency
   */
  constexpr int liveModeLatencySamples = 64;

  /**
   * Mix Mode: Prioritizes quality over latency
   * Allows more lookahead for smoother pitch transitions
   */
  constexpr int mixModeLatencySamples = 512;

  //==========================================================================
  // SMOOTHING CONFIGURATION
  //==========================================================================

  /**
   * Pitch smoothing time constant in milliseconds
   * Higher = smoother pitch transitions but slower response
   */
  constexpr float pitchSmoothingMs = 5.0f;

  /**
   * Gain smoothing time constant in milliseconds
   * Prevents clicks when changing volume
   */
  constexpr float gainSmoothingMs = 10.0f;

  //==========================================================================
  // BUFFER SIZES
  //==========================================================================

  /** Maximum expected block size from host DAW */
  constexpr int maxBlockSize = 4096;

  /**
   * Ring buffer size for delay lines and analysis
   * Must be power of 2 for efficient modulo operations
   */
  constexpr int ringBufferSize = 8192;

  //==========================================================================
  // MUSICAL CONSTANTS
  //==========================================================================

  /** Concert pitch A4 in Hz */
  constexpr float concertPitchHz = 440.0f;

  /** MIDI note number for A4 */
  constexpr int midiNoteA4 = 69;

  /** Cents per semitone */
  constexpr float centsPerSemitone = 100.0f;

  /** Semitones per octave */
  constexpr int semitonesPerOctave = 12;

} // namespace DSPConfig
