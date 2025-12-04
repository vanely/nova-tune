# NovaTune Development TODO Checklist

This document tracks all remaining work for NovaTune MVP and future phases.

---

## 🔴 Priority 1: Core DSP Completion

These are the `TODO` items left in the code that need implementation for a fully functional plugin.

### Pitch Detection Enhancements
- [ ] **Implement proper YIN threshold tuning** (`PitchDetector.cpp`)
  - Current threshold is static (0.15)
  - Add adaptive threshold based on signal energy
  - Test with different voice types and adjust ranges

- [ ] **Add pitch detection confidence smoothing**
  - Implement hysteresis to prevent rapid voiced/unvoiced switching
  - Add minimum duration for pitch changes (avoid glitches on fast passages)

- [ ] **Optimize pitch detection for low latency**
  - Experiment with smaller frame sizes for Live Mode
  - Implement running autocorrelation to reduce per-block computation

### Pitch Shifting Improvements
- [ ] **Tune WSOLA parameters for vocal quality**
  - Experiment with window sizes (20-50ms range)
  - Adjust overlap factor for best quality/CPU tradeoff
  - Test search range for waveform similarity matching

- [ ] **Add PSOLA option for pitched signals**
  - Implement pitch-synchronous grain extraction
  - Use detected pitch period to align grains
  - Compare quality vs. WSOLA

- [ ] **Implement proper crossfade windows**
  - Test different window shapes (Hann, Hamming, Blackman)
  - Optimize for minimal artifacts at grain boundaries

### Formant Processing
- [ ] **Improve formant preservation algorithm**
  - Current filter bank approach is basic
  - Research LPC (Linear Predictive Coding) implementation
  - Consider cepstral-based formant estimation

- [ ] **Add formant detection for adaptive correction**
  - Track F1/F2 formants in real-time
  - Adjust formant shift based on detected formants vs. expected

### Vibrato Handling
- [ ] **Implement vibrato detection** (`LeadCorrection.cpp`)
  - Detect vibrato rate (typically 4-7 Hz)
  - Detect vibrato depth (typically ±30-100 cents)
  - Track vibrato phase

- [ ] **Add vibrato preservation modes**
  - "Natural": Preserve detected vibrato fully
  - "Tighten": Reduce vibrato depth while keeping rate
  - "Remove": Flatten vibrato completely
  - "Enhance": Increase vibrato depth

- [ ] **Implement vibrato bypass during correction**
  - Don't correct pitch during vibrato peaks
  - Only correct the "center" pitch of vibrato

---

## 🟠 Priority 2: Testing & Validation

### DAW Compatibility Testing
- [ ] **Ableton Live** (Windows & macOS)
  - [ ] Plugin loads correctly
  - [ ] Parameters automate properly
  - [ ] Preset save/load works
  - [ ] Latency compensation correct
  - [ ] CPU usage acceptable

- [ ] **FL Studio** (Windows)
  - [ ] Same tests as above

- [ ] **Logic Pro** (macOS)
  - [ ] AU format works
  - [ ] Same tests as above

- [ ] **Pro Tools** (Phase 2 - AAX)
  - [ ] Deferred until AAX implementation

- [ ] **Reaper** (Windows & macOS)
  - [ ] Same tests as above

- [ ] **Studio One** (Windows & macOS)
  - [ ] Same tests as above

### Audio Quality Testing
- [ ] **Test with various vocal types**
  - [ ] Female soprano
  - [ ] Female alto
  - [ ] Male tenor
  - [ ] Male baritone/bass
  - [ ] Child voice

- [ ] **Test edge cases**
  - [ ] Fast melodic runs (16th notes at 120 BPM)
  - [ ] Large pitch jumps (octave+)
  - [ ] Whispered/breathy vocals
  - [ ] Vocals with heavy vibrato
  - [ ] Sibilants (s, t, ch sounds)
  - [ ] Silence to note transitions
  - [ ] Note to silence transitions

- [ ] **A/B comparison with competitors**
  - [ ] Compare to Antares Auto-Tune
  - [ ] Compare to Waves Tune Real-Time
  - [ ] Compare to Melodyne (offline reference)

### Performance Testing
- [ ] **Latency measurements**
  - [ ] Measure actual round-trip latency at 44.1kHz
  - [ ] Measure at 48kHz, 88.2kHz, 96kHz
  - [ ] Test at buffer sizes: 32, 64, 128, 256, 512, 1024

- [ ] **CPU usage profiling**
  - [ ] Profile with single instance
  - [ ] Test with 4, 8, 16 instances
  - [ ] Identify hotspots and optimize

- [ ] **Memory usage**
  - [ ] Measure memory footprint
  - [ ] Check for memory leaks (Valgrind/Instruments)

---

## 🟡 Priority 3: UI/UX Improvements

