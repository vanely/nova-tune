#include "PluginEditor.h"

/**
 * PluginEditor.cpp
 *
 * Implementation of the NovaTune graphical user interface.
 */

//==============================================================================
// COLOURS
//==============================================================================

const juce::Colour NovaTuneLookAndFeel::backgroundColour = juce::Colour(0xFF1A1A2E);
const juce::Colour NovaTuneLookAndFeel::panelColour = juce::Colour(0xFF16213E);
const juce::Colour NovaTuneLookAndFeel::accentColour = juce::Colour(0xFF0F3460);
const juce::Colour NovaTuneLookAndFeel::textColour = juce::Colour(0xFFE94560);
const juce::Colour NovaTuneLookAndFeel::dimTextColour = juce::Colour(0xFF8B8B8B);

//==============================================================================
// CUSTOM LOOK AND FEEL
//==============================================================================

NovaTuneLookAndFeel::NovaTuneLookAndFeel() {
  // Set default colours
  setColour(juce::Slider::rotarySliderFillColourId, textColour);
  setColour(juce::Slider::rotarySliderOutlineColourId, accentColour);
  setColour(juce::Slider::thumbColourId, textColour);

  setColour(juce::ComboBox::backgroundColourId, panelColour);
  setColour(juce::ComboBox::textColourId, juce::Colours::white);
  setColour(juce::ComboBox::outlineColourId, accentColour);
  setColour(juce::ComboBox::arrowColourId, textColour);

  setColour(juce::PopupMenu::backgroundColourId, panelColour);
  setColour(juce::PopupMenu::textColourId, juce::Colours::white);
  setColour(juce::PopupMenu::highlightedBackgroundColourId, accentColour);
  setColour(juce::PopupMenu::highlightedTextColourId, textColour);

  setColour(juce::Label::textColourId, juce::Colours::white);

  setColour(juce::ToggleButton::textColourId, juce::Colours::white);
  setColour(juce::ToggleButton::tickColourId, textColour);
  setColour(juce::ToggleButton::tickDisabledColourId, dimTextColour);
}

void NovaTuneLookAndFeel::drawRotarySlider(juce::Graphics &g,
                                           int x, int y, int width, int height,
                                           float sliderPos,
                                           float rotaryStartAngle,
                                           float rotaryEndAngle,
                                           juce::Slider &slider) {
  auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(8);
  auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
  auto centreX = bounds.getCentreX();
  auto centreY = bounds.getCentreY();
  auto rx = centreX - radius;
  auto ry = centreY - radius;
  auto rw = radius * 2.0f;
  auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

  // Background arc
  juce::Path backgroundArc;
  backgroundArc.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                              rotaryStartAngle, rotaryEndAngle, true);
  g.setColour(accentColour);
  g.strokePath(backgroundArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

  // Value arc
  if (slider.isEnabled()) {
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                           rotaryStartAngle, angle, true);
    g.setColour(textColour);
    g.strokePath(valueArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
  }

  // Thumb
  juce::Path thumb;
  auto thumbWidth = 6.0f;
  thumb.addRectangle(-thumbWidth / 2, -radius, thumbWidth, radius * 0.4f);
  g.setColour(juce::Colours::white);
  g.fillPath(thumb, juce::AffineTransform::rotation(angle).translated(centreX, centreY));

  // Centre dot
  g.setColour(slider.isEnabled() ? textColour : dimTextColour);
  g.fillEllipse(centreX - 6.0f, centreY - 6.0f, 12.0f, 12.0f);
}

