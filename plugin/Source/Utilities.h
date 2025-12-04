#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include "DSPConfig.h"

/**
 * Utilities.h
 *
 * Pure utility functions for common audio/DSP operations.
 * These are stateless helper functions - no side effects, always
 * return the same output for the same input.
 *
 * ANALOGY: Like a utils.ts file with pure helper functions.
 */

namespace NovaTuneUtils {
  //==========================================================================
  // FREQUENCY <-> MIDI NOTE CONVERSIONS
  //==========================================================================

  /**
   * Convert frequency in Hz to MIDI note number (floating point).
   *
   * MIDI notes: 0-127 where 60 = Middle C, 69 = A4 (440 Hz)
   * Using float allows for cents precision (1 semitone = 100 cents)
   *
   * Formula: midiNote = 69 + 12 * log2(freq / 440)
   *
   * @param frequencyHz Frequency in Hertz
   * @return MIDI note number (e.g., 69.0 for 440 Hz, 69.5 for ~452 Hz)
   */
  inline float frequencyToMidiNote(float frequencyHz) {
    if (frequencyHz <= 0.0f)
      return 0.0f;

    return static_cast<float>(DSPConfig::midiNoteA4) +
           12.0f * std::log2(frequencyHz / DSPConfig::concertPitchHz);
  }

  /**
   * Convert MIDI note number to frequency in Hz.
   *
   * Formula: freq = 440 * 2^((midiNote - 69) / 12)
   *
   * @param midiNote MIDI note number (can be fractional)
   * @return Frequency in Hertz
   */
  inline float midiNoteToFrequency(float midiNote) {
    return DSPConfig::concertPitchHz *
           std::pow(2.0f, (midiNote - static_cast<float>(DSPConfig::midiNoteA4)) / 12.0f);
  }

  /**
   * Get the pitch ratio between two MIDI notes.
   *
   * Used for pitch shifting: multiply frequencies by this ratio
   * to shift from sourceMidi to targetMidi.
   *
   * @param targetMidi The note we want to shift TO
   * @param sourceMidi The note we're shifting FROM
   * @return Pitch ratio (e.g., 2.0 for octave up, 0.5 for octave down)
   */
  inline float getPitchRatio(float targetMidi, float sourceMidi) {
    return std::pow(2.0f, (targetMidi - sourceMidi) / 12.0f);
  }

  /**
   * Convert semitones to pitch ratio.
   *
   * @param semitones Number of semitones (positive = up, negative = down)
   * @return Pitch ratio
   */
  inline float semitonesToRatio(float semitones) {
    return std::pow(2.0f, semitones / 12.0f);
  }

  /**
   * Convert pitch ratio to semitones.
   *
   * @param ratio Pitch ratio (e.g., 2.0 = octave up)
   * @return Semitones (e.g., 12.0 for octave)
   */
  inline float ratioToSemitones(float ratio) {
    if (ratio <= 0.0f)
      return 0.0f;
    return 12.0f * std::log2(ratio);
  }

  /**
   * Convert cents to pitch ratio.
   * 100 cents = 1 semitone
   *
   * @param cents Cents offset
   * @return Pitch ratio
   */
  inline float centsToRatio(float cents) {
    return std::pow(2.0f, cents / 1200.0f);
  }

  //==========================================================================
  // NOTE NAME UTILITIES
  //==========================================================================

  /**
   * Get the note name for a MIDI note number.
   *
   * @param midiNote MIDI note (0-127)
   * @return Note name like "C4", "F#3", etc.
   */
  inline juce::String getMidiNoteName(int midiNote) {
    static const char *noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const int octave = (midiNote / 12) - 1;
    const int note = midiNote % 12;
    return juce::String(noteNames[note]) + juce::String(octave);
  }

  /**
   * Round a floating-point MIDI note to the nearest integer note.
   */
  inline int roundToNearestNote(float midiNote) {
    return static_cast<int>(std::round(midiNote));
  }

  /**
   * Get the fractional part of a MIDI note (cents offset from nearest note).
   *
   * @param midiNote The MIDI note (can be fractional)
   * @return Cents offset (-50 to +50)
   */
  inline float getCentsOffset(float midiNote) {
    float nearestNote = std::round(midiNote);
    return (midiNote - nearestNote) * 100.0f;
  }