### Visual Enhancements
- [ ] **Improve pitch visualization**
  - [ ] Add scrolling pitch graph (like Auto-Tune's)
  - [ ] Show correction amount visually
  - [ ] Color-code: green=in tune, red=correcting

- [ ] **Add input/output level meters**
  - [ ] Peak meters with hold
  - [ ] RMS meters option
  - [ ] Clip indicators

- [ ] **Keyboard visualization**
  - [ ] Show detected note on piano keyboard
  - [ ] Highlight scale notes
  - [ ] Show harmony notes

- [ ] **Improve overall UI polish**
  - [ ] Add tooltips to all controls
  - [ ] Add value readouts on hover
  - [ ] Smooth animations for meters

### Usability Improvements
- [ ] **Add undo/redo support**
  - [ ] Integrate with JUCE UndoManager
  - [ ] Test with DAW undo systems

- [ ] **Improve preset system**
  - [ ] Add preset categories (Subtle, Pop, Trap, etc.)
  - [ ] Add A/B comparison
  - [ ] Add "randomize" for experimentation

- [ ] **Add MIDI input option (Phase 2)**
  - [ ] Accept MIDI notes for target pitch
  - [ ] Accept MIDI for key/scale changes

### Accessibility
- [ ] **Keyboard navigation**
  - [ ] All controls accessible via keyboard
  - [ ] Tab order logical

- [ ] **Screen reader support**
  - [ ] Add accessibility labels
  - [ ] Test with VoiceOver (macOS)

---

## 🟢 Priority 4: Feature Additions

### Harmony Preset System
- [ ] **Implement harmony preset application**
  - Currently presets are defined but don't auto-configure voices
  - [ ] "Pop 3rd Up" → Enable Voice A, set to +3rd diatonic
  - [ ] "Pop 3rd & 5th" → Enable A (+3rd) and B (+5th)
  - [ ] Implement all 8 presets from spec

- [ ] **Add user preset saving for harmonies**
  - [ ] Save current harmony config as preset
  - [ ] Load/rename/delete user presets

### Advanced Correction Features
- [ ] **Add note bypass (scale note exclusion)**
  - [ ] Allow user to exclude specific notes from correction
  - [ ] Useful for blues notes, passing tones

- [ ] **Add pitch bend/glide control**
  - [ ] Preserve natural glides between notes
  - [ ] Adjustable glide time

- [ ] **Add throat/gender control**
  - [ ] More sophisticated formant shifting
  - [ ] "Throat length" modeling

### Input Analysis
- [ ] **Add automatic key detection**
  - [ ] Analyze incoming audio for probable key
  - [ ] Suggest key/scale to user
  - [ ] Optional auto-follow mode

- [ ] **Add reference track input**
  - [ ] Sidechain input for pitch reference
  - [ ] Useful for doubling/matching another vocal

---

## 🔵 Priority 5: Performance Optimization

### DSP Optimization
- [ ] **Add SIMD optimization**
  - [ ] Vectorize pitch detection correlation loops
  - [ ] Vectorize WSOLA overlap-add
  - [ ] Use JUCE's SIMD helpers or manual intrinsics

- [ ] **Optimize memory access patterns**
  - [ ] Ensure cache-friendly buffer layouts
  - [ ] Minimize cache misses in hot loops

- [ ] **Profile and optimize hotspots**
  - [ ] Use profiler to identify slowest functions
  - [ ] Optimize top 3-5 hotspots

### Build Optimization
- [ ] **Enable Link-Time Optimization (LTO)**
  - [ ] Already in CMakeLists.txt, verify it's working

- [ ] **Test Release vs Debug performance**
  - [ ] Ensure debug builds don't ship

- [ ] **Reduce binary size**
  - [ ] Strip unused code
  - [ ] Consider splitting large functions

---

## 🟣 Phase 2 Features (Post-MVP)

### Graph/Editor Mode
- [ ] **Design offline pitch editing UI**
  - [ ] Pitch curve visualization
  - [ ] Note block editing (like Melodyne)
  - [ ] Time selection tools

- [ ] **Implement offline pitch manipulation**
  - [ ] Store detected pitch data
  - [ ] Allow manual note adjustments
  - [ ] Render changes to audio

### ARA2 Integration
- [ ] **Research ARA2 SDK**
  - [ ] Understand ARA2 architecture
  - [ ] Plan integration points

- [ ] **Implement ARA2 plugin variant**
  - [ ] ARA document controller
  - [ ] Audio modification interface
  - [ ] Integration with Logic, Studio One, etc.

### AAX Format
- [ ] **Set up AAX SDK**
  - [ ] Apply for Avid developer account
  - [ ] Integrate AAX wrapper

- [ ] **Test in Pro Tools**
  - [ ] Standard testing suite

### Advanced Features
- [ ] **Multi-voice polyphonic detection**
  - [ ] Research polyphonic pitch detection
  - [ ] Implement for instrument mode

- [ ] **Machine learning pitch estimation**
  - [ ] Research CREPE, SPICE, or similar
  - [ ] Evaluate latency/accuracy tradeoffs
  - [ ] Optional ML mode for higher quality

---

## 📋 Documentation & Release

### Documentation
- [ ] **Write user manual**
  - [ ] Getting started guide
  - [ ] Parameter reference
  - [ ] Troubleshooting

- [ ] **Create tutorial videos**
  - [ ] Basic pitch correction workflow
  - [ ] Harmony creation tutorial
  - [ ] Tips for natural vs. robotic sound

### Release Preparation
- [ ] **Code signing**
  - [ ] macOS: Apple Developer certificate
  - [ ] Windows: Code signing certificate

- [ ] **Installer creation**
  - [ ] macOS: PKG installer
  - [ ] Windows: NSIS or WiX installer

- [ ] **Beta testing program**
  - [ ] Recruit beta testers
  - [ ] Set up feedback collection
  - [ ] Bug tracking system

---

## 🐛 Known Issues / Bugs

Track bugs here as they're discovered:

- [ ] _No known bugs yet - add as discovered_

---

## 📊 Progress Tracking

| Category | Total | Complete | Remaining |
|----------|-------|----------|-----------|
| Priority 1: Core DSP | 15 | 0 | 15 |
| Priority 2: Testing | 25 | 0 | 25 |
| Priority 3: UI/UX | 15 | 0 | 15 |
| Priority 4: Features | 12 | 0 | 12 |
| Priority 5: Optimization | 8 | 0 | 8 |
| Phase 2 | 10 | 0 | 10 |
| Documentation | 6 | 0 | 6 |
| **TOTAL** | **91** | **0** | **91** |

---

## 📝 Notes

_Add development notes, decisions, and learnings here:_

- 

---

*Last updated: [DATE]*