void NovaTuneLookAndFeel::drawComboBox(juce::Graphics &g,
                                       int width, int height,
                                       bool /*isButtonDown*/,
                                       int /*buttonX*/, int /*buttonY*/,
                                       int /*buttonW*/, int /*buttonH*/,
                                       juce::ComboBox &box) {
  auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();

  g.setColour(panelColour);
  g.fillRoundedRectangle(bounds, 4.0f);

  g.setColour(accentColour);
  g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

  // Draw arrow
  auto arrowZone = bounds.removeFromRight(30.0f).reduced(8.0f);
  juce::Path arrow;
  arrow.addTriangle(arrowZone.getX(), arrowZone.getY(),
                    arrowZone.getRight(), arrowZone.getY(),
                    arrowZone.getCentreX(), arrowZone.getBottom());
  g.setColour(box.isEnabled() ? textColour : dimTextColour);
  g.fillPath(arrow);
}

void NovaTuneLookAndFeel::drawToggleButton(juce::Graphics &g,
                                           juce::ToggleButton &button,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool /*shouldDrawButtonAsDown*/) {
  auto bounds = button.getLocalBounds().toFloat();
  auto boxSize = 20.0f;

  auto boxBounds = bounds.removeFromLeft(boxSize).reduced(2.0f);

  g.setColour(panelColour);
  g.fillRoundedRectangle(boxBounds, 3.0f);

  g.setColour(shouldDrawButtonAsHighlighted ? textColour : accentColour);
  g.drawRoundedRectangle(boxBounds, 3.0f, 1.0f);

  if (button.getToggleState()) {
    auto innerBounds = boxBounds.reduced(4.0f);
    g.setColour(textColour);
    g.fillRoundedRectangle(innerBounds, 2.0f);
  }

  g.setColour(button.isEnabled() ? juce::Colours::white : dimTextColour);
  g.setFont(14.0f);
  g.drawText(button.getButtonText(), bounds.reduced(4.0f, 0.0f),
             juce::Justification::centredLeft, true);
}

//==============================================================================
// PITCH DISPLAY COMPONENT
//==============================================================================

PitchDisplayComponent::PitchDisplayComponent(NovaTuneAudioProcessor &p)
    : processor(p) {
  startTimerHz(30); // Update 30 times per second
}

void PitchDisplayComponent::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Background
  g.setColour(NovaTuneLookAndFeel::panelColour);
  g.fillRoundedRectangle(bounds, 8.0f);

  // Title
  g.setColour(juce::Colours::white);
  g.setFont(14.0f);
  g.drawText("PITCH", bounds.removeFromTop(25.0f), juce::Justification::centred);

  // Pitch meter (cents display)
  auto meterBounds = bounds.removeFromTop(40.0f).reduced(10.0f, 5.0f);

  // Meter background
  g.setColour(NovaTuneLookAndFeel::accentColour);
  g.fillRoundedRectangle(meterBounds, 4.0f);

  // Center line
  g.setColour(juce::Colours::white.withAlpha(0.3f));
  g.fillRect(meterBounds.getCentreX() - 1.0f, meterBounds.getY(),
             2.0f, meterBounds.getHeight());

  // Pitch indicator
  if (isVoiced) {
    float normalizedCents = juce::jlimit(-50.0f, 50.0f, displayedCents) / 50.0f;
    float indicatorX = meterBounds.getCentreX() + normalizedCents * (meterBounds.getWidth() / 2.0f - 5.0f);

    g.setColour(NovaTuneLookAndFeel::textColour);
    g.fillEllipse(indicatorX - 5.0f, meterBounds.getCentreY() - 5.0f, 10.0f, 10.0f);
  }

  // Note display
  g.setColour(juce::Colours::white);
  g.setFont(16.0f);

  if (isVoiced) {
    int noteNum = static_cast<int>(std::round(displayedPitch));
    juce::String noteName = NovaTuneUtils::getMidiNoteName(noteNum);
    juce::String centsStr = juce::String(static_cast<int>(displayedCents));
    if (displayedCents >= 0)
      centsStr = "+" + centsStr;

    g.drawText("Detected: " + noteName, bounds.removeFromTop(20.0f),
               juce::Justification::centred);
    g.drawText(centsStr + " cents", bounds.removeFromTop(20.0f),
               juce::Justification::centred);

    int targetNum = static_cast<int>(std::round(displayedTarget));
    juce::String targetName = NovaTuneUtils::getMidiNoteName(targetNum);

    g.setColour(NovaTuneLookAndFeel::textColour);
    g.drawText("Target: " + targetName, bounds.removeFromTop(20.0f),
               juce::Justification::centred);
  } else {
    g.setColour(NovaTuneLookAndFeel::dimTextColour);
    g.drawText("No pitch detected", bounds.removeFromTop(40.0f),
               juce::Justification::centred);
  }
}

