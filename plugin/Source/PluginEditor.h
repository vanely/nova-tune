#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ParameterIDs.h"

/**
 * PluginEditor.h
 *
 * The plugin's graphical user interface.
 *
 * ANALOGY TO WEB DEVELOPMENT:
 * This is like a React component:
 * - It renders the UI (paint() is like render())
 * - It handles user interaction (mouse/keyboard events)
 * - It communicates with the "backend" (processor) via parameters
 * - Parameters with attachments are like controlled components with state
 *
 * UI LAYOUT FOR NOVATUNE:
 *
 * ┌──────────────────────────────────────────────────────────────────┐
 * │  NOVATUNE - Vocal Pitch Correction & Harmonizer                  │
 * ├──────────────────────────────────────────────────────────────────┤
 * │                                                                  │
 * │  ┌─────────────┐  ┌──────────────────────────────────────────┐  │
 * │  │   PITCH     │  │          MAIN CONTROLS                   │  │
 * │  │   DISPLAY   │  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐         │  │
 * │  │             │  │  │ Key │ │Scale│ │Input│ │Mode │         │  │
 * │  │  [===|===]  │  │  └─────┘ └─────┘ └─────┘ └─────┘         │  │
 * │  │             │  │                                          │  │
 * │  │  Detected:  │  │  ┌───────────┐  ┌───────┐  ┌───────┐     │  │
 * │  │    A4       │  │  │  RETUNE   │  │HUMANIZE│  │ MIX  │     │  │
 * │  │  Target:    │  │  │  SPEED    │  │       │  │      │     │  │
 * │  │    A4       │  │  │    ◯     │  │   ◯   │  │  ◯   │     │  │
 * │  └─────────────┘  │  └───────────┘  └───────┘  └───────┘     │  │
 * │                   └──────────────────────────────────────────┘  │
 * │                                                                  │
 * │  ┌────────────────────────────────────────────────────────────┐ │
 * │  │                    HARMONY VOICES                          │ │
 * │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │ │
 * │  │  │   VOICE A   │  │   VOICE B   │  │   VOICE C   │        │ │
 * │  │  │ [✓] Enabled │  │ [✓] Enabled │  │ [ ] Enabled │        │ │
 * │  │  │ Mode: Dia   │  │ Mode: Dia   │  │ Mode: Semi  │        │ │
 * │  │  │ Int: +3rd   │  │ Int: +5th   │  │ Int: +7     │        │ │
 * │  │  │ Level: -12  │  │ Level: -15  │  │ Level: -18  │        │ │
 * │  │  │ Pan: ←      │  │ Pan: →      │  │ Pan: ↔      │        │ │
 * │  │  └─────────────┘  └─────────────┘  └─────────────┘        │ │
 * │  └────────────────────────────────────────────────────────────┘ │
 * │                                                                  │
 * │  [ Preset: None ▼ ]                              [Bypass]       │
 * └──────────────────────────────────────────────────────────────────┘
 */

//==============================================================================
// CUSTOM LOOK AND FEEL
//==============================================================================

/**
 * Custom look and feel for NovaTune's modern dark theme.
 */
class NovaTuneLookAndFeel : public juce::LookAndFeel_V4 {
public:
  NovaTuneLookAndFeel();

  // Override slider drawing for custom knobs
  void drawRotarySlider(juce::Graphics &g,
                        int x, int y, int width, int height,
                        float sliderPos,
                        float rotaryStartAngle,
                        float rotaryEndAngle,
                        juce::Slider &slider) override;

  // Override combo box for custom dropdowns
  void drawComboBox(juce::Graphics &g,
                    int width, int height,
                    bool isButtonDown,
                    int buttonX, int buttonY,
                    int buttonW, int buttonH,
                    juce::ComboBox &box) override;

  // Custom toggle button
  void drawToggleButton(juce::Graphics &g,
                        juce::ToggleButton &button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

  // Colors
  static const juce::Colour backgroundColour;
  static const juce::Colour panelColour;
  static const juce::Colour accentColour;
  static const juce::Colour textColour;
  static const juce::Colour dimTextColour;
};

//==============================================================================
// PITCH DISPLAY COMPONENT
//==============================================================================

/**
 * Displays the detected pitch and correction status.
 */
class PitchDisplayComponent : public juce::Component,
                              public juce::Timer {
public:
  PitchDisplayComponent(NovaTuneAudioProcessor &processor);

  void paint(juce::Graphics &g) override;
  void timerCallback() override;

private:
  NovaTuneAudioProcessor &processor;

  float displayedPitch = 0.0f;
  float displayedTarget = 0.0f;
  float displayedCents = 0.0f;
  bool isVoiced = false;
};

//==============================================================================
// HARMONY VOICE PANEL
//==============================================================================

/**
 * Panel for controlling a single harmony voice.
 */
class HarmonyVoicePanel : public juce::Component {
public:
  HarmonyVoicePanel(NovaTuneAudioProcessor &processor, int voiceIndex);
  ~HarmonyVoicePanel() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  using APVTS = juce::AudioProcessorValueTreeState;
  using ButtonAttachment = APVTS::ButtonAttachment;
  using ComboBoxAttachment = APVTS::ComboBoxAttachment;
  using SliderAttachment = APVTS::SliderAttachment;

