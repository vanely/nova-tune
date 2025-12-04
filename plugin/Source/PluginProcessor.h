#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterIDs.h"
#include "dsp/TunerEngine.h"

/**
 * PluginProcessor.h
 *
 * The main plugin class - this is what the DAW (host) communicates with.
 *
 * ANALOGY TO WEB DEVELOPMENT:
 * Think of this like an Express.js server:
 * - The DAW is like a client making requests
 * - processBlock() is like handling incoming requests
 * - Parameters are like the server's state/configuration
 * - The Editor is like the frontend UI that talks to this backend
 *
 * PLUGIN LIFECYCLE:
 * 1. Host loads plugin → Constructor called
 * 2. Host configures audio → prepareToPlay() called
 * 3. Audio plays → processBlock() called repeatedly
 * 4. Audio stops → releaseResources() called
 * 5. Host unloads plugin → Destructor called
 *
 * THREAD SAFETY:
 * - processBlock() runs on the AUDIO THREAD (real-time, cannot block)
 * - Editor/UI runs on the MESSAGE THREAD (can block for UI updates)
 * - Parameters use atomic operations for thread-safe communication
 */

// Forward declaration to avoid circular includes
class NovaTuneAudioProcessorEditor;

class NovaTuneAudioProcessor : public juce::AudioProcessor {
public:
  //==========================================================================
  // TYPE ALIASES
  //==========================================================================

  /** Shorthand for the parameter state management class */
  using APVTS = juce::AudioProcessorValueTreeState;

  //==========================================================================
  // CONSTRUCTION / DESTRUCTION
  //==========================================================================

  NovaTuneAudioProcessor();
  ~NovaTuneAudioProcessor() override;

  //==========================================================================
  // AUDIO PROCESSING
  //==========================================================================

  /**
   * Called before audio processing starts.
   * Set up any resources that need the sample rate or buffer size.
   */
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;

  /**
   * Called when audio processing stops.
   * Release any resources that were allocated in prepareToPlay().
   */
  void releaseResources() override;

  /**
   * Check if a given bus layout is supported.
   * We support mono and stereo input/output.
   */
  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

  /**
   * Process a block of audio.
   * This is called repeatedly on the audio thread - must be fast!
   */
  void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) override;

  //==========================================================================
  // EDITOR (UI)
  //==========================================================================

  /**
   * Create the editor (UI) component.
   */
  juce::AudioProcessorEditor *createEditor() override;

  /**
   * Does this plugin have an editor?
   */
  bool hasEditor() const override;

  //==========================================================================
  // PLUGIN INFORMATION
  //==========================================================================

  /** Plugin name shown in DAW */
  const juce::String getName() const override;

  /** Does this plugin accept MIDI input? */
  bool acceptsMidi() const override;

  /** Does this plugin produce MIDI output? */
  bool producesMidi() const override;

  /** Is this a MIDI effect (MIDI in, MIDI out, no audio)? */
  bool isMidiEffect() const override;

  /** Get the length of the plugin's "tail" (reverb decay, etc.) in seconds */
  double getTailLengthSeconds() const override;

  //==========================================================================
  // PRESET / PROGRAM MANAGEMENT
  //==========================================================================

  /** How many presets does this plugin have? */
  int getNumPrograms() override;

  /** Get the current preset index */
  int getCurrentProgram() override;

  /** Set the current preset by index */
  void setCurrentProgram(int index) override;

  /** Get the name of a preset by index */
  const juce::String getProgramName(int index) override;

  /** Change the name of a preset */
  void changeProgramName(int index, const juce::String &newName) override;

  //==========================================================================
  // STATE MANAGEMENT (Preset Save/Load)
  //==========================================================================

  /**
   * Save the plugin state to a memory block.
   * Called when saving a project or preset.
   */
  void getStateInformation(juce::MemoryBlock &destData) override;

  /**
   * Load the plugin state from a memory block.
   * Called when loading a project or preset.
   */
  void setStateInformation(const void *data, int sizeInBytes) override;

  //==========================================================================
  // PARAMETER ACCESS
  //==========================================================================

  /**
   * Get the parameter state tree.
   * Used by the Editor to connect UI controls to parameters.
   */
  APVTS &getValueTreeState() { return apvts; }
  const APVTS &getValueTreeState() const { return apvts; }

  //==========================================================================
  // DSP ENGINE ACCESS (for UI visualization)
  //==========================================================================

  /**
   * Get the tuner engine for reading pitch detection results.
   * Used by the Editor for pitch visualization.
   */
  const TunerEngine &getTunerEngine() const { return tunerEngine; }

private:
  //==========================================================================
  // PARAMETER STATE
  //==========================================================================

  /**
   * AudioProcessorValueTreeState manages all parameters.
   * It handles:
   * - Thread-safe parameter access
   * - Automation recording/playback
   * - Parameter serialization for presets
   * - Undo/redo support
   */
  APVTS apvts;

  //==========================================================================
  // DSP ENGINE
  //==========================================================================

  /** The main DSP processing engine */
  TunerEngine tunerEngine;

  //==========================================================================
  // PREVENT COPYING
  //==========================================================================

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NovaTuneAudioProcessor)
};

//==============================================================================
// PARAMETER LAYOUT CREATION
//==============================================================================

/**
 * Create the parameter layout for the AudioProcessorValueTreeState.
 * This defines all the automatable parameters in the plugin.
 */
APVTS::ParameterLayout createParameterLayout();