  //==========================================================================
  // SCALE QUANTIZATION
  //==========================================================================

  /**
   * Quantize a MIDI note to the nearest note in a scale.
   *
   * @param midiNote The input MIDI note (can be fractional)
   * @param keyRoot The root note of the key (0-11, where 0=C)
   * @param scaleIntervals Vector of semitone intervals that define the scale
   * @return The quantized MIDI note
   */
  inline float quantizeToScale(float midiNote, int keyRoot, const std::vector<int> &scaleIntervals) {
    // Get the pitch class (0-11) relative to C
    int absolutePitchClass = static_cast<int>(std::round(midiNote)) % 12;
    if (absolutePitchClass < 0)
      absolutePitchClass += 12;

    // Get pitch class relative to the key root
    int relativePitchClass = (absolutePitchClass - keyRoot + 12) % 12;

    // Find the nearest scale degree
    int nearestScaleInterval = 0;
    int minDistance = 12;

    for (int interval : scaleIntervals) {
      int distance = std::abs(relativePitchClass - interval);
      // Also check wrapping around the octave
      distance = std::min(distance, 12 - distance);

      if (distance < minDistance) {
        minDistance = distance;
        nearestScaleInterval = interval;
      }
    }

    // Calculate the target pitch class (absolute)
    int targetPitchClass = (keyRoot + nearestScaleInterval) % 12;

    // Find the nearest note with this pitch class to the original
    int roundedNote = static_cast<int>(std::round(midiNote));
    int currentPitchClass = roundedNote % 12;
    if (currentPitchClass < 0)
      currentPitchClass += 12;

    int diff = targetPitchClass - currentPitchClass;
    if (diff > 6)
      diff -= 12;
    if (diff < -6)
      diff += 12;

    return static_cast<float>(roundedNote + diff);
  }

  /**
   * Get the interval in semitones for a diatonic scale degree.
   *
   * @param scaleDegree Scale degree offset (-7 to +7)
   * @param scaleIntervals The scale intervals
   * @return Semitone offset
   */
  inline int diatonicToSemitones(int scaleDegree, const std::vector<int> &scaleIntervals) {
    int numNotesInScale = static_cast<int>(scaleIntervals.size());

    if (scaleDegree == 0)
      return 0;

    // Handle negative degrees (going down)
    if (scaleDegree < 0) {
      // Octaves down
      int octavesDown = (-scaleDegree) / numNotesInScale;
      int remainingDegrees = (-scaleDegree) % numNotesInScale;

      if (remainingDegrees == 0)
        return -12 * octavesDown;

      // Get the interval from the top of the scale
      int indexFromTop = numNotesInScale - remainingDegrees;
      return -(12 * (octavesDown + 1)) + scaleIntervals[indexFromTop];
    } else /*Positive degrees (going up)*/ {
      int octavesUp = scaleDegree / numNotesInScale;
      int remainingDegrees = scaleDegree % numNotesInScale;

      return 12 * octavesUp + scaleIntervals[remainingDegrees];
    }
  }

  //==========================================================================
  // SMOOTHING / FILTERING
  //==========================================================================

  /**
   * Calculate the coefficient for a one-pole lowpass filter (exponential smoothing).
   *
   * This is like the "alpha" in exponential moving average:
   * y[n] = y[n-1] + alpha * (x[n] - y[n-1])
   *
   * @param timeConstantMs Time constant in milliseconds (time to reach ~63% of target)
   * @param sampleRate Sample rate in Hz
   * @return Filter coefficient (0.0 to 1.0)
   */
  inline float calculateSmoothingCoeff(float timeConstantMs, double sampleRate) {
    if (timeConstantMs <= 0.0f)
      return 1.0f; // No smoothing

    // Time constant in samples
    float timeConstantSamples = (timeConstantMs / 1000.0f) * static_cast<float>(sampleRate);

    // One-pole coefficient: 1 - e^(-1/τ)
    return 1.0f - std::exp(-1.0f / timeConstantSamples);
  }