void PitchDisplayComponent::timerCallback() {
  const auto &detector = processor.getTunerEngine().getPitchDetector();
  const auto &mapper = processor.getTunerEngine().getPitchMapper();

  isVoiced = detector.isVoiced();

  if (isVoiced) {
    displayedPitch = detector.getMidiNote();

    const auto &result = mapper.getLastResult();
    displayedTarget = result.leadTargetMidiNote;
    displayedCents = result.centsOffTarget;
  }

  repaint();
}

//==============================================================================
// HARMONY VOICE PANEL
//==============================================================================

HarmonyVoicePanel::HarmonyVoicePanel(NovaTuneAudioProcessor &p, int index)
    : processor(p),
      voiceIndex(index) {
  // Set voice name
  switch (voiceIndex) {
    case 0:
      voiceName = "A";
      break;
    case 1:
      voiceName = "B";
      break;
    case 2:
      voiceName = "C";
      break;
    default:
      voiceName = "?";
      break;
  }

  auto &apvts = processor.getValueTreeState();

  // Get parameter IDs for this voice
  const char *enabledId = nullptr;
  const char *modeId = nullptr;
  const char *diaId = nullptr;
  const char *semiId = nullptr;
  const char *levelId = nullptr;
  const char *panId = nullptr;
  const char *formId = nullptr;

  switch (voiceIndex) {
    case 0:
      enabledId = ParamIDs::A_enabled;
      modeId = ParamIDs::A_mode;
      diaId = ParamIDs::A_intervalDiatonic;
      semiId = ParamIDs::A_intervalSemi;
      levelId = ParamIDs::A_level;
      panId = ParamIDs::A_pan;
      formId = ParamIDs::A_formantShift;
      break;
    case 1:
      enabledId = ParamIDs::B_enabled;
      modeId = ParamIDs::B_mode;
      diaId = ParamIDs::B_intervalDiatonic;
      semiId = ParamIDs::B_intervalSemi;
      levelId = ParamIDs::B_level;
      panId = ParamIDs::B_pan;
      formId = ParamIDs::B_formantShift;
      break;
    case 2:
      enabledId = ParamIDs::C_enabled;
      modeId = ParamIDs::C_mode;
      diaId = ParamIDs::C_intervalDiatonic;
      semiId = ParamIDs::C_intervalSemi;
      levelId = ParamIDs::C_level;
      panId = ParamIDs::C_pan;
      formId = ParamIDs::C_formantShift;
      break;
  }

  // Enabled button
  enabledButton.setButtonText("Voice " + voiceName);
  addAndMakeVisible(enabledButton);
  enabledAttachment = std::make_unique<ButtonAttachment>(apvts, enabledId, enabledButton);

  // Mode combo box
  modeBox.addItemList(NovaTuneEnums::getHarmonyModeNames(), 1);
  addAndMakeVisible(modeBox);
  modeAttachment = std::make_unique<ComboBoxAttachment>(apvts, modeId, modeBox);

  modeLabel.setText("Mode", juce::dontSendNotification);
  modeLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(modeLabel);

  // Interval combo box (diatonic)
  intervalDiatonicBox.addItemList(NovaTuneEnums::getDiatonicIntervalNames(), 1);
  addAndMakeVisible(intervalDiatonicBox);
  intervalDiatonicAttachment = std::make_unique<ComboBoxAttachment>(apvts, diaId, intervalDiatonicBox);

  // Interval slider (semitone)
  intervalSemiSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  intervalSemiSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
  addAndMakeVisible(intervalSemiSlider);
  intervalSemiAttachment = std::make_unique<SliderAttachment>(apvts, semiId, intervalSemiSlider);

  intervalLabel.setText("Interval", juce::dontSendNotification);
  intervalLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(intervalLabel);

  // Level slider
  levelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  levelSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
  addAndMakeVisible(levelSlider);
  levelAttachment = std::make_unique<SliderAttachment>(apvts, levelId, levelSlider);

  levelLabel.setText("Level", juce::dontSendNotification);
  levelLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(levelLabel);

  // Pan slider
  panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  panSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
  addAndMakeVisible(panSlider);
  panAttachment = std::make_unique<SliderAttachment>(apvts, panId, panSlider);

  panLabel.setText("Pan", juce::dontSendNotification);
  panLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(panLabel);

  // Formant slider
  formantSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  formantSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
  addAndMakeVisible(formantSlider);
  formantAttachment = std::make_unique<SliderAttachment>(apvts, formId, formantSlider);

  formantLabel.setText("Formant", juce::dontSendNotification);
  formantLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(formantLabel);
}

