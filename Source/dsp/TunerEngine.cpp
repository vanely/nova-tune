#include "TunerEngine.h"

/**
 * TunerEngine.cpp
 *
 * Implementation of the main DSP orchestrator.
 */

TunerEngine::TunerEngine() {
  // Components will be properly initialized in prepare()
}

void TunerEngine::prepare(double sr, int blockSize, int channels) {
  sampleRate = sr;
  samplesPerBlock = blockSize;
  numChannels = channels;

  // Prepare all DSP components
  pitchDetector.prepare(sampleRate, samplesPerBlock);
  pitchMapper.prepare(sampleRate);
  leadCorrection.prepare(sampleRate, samplesPerBlock, numChannels);

  for (auto &voice : harmonyVoices) {
    voice.prepare(sampleRate, samplesPerBlock, numChannels);
  }

  // Allocate internal buffers
  leadBuffer.setSize(numChannels, samplesPerBlock);
  harmonyBuffer.setSize(numChannels, samplesPerBlock);
  dryBuffer.setSize(numChannels, samplesPerBlock);

  reset();
}

void TunerEngine::reset() {
  pitchDetector.reset();
  pitchMapper.reset();
  leadCorrection.reset();

  for (auto &voice : harmonyVoices) {
    voice.reset();
  }

  leadBuffer.clear();
  harmonyBuffer.clear();
  dryBuffer.clear();
}

void TunerEngine::updateFromParameters(juce::AudioProcessorValueTreeState &apvts) {
  using namespace ParamIDs;

  // Update input type for pitch detector
  int inputTypeIndex = static_cast<int>(apvts.getRawParameterValue(inputType)->load());
  pitchDetector.setInputType(static_cast<NovaTuneEnums::InputType>(inputTypeIndex));

  // Update pitch mapper (key, scale, harmony intervals)
  pitchMapper.updateFromParameters(apvts);

  // Update lead correction (retune speed, humanize, vibrato, mix)
  leadCorrection.updateFromParameters(apvts);

  // Update each harmony voice
  for (int i = 0; i < DSPConfig::maxHarmonyVoices; ++i) {
    harmonyVoices[static_cast<size_t>(i)].updateFromParameters(i, apvts);
  }
}

void TunerEngine::process(juce::AudioBuffer<float> &buffer,
                          juce::MidiBuffer & /*midi*/,
                          juce::AudioProcessorValueTreeState &apvts) {
  const int numSamples = buffer.getNumSamples();

  // Handle block size changes at runtime (some DAWs do this)
  if (numSamples != samplesPerBlock) {
    samplesPerBlock = numSamples;
    leadBuffer.setSize(numChannels, samplesPerBlock, false, false, true);
    harmonyBuffer.setSize(numChannels, samplesPerBlock, false, false, true);
    dryBuffer.setSize(numChannels, samplesPerBlock, false, false, true);

    // Re-prepare components that need it
    leadCorrection.prepare(sampleRate, samplesPerBlock, numChannels);
    for (auto &voice : harmonyVoices) {
      voice.prepare(sampleRate, samplesPerBlock, numChannels);
    }
  }

  //==========================================================================
  // UPDATE PARAMETERS
  // Read the current parameter values from the thread-safe state
  //==========================================================================

  updateFromParameters(apvts);

  //==========================================================================
  // STORE DRY SIGNAL
  // Keep a copy for dry/wet mixing later
  //==========================================================================

  dryBuffer.makeCopyOf(buffer, true);

  //==========================================================================
  // STEP 1: PITCH DETECTION
  // Analyze the input to determine what note the singer is currently singing
  //==========================================================================

  pitchDetector.process(buffer);

  //==========================================================================
  // STEP 2: PITCH MAPPING
  // Determine the target note based on the detected pitch and selected key/scale
  //==========================================================================

  pitchMapper.process(pitchDetector);

  //==========================================================================
  // STEP 3: LEAD CORRECTION
  // Pitch-shift the lead vocal to the target note
  //==========================================================================

  // Copy input to lead buffer
  leadBuffer.makeCopyOf(buffer, true);

  // Apply pitch correction
  leadCorrection.process(leadBuffer, pitchDetector, pitchMapper);

  //==========================================================================
  // STEP 4: HARMONY GENERATION
  // Generate harmony voices from the corrected lead
  //==========================================================================

  // Clear harmony buffer (we'll accumulate into it)
  harmonyBuffer.clear();

  // Process each enabled harmony voice
  for (int i = 0; i < DSPConfig::maxHarmonyVoices; ++i) {
    harmonyVoices[static_cast<size_t>(i)].process(
        harmonyBuffer,
        leadBuffer, // Use corrected lead as source for tight harmonies
        pitchDetector,
        pitchMapper);
  }

  //==========================================================================
  // STEP 5: MIX OUTPUT
  // Combine corrected lead with harmony voices
  //==========================================================================

  // Start with the corrected lead
  buffer.makeCopyOf(leadBuffer, true);

  // Add harmony voices
  for (int ch = 0; ch < numChannels; ++ch) {
    buffer.addFrom(ch, 0, harmonyBuffer, ch, 0, numSamples);
  }

  //==========================================================================
  // STEP 6: SOFT CLIP / PROTECT
  // Prevent digital clipping when harmonies stack up
  //==========================================================================

  // Simple soft clipper using tanh
  // This prevents harsh digital distortion when all voices are loud
  for (int ch = 0; ch < numChannels; ++ch) {
    float *data = buffer.getWritePointer(ch);
    for (int i = 0; i < numSamples; ++i) {
      // Soft clip at approximately ±1.5 dB headroom
      data[i] = std::tanh(data[i] * 0.9f) / 0.9f;
    }
  }
}

int TunerEngine::getLatencySamples() const {
  // Total latency is the sum of all series components
  // (Parallel components don't add latency)

  int latency = 0;

  // Lead correction latency (includes pitch shifter)
  latency += leadCorrection.getLatencySamples();

  // Note: Pitch detection runs in parallel, doesn't add to output latency
  // Note: Harmony voices run in parallel with lead, so we take the max
  //       but since they're based on the same shifter, it's roughly equal

  return latency;
}