  /**
   * Apply one-pole lowpass filter (exponential smoothing).
   *
   * @param current Current smoothed value
   * @param target Target value
   * @param coeff Smoothing coefficient (0=no change, 1=instant)
   * @return New smoothed value
   */
  inline float smoothValue(float current, float target, float coeff) {
    return current + coeff * (target - coeff);
  }

  //==========================================================================
  // LEVEL / GAIN UTILITIES
  //==========================================================================

  /**
   * Convert decibels to linear gain.
   * 0 dB = 1.0, -6 dB ≈ 0.5, +6 dB ≈ 2.0
   */
  inline float dbToGain(float db) {
    return std::pow(10.0f, db / 20.0f);
  }

  /**
   * Convert linear gain to decibels.
   */
  inline float gainToDb(float gain) {
    if (gain <= 0.0f)
      return -100.0f; // Effectively -infinity
    return 20.0f * std::log10(gain);
  }

  /**
   * Constant-power pan law.
   *
   * When panning, naive linear panning causes the center to sound quieter.
   * Constant-power panning maintains perceived loudness across the stereo field.
   *
   * @param pan Pan position (-1.0 = left, 0.0 = center, +1.0 = right)
   * @param outLeft Output gain for left channel
   * @param outRight Output gain for right channel
   */
  inline void constantPowerPan(float pan, float &outLeft, float &outRight) {
    // Map pan (-1 to 1) to angle (0 to π/2)
    float angle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;

    outLeft = std::cos(angle);
    outRight = std::sin(angle);
  }

  //==========================================================================
  // BUFFER UTILITIES
  //==========================================================================

  /**
   * Wrap an index for circular buffer access.
   * Uses bitwise AND for power-of-2 buffer sizes (much faster than modulo).
   *
   * @param index The index to wrap
   * @param bufferSize Buffer size (must be power of 2)
   * @return Wrapped index (0 to bufferSize-1)
   */
  inline int wrapIndex(int index, int bufferSize) {
    // For power-of-2 sizes, this is equivalent to index % bufferSize
    // but much faster (single AND instruction vs division)
    return index & (bufferSize - 1);
  }

  /**
   * Linear interpolation between two values.
   *
   * @param a First value
   * @param b Second value
   * @param t Interpolation factor (0.0 = a, 1.0 = b)
   * @return Interpolated value
   */
  inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
  }

  /**
   * Cubic interpolation for higher quality sample interpolation.
   * Uses Catmull-Rom spline.
   *
   * @param y0 Sample before a
   * @param y1 Sample a
   * @param y2 Sample b
   * @param y3 Sample after b
   * @param t Interpolation factor between y1 and y2
   * @return Interpolated value
   */
  inline float cubicInterpolate(float y0, float y1, float y2, float y3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float a2 = -0.5f * y0 + 0.5f * y2;
    float a3 = y1;

    return a0 * t3 + a1 * t2 + a2 * t + a3;
  }

  //==========================================================================
  // WINDOWING FUNCTIONS
  //==========================================================================

  /**
   * Hann (Hanning) window function.
   *
   * Windows are used to smoothly fade in/out audio segments,
   * preventing clicks and improving frequency analysis accuracy.
   *
   * @param index Sample index (0 to windowSize-1)
   * @param windowSize Total window size
   * @return Window coefficient (0.0 to 1.0)
   */
  inline float hannWindow(int index, int windowSize) {
    return 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi *
                                   static_cast<float>(index) /
                                   static_cast<float>(windowSize - 1)));
  }

  /**
   * Pre-compute a Hann window into a buffer.
   *
   * @param buffer Output buffer for window values
   * @param size Window size
   */
  inline void fillHannWindow(std::vector<float> &buffer, int size) {
    buffer.resize(static_cast<size_t>(size));
    for (int i = 0; i < size; ++i)
    {
      buffer[static_cast<size_t>(i)] = hannWindow(i, size);
    }
  }

  //==========================================================================
  // RANDOM UTILITIES (for humanization)
  //==========================================================================

  /**
   * Generate a random float in a range using JUCE's Random class.
   * Thread-safe since each thread gets its own Random instance.
   */
  inline float randomFloat(float min, float max) {
    static juce::Random random;
    return min + random.nextFloat() * (max - min);
  }

} // namespace NovaTuneUtils