HarmonyVoicePanel::~HarmonyVoicePanel() = default;

void HarmonyVoicePanel::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  g.setColour(NovaTuneLookAndFeel::panelColour);
  g.fillRoundedRectangle(bounds, 8.0f);

  g.setColour(NovaTuneLookAndFeel::accentColour);
  g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);
}

void HarmonyVoicePanel::resized() {
  auto bounds = getLocalBounds().reduced(10);

  // Enabled button at top
  enabledButton.setBounds(bounds.removeFromTop(25));
  bounds.removeFromTop(5);

  // Mode and interval
  auto row1 = bounds.removeFromTop(50);
  auto modeArea = row1.removeFromLeft(row1.getWidth() / 2).reduced(2);
  auto intervalArea = row1.reduced(2);

  modeLabel.setBounds(modeArea.removeFromTop(18));
  modeBox.setBounds(modeArea);

  intervalLabel.setBounds(intervalArea.removeFromTop(18));
  intervalDiatonicBox.setBounds(intervalArea);

  bounds.removeFromTop(5);

  // Knobs
  auto knobRow = bounds.removeFromTop(80);
  int knobWidth = knobRow.getWidth() / 3;

  auto levelArea = knobRow.removeFromLeft(knobWidth).reduced(2);
  levelLabel.setBounds(levelArea.removeFromTop(15));
  levelSlider.setBounds(levelArea);

  auto panArea = knobRow.removeFromLeft(knobWidth).reduced(2);
  panLabel.setBounds(panArea.removeFromTop(15));
  panSlider.setBounds(panArea);

  auto formantArea = knobRow.reduced(2);
  formantLabel.setBounds(formantArea.removeFromTop(15));
  formantSlider.setBounds(formantArea);
}

//==============================================================================
// MAIN EDITOR
//==============================================================================