  NovaTuneAudioProcessor &processor;
  int voiceIndex;
  juce::String voiceName;

  // Controls
  juce::ToggleButton enabledButton;
  juce::ComboBox modeBox;
  juce::ComboBox intervalDiatonicBox;
  juce::Slider intervalSemiSlider;
  juce::Slider levelSlider;
  juce::Slider panSlider;
  juce::Slider formantSlider;

  // Labels
  juce::Label modeLabel;
  juce::Label intervalLabel;
  juce::Label levelLabel;
  juce::Label panLabel;
  juce::Label formantLabel;

  // Attachments
  std::unique_ptr<ButtonAttachment> enabledAttachment;
  std::unique_ptr<ComboBoxAttachment> modeAttachment;
  std::unique_ptr<ComboBoxAttachment> intervalDiatonicAttachment;
  std::unique_ptr<SliderAttachment> intervalSemiAttachment;
  std::unique_ptr<SliderAttachment> levelAttachment;
  std::unique_ptr<SliderAttachment> panAttachment;
  std::unique_ptr<SliderAttachment> formantAttachment;
};

//==============================================================================
// MAIN EDITOR
//==============================================================================

class NovaTuneAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  explicit NovaTuneAudioProcessorEditor(NovaTuneAudioProcessor &processor);
  ~NovaTuneAudioProcessorEditor() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  using APVTS = juce::AudioProcessorValueTreeState;
  using ComboBoxAttachment = APVTS::ComboBoxAttachment;
  using SliderAttachment = APVTS::SliderAttachment;
  using ButtonAttachment = APVTS::ButtonAttachment;

  NovaTuneAudioProcessor &processor;
  NovaTuneLookAndFeel lookAndFeel;

  //==========================================================================
  // MAIN CONTROLS
  //==========================================================================

  // Key and Scale selection
  juce::ComboBox keyBox;
  juce::ComboBox scaleBox;
  juce::ComboBox inputTypeBox;
  juce::ComboBox qualityModeBox;

  // Main knobs
  juce::Slider retuneSpeedSlider;
  juce::Slider humanizeSlider;
  juce::Slider mixSlider;

  // Labels
  juce::Label keyLabel;
  juce::Label scaleLabel;
  juce::Label inputTypeLabel;
  juce::Label qualityModeLabel;
  juce::Label retuneSpeedLabel;
  juce::Label humanizeLabel;
  juce::Label mixLabel;

  //==========================================================================
  // HARMONY SECTION
  //==========================================================================

  juce::ComboBox harmonyPresetBox;
  juce::Label harmonyPresetLabel;

  std::unique_ptr<HarmonyVoicePanel> voicePanelA;
  std::unique_ptr<HarmonyVoicePanel> voicePanelB;
  std::unique_ptr<HarmonyVoicePanel> voicePanelC;

  //==========================================================================
  // VISUALIZATIONS
  //==========================================================================

  PitchDisplayComponent pitchDisplay;

  //==========================================================================
  // GLOBAL CONTROLS
  //==========================================================================

  juce::ToggleButton bypassButton;

  //==========================================================================
  // ATTACHMENTS (connect UI to parameters)
  //==========================================================================

  std::unique_ptr<ComboBoxAttachment> keyAttachment;
  std::unique_ptr<ComboBoxAttachment> scaleAttachment;
  std::unique_ptr<ComboBoxAttachment> inputTypeAttachment;
  std::unique_ptr<ComboBoxAttachment> qualityModeAttachment;
  std::unique_ptr<ComboBoxAttachment> harmonyPresetAttachment;

  std::unique_ptr<SliderAttachment> retuneSpeedAttachment;
  std::unique_ptr<SliderAttachment> humanizeAttachment;
  std::unique_ptr<SliderAttachment> mixAttachment;

  std::unique_ptr<ButtonAttachment> bypassAttachment;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NovaTuneAudioProcessorEditor)
};