NovaTuneAudioProcessorEditor::NovaTuneAudioProcessorEditor(NovaTuneAudioProcessor &p)
    : AudioProcessorEditor(&p),
      processor(p),
      pitchDisplay(p) {
  setLookAndFeel(&lookAndFeel);

  auto &apvts = processor.getValueTreeState();

  //==========================================================================
  // KEY AND SCALE
  //==========================================================================

  keyBox.addItemList(NovaTuneEnums::getKeyNames(), 1);
  addAndMakeVisible(keyBox);
  keyAttachment = std::make_unique<ComboBoxAttachment>(apvts, ParamIDs::key, keyBox);

  keyLabel.setText("Key", juce::dontSendNotification);
  keyLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(keyLabel);

  scaleBox.addItemList(NovaTuneEnums::getScaleNames(), 1);
  addAndMakeVisible(scaleBox);
  scaleAttachment = std::make_unique<ComboBoxAttachment>(apvts, ParamIDs::scale, scaleBox);

  scaleLabel.setText("Scale", juce::dontSendNotification);
  scaleLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(scaleLabel);

  //==========================================================================
  // INPUT TYPE AND QUALITY
  //==========================================================================

  inputTypeBox.addItemList(NovaTuneEnums::getInputTypeNames(), 1);
  addAndMakeVisible(inputTypeBox);
  inputTypeAttachment = std::make_unique<ComboBoxAttachment>(apvts, ParamIDs::inputType, inputTypeBox);

  inputTypeLabel.setText("Input", juce::dontSendNotification);
  inputTypeLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(inputTypeLabel);

  qualityModeBox.addItemList(NovaTuneEnums::getQualityModeNames(), 1);
  addAndMakeVisible(qualityModeBox);
  qualityModeAttachment = std::make_unique<ComboBoxAttachment>(apvts, ParamIDs::qualityMode, qualityModeBox);

  qualityModeLabel.setText("Mode", juce::dontSendNotification);
  qualityModeLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(qualityModeLabel);

  //==========================================================================
  // MAIN KNOBS
  //==========================================================================

  retuneSpeedSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  retuneSpeedSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
  addAndMakeVisible(retuneSpeedSlider);
  retuneSpeedAttachment = std::make_unique<SliderAttachment>(apvts, ParamIDs::retuneSpeed, retuneSpeedSlider);

  retuneSpeedLabel.setText("Retune Speed", juce::dontSendNotification);
  retuneSpeedLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(retuneSpeedLabel);

  humanizeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  humanizeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
  addAndMakeVisible(humanizeSlider);
  humanizeAttachment = std::make_unique<SliderAttachment>(apvts, ParamIDs::humanize, humanizeSlider);

  humanizeLabel.setText("Humanize", juce::dontSendNotification);
  humanizeLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(humanizeLabel);

  mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
  addAndMakeVisible(mixSlider);
  mixAttachment = std::make_unique<SliderAttachment>(apvts, ParamIDs::mix, mixSlider);

  mixLabel.setText("Mix", juce::dontSendNotification);
  mixLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(mixLabel);

  //==========================================================================
  // HARMONY SECTION
  //==========================================================================

  harmonyPresetBox.addItemList(NovaTuneEnums::getHarmonyPresetNames(), 1);
  addAndMakeVisible(harmonyPresetBox);
  harmonyPresetAttachment = std::make_unique<ComboBoxAttachment>(apvts, ParamIDs::harmonyPreset, harmonyPresetBox);

  harmonyPresetLabel.setText("Harmony Preset", juce::dontSendNotification);
  harmonyPresetLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(harmonyPresetLabel);

  voicePanelA = std::make_unique<HarmonyVoicePanel>(processor, 0);
  voicePanelB = std::make_unique<HarmonyVoicePanel>(processor, 1);
  voicePanelC = std::make_unique<HarmonyVoicePanel>(processor, 2);

  addAndMakeVisible(*voicePanelA);
  addAndMakeVisible(*voicePanelB);
  addAndMakeVisible(*voicePanelC);

  //==========================================================================
  // PITCH DISPLAY
  //==========================================================================

  addAndMakeVisible(pitchDisplay);

  //==========================================================================
  // BYPASS
  //==========================================================================

  bypassButton.setButtonText("Bypass");
  addAndMakeVisible(bypassButton);
  bypassAttachment = std::make_unique<ButtonAttachment>(apvts, ParamIDs::bypass, bypassButton);

  //==========================================================================
  // WINDOW SIZE
  //==========================================================================

  setSize(700, 550);
}

NovaTuneAudioProcessorEditor::~NovaTuneAudioProcessorEditor() {
  setLookAndFeel(nullptr);
}

void NovaTuneAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(NovaTuneLookAndFeel::backgroundColour);

  // Title
  g.setColour(juce::Colours::white);
  g.setFont(24.0f);
  g.drawText("NOVATUNE", getLocalBounds().removeFromTop(40),
             juce::Justification::centred);

  g.setFont(12.0f);
  g.setColour(NovaTuneLookAndFeel::dimTextColour);
  g.drawText("Vocal Pitch Correction & Harmonizer",
             getLocalBounds().removeFromTop(55).removeFromBottom(15),
             juce::Justification::centred);
}

void NovaTuneAudioProcessorEditor::resized() {
  auto bounds = getLocalBounds().reduced(10);
  bounds.removeFromTop(50); // Space for title

  //==========================================================================
  // TOP SECTION: Pitch display + Controls
  //==========================================================================

  auto topSection = bounds.removeFromTop(200);

  // Pitch display on the left
  pitchDisplay.setBounds(topSection.removeFromLeft(150).reduced(5));

  // Controls on the right
  auto controlsArea = topSection.reduced(5);

  // Dropdowns row
  auto dropdownRow = controlsArea.removeFromTop(50);
  int dropdownWidth = dropdownRow.getWidth() / 4;

  auto keyArea = dropdownRow.removeFromLeft(dropdownWidth).reduced(5);
  keyLabel.setBounds(keyArea.removeFromTop(18));
  keyBox.setBounds(keyArea);

  auto scaleArea = dropdownRow.removeFromLeft(dropdownWidth).reduced(5);
  scaleLabel.setBounds(scaleArea.removeFromTop(18));
  scaleBox.setBounds(scaleArea);

  auto inputArea = dropdownRow.removeFromLeft(dropdownWidth).reduced(5);
  inputTypeLabel.setBounds(inputArea.removeFromTop(18));
  inputTypeBox.setBounds(inputArea);

  auto modeArea = dropdownRow.reduced(5);
  qualityModeLabel.setBounds(modeArea.removeFromTop(18));
  qualityModeBox.setBounds(modeArea);

  controlsArea.removeFromTop(10);

  // Main knobs row
  auto knobRow = controlsArea.removeFromTop(130);
  int knobWidth = knobRow.getWidth() / 3;

  auto retuneArea = knobRow.removeFromLeft(knobWidth).reduced(10);
  retuneSpeedLabel.setBounds(retuneArea.removeFromTop(20));
  retuneSpeedSlider.setBounds(retuneArea);

  auto humanizeArea = knobRow.removeFromLeft(knobWidth).reduced(10);
  humanizeLabel.setBounds(humanizeArea.removeFromTop(20));
  humanizeSlider.setBounds(humanizeArea);

  auto mixArea = knobRow.reduced(10);
  mixLabel.setBounds(mixArea.removeFromTop(20));
  mixSlider.setBounds(mixArea);

  bounds.removeFromTop(10);

  //==========================================================================
  // HARMONY SECTION
  //==========================================================================

  auto harmonySection = bounds.removeFromTop(220);

  // Preset selector
  auto presetRow = harmonySection.removeFromTop(30);
  harmonyPresetLabel.setBounds(presetRow.removeFromLeft(100));
  harmonyPresetBox.setBounds(presetRow.removeFromLeft(150));

  harmonySection.removeFromTop(5);

  // Voice panels
  auto voicesRow = harmonySection;
  int panelWidth = voicesRow.getWidth() / 3;

  voicePanelA->setBounds(voicesRow.removeFromLeft(panelWidth).reduced(5));
  voicePanelB->setBounds(voicesRow.removeFromLeft(panelWidth).reduced(5));
  voicePanelC->setBounds(voicesRow.reduced(5));

  //==========================================================================
  // BOTTOM: Bypass
  //==========================================================================

  auto bottomRow = bounds.removeFromBottom(30);
  bypassButton.setBounds(bottomRow.removeFromRight(100).reduced(5));
}
