# Hollywood Orchestrator Clone Implementation Plan

> **For Antigravity:** REQUIRED WORKFLOW: Use `.agent/workflows/execute-plan.md` to execute this plan in single-flow mode.

**Goal:** Build a native VST3 orchestral MIDI generator and arrangement engine for macOS and Cubase featuring intelligent chord recognition, multi-section voice-leading, step sequencing, universal articulation mapping, modal transformations, and timeline drag-and-drop.

**Architecture:** A high-performance C++20 audio/MIDI core executing within the VST3 audio thread, featuring an adaptive grace window for polyphonic key-strike debouncing, a rule-based chord classifier supporting 50+ chord types and slash chords, an acoustic-pyramid voice-leading engine, and a multi-track Standard MIDI File generator; paired with an ultra-responsive modern minimalist UI for sequencing, preset management, and drag-and-drop.

**Tech Stack:** C++20, JUCE Framework (AudioProcessor, VST3, AU, Standalone), CMake, Standard MIDI File (SMF Type 1/0) encoder, Modern HTML5/TypeScript/CSS UI layer.

---

### Task 1: Project Scaffolding & CMake Build Configuration

**Files:**
- Create: `CMakeLists.txt`
- Create: `src/Common/HarmonicTypes.h`
- Create: `tests/TestRunner.cpp`

**Step 1: Write test runner structure**
Create a standalone C++ test runner to verify core engine algorithms without requiring a DAW to be running.

**Step 2: Write CMakeLists.txt**
Configure CMake to build both the test executable and the plugin targets using C++20 standard, Apple Silicon (ARM64) and x86_64 architecture flags.

**Step 3: Verify build**
Run: `cmake -B build -S . && cmake --build build`
Expected: Successful compilation of test runner target.

**Step 4: Commit**
```bash
git add CMakeLists.txt src/Common/HarmonicTypes.h tests/TestRunner.cpp
git commit -m "chore: setup CMake build system and test harness"
```

---

### Task 2: Core Harmonic Types & Orchestral Data Models

**Files:**
- Create: `src/Common/HarmonicTypes.h`
- Test: `tests/TestHarmonicTypes.cpp`

**Step 1: Write test for Harmonic Types and conversions**
Validate pitch-class to note-name conversions, chord quality enumeration, and scale mode mappings.

**Step 2: Run test to verify it fails**
Run: `cmake --build build && ./build/orchestrator_tests --test=types`
Expected: Fails or not implemented.

**Step 3: Implement HarmonicTypes.h**
Define enums for `ChordQuality`, `ScaleMode`, `OrchestralSection`, `InstrumentId`, `ArticulationType`, `StepActionType`, and structs `HarmonicFrame`, `VoiceAssignment`, `OrchestralVoicing`.

**Step 4: Run test to verify it passes**
Run: `./build/orchestrator_tests --test=types`
Expected: PASS.

**Step 5: Commit**
```bash
git add src/Common/HarmonicTypes.h tests/TestHarmonicTypes.cpp
git commit -m "feat: implement harmonic types and data structures"
```

---

### Task 3: Adaptive Grace Window Buffer (Polyphonic Input Debounce)

**Files:**
- Create: `src/Harmonic/GraceWindowBuffer.h`
- Create: `src/Harmonic/GraceWindowBuffer.cpp`
- Test: `tests/TestGraceWindow.cpp`

**Step 1: Write failing test for Grace Window**
Simulate human finger latency: send NoteOn(C3) at t=0ms, NoteOn(E3) at t=8ms, NoteOn(G3) at t=19ms. Verify that the buffer does not prematurely emit single notes, but emits the aggregated triad when window expires (at t=25ms).

**Step 2: Run test to verify failure**
Run: `cmake --build build && ./build/orchestrator_tests --test=grace`
Expected: FAIL.

**Step 3: Implement GraceWindowBuffer**
Sample-accurate / millisecond buffer tracking incoming NoteOn and NoteOff events, CC64 (sustain pedal) status, and triggering the analysis callback when quiet interval elapses.

**Step 4: Run test to verify it passes**
Run: `./build/orchestrator_tests --test=grace`
Expected: PASS.

**Step 5: Commit**
```bash
git add src/Harmonic/GraceWindowBuffer.* tests/TestGraceWindow.cpp
git commit -m "feat: implement adaptive grace window buffer"
```

---

### Task 4: Harmonic Input Recognition & Chord Detector

**Files:**
- Create: `src/Harmonic/ChordDetector.h`
- Create: `src/Harmonic/ChordDetector.cpp`
- Test: `tests/TestChordDetector.cpp`

**Step 1: Write failing tests for 50+ chord types and slash chords**
Test triads (Maj, Min, Dim, Aug, Sus2, Sus4), 7ths (Dom7, Maj7, Min7, Dim7, Half-dim m7b5), extensions (9, Maj9, Min9, 11, #11, 13), power chords, single notes, and slash chords (e.g. C/E, G/B, Dm/F, Bb/C).

**Step 2: Run test to verify failure**
Run: `cmake --build build && ./build/orchestrator_tests --test=chords`
Expected: FAIL.

**Step 3: Implement ChordDetector**
- Extract absolute lowest note as `bassMidiNote`.
- Calculate pitch class set (modulo 12 bitmask / array).
- Match against chord taxonomy with priority scoring.
- Calculate chord root, inversion index, and tension extensions.
- Package into `HarmonicFrame`.

**Step 4: Run test to verify it passes**
Run: `./build/orchestrator_tests --test=chords`
Expected: PASS all chord and slash-chord test cases.

**Step 5: Commit**
```bash
git add src/Harmonic/ChordDetector.* tests/TestChordDetector.cpp
git commit -m "feat: implement harmonic chord detector with slash-chord analysis"
```

---

### Task 5: Modal Scale Transformation & Harmonic Quantizer

**Files:**
- Create: `src/Harmonic/ScaleQuantizer.h`
- Create: `src/Harmonic/ScaleQuantizer.cpp`
- Test: `tests/TestScaleQuantizer.cpp`

**Step 1: Write failing test for modal shifting**
Test modulating an input C Major chord into C Dorian, C Phrygian, C Lydian (#11), C Mixolydian, C Aeolian, C Harmonic Minor, C Whole-Tone, and C Octatonic.

**Step 2: Run test to verify failure**
Run: `cmake --build build && ./build/orchestrator_tests --test=modes`
Expected: FAIL.

**Step 3: Implement ScaleQuantizer**
Defines intervals for the 7 diatonic modes and synthetic cinematic scales; remaps chord members and scale step degrees dynamically to match the target scale while preserving acoustic voice spacing.

**Step 4: Run test to verify it passes**
Run: `./build/orchestrator_tests --test=modes`
Expected: PASS.

**Step 5: Commit**
```bash
git add src/Harmonic/ScaleQuantizer.* tests/TestScaleQuantizer.cpp
git commit -m "feat: implement modal transformation and scale quantizer"
```

---

### Task 6: Orchestral Tessituras, Voicing & Voice-Leading Engine

**Files:**
- Create: `src/Orchestration/OrchestralTessituras.h`
- Create: `src/Orchestration/VoicingEngine.h`
- Create: `src/Orchestration/VoicingEngine.cpp`
- Test: `tests/TestVoicingEngine.cpp`

**Step 1: Write failing test for orchestral voice distribution**
Verify that a 4-note chord (e.g. C-E-G-B) is distributed properly across Strings (Bass, Celli, Violas, Violins 2, Violins 1), Brass, and Woodwinds within their proper realistic registers, and that moving from Cmaj to Amin exhibits minimal distance displacement (voice-leading continuity).

**Step 2: Run test to verify failure**
Run: `cmake --build build && ./build/orchestrator_tests --test=voicing`
Expected: FAIL.

**Step 3: Implement VoicingEngine**
- Define register boundaries (min, max, sweet spot) for 14 orchestral instruments.
- Implement voicing styles: Acoustic Pyramid (bass octave doublings, open lower-mid, dense upper), Drop-2, Drop-4, and Smart Divisi.
- Implement least-action voice-leading algorithm comparing current pitches with previous frame to minimize melodic leaps.

**Step 4: Run test to verify it passes**
Run: `./build/orchestrator_tests --test=voicing`
Expected: PASS.

**Step 5: Commit**
```bash
git add src/Orchestration/* tests/TestVoicingEngine.cpp
git commit -m "feat: implement orchestral voicing engine with voice leading"
```

---

### Task 7: Harmonic Randomizer & Humanizer

**Files:**
- Create: `src/Orchestration/HarmonicRandomizer.h`
- Create: `src/Orchestration/HarmonicRandomizer.cpp`
- Test: `tests/TestRandomizer.cpp`

**Step 1: Write failing test for controlled randomizer**
Verify inversion jitter changes inversion musically, tension inject adds allowed modal degrees (9, 11), and humanize applies subtle timing and velocity jitter within bounded constraints.

**Step 2: Run test to verify failure**
Run: `cmake --build build && ./build/orchestrator_tests --test=randomizer`
Expected: FAIL.

**Step 3: Implement HarmonicRandomizer**
Parameters: `inversionJitterRate`, `extensionChance`, `timingJitterMs`, `velocityJitterPercent`. Produces varied but harmonically coherent musical output.

**Step 4: Run test to verify it passes**
Run: `./build/orchestrator_tests --test=randomizer`
Expected: PASS.

**Step 5: Commit**
```bash
git add src/Orchestration/HarmonicRandomizer.* tests/TestRandomizer.cpp
git commit -m "feat: implement harmonic randomizer and humanizer"
```

---

### Task 8: Pattern Sequencer & Host Transport Sync

**Files:**
- Create: `src/Sequencer/PatternModel.h`
- Create: `src/Sequencer/PatternModel.cpp`
- Create: `src/Sequencer/SequencerEngine.h`
- Create: `src/Sequencer/SequencerEngine.cpp`
- Test: `tests/TestSequencerEngine.cpp`

**Step 1: Write failing test for step sequencing**
Test 16-step Ostinato, Sustain, Arpeggio, and Rest step actions. Verify sample-accurate step progression given BPM, time signature, and sample position.

**Step 2: Run test to verify failure**
Run: `cmake --build build && ./build/orchestrator_tests --test=sequencer`
Expected: FAIL.

**Step 3: Implement PatternModel and SequencerEngine**
- Stores per-instrument lanes with step actions (`Sustain`, `Ostinato`, `ArpUp`, `ArpDown`, `Runs`, `Rest`), velocity (0-127), gate (10-100%), octave offset (-2 to +2), and articulation.
- Evaluates transport ticks, tracks PPQ position, and schedules NoteOn/NoteOff events.

**Step 4: Run test to verify it passes**
Run: `./build/orchestrator_tests --test=sequencer`
Expected: PASS.

**Step 5: Commit**
```bash
git add src/Sequencer/* tests/TestSequencerEngine.cpp
git commit -m "feat: implement multi-track pattern sequencer engine"
```

---

### Task 9: Universal Articulation Mapping & Library Presets

**Files:**
- Create: `src/Articulation/ArticulationMap.h`
- Create: `src/Articulation/ArticulationMap.cpp`
- Create: `src/Articulation/LibraryPresets.h`
- Test: `tests/TestArticulationMap.cpp`

**Step 1: Write failing test for articulation mapping**
Verify mapping `Staccato` for Cinematic Studio Strings triggers CC58=20; Spitfire BBCSO triggers UACC CC32=42 or Keyswitch C-1; EastWest OPUS triggers configured keyswitch.

**Step 2: Run test to verify failure**
Run: `cmake --build build && ./build/orchestrator_tests --test=articulations`
Expected: FAIL.

**Step 3: Implement ArticulationMap and LibraryPresets**
Supports Keyswitches (NoteOn pitch + velocity + pre-delay), CC controller values (e.g. CC58, CC32), Program Change, and channel routing. Includes out-of-the-box profiles for CSS, Spitfire, EW Opus, VSL, Kontakt.

**Step 4: Run test to verify it passes**
Run: `./build/orchestrator_tests --test=articulations`
Expected: PASS.

**Step 5: Commit**
```bash
git add src/Articulation/* tests/TestArticulationMap.cpp
git commit -m "feat: implement universal articulation mapping and library presets"
```

---

### Task 10: Standard MIDI File (SMF) Generator for Drag & Drop

**Files:**
- Create: `src/MidiExport/StandardMidiWriter.h`
- Create: `src/MidiExport/StandardMidiWriter.cpp`
- Test: `tests/TestMidiWriter.cpp`

**Step 1: Write failing test for MIDI file export**
Verify generated file has valid SMF headers (`MThd`, `MTrk`), correct delta-times, track names ("Violins 1", "Celli", "Horns"), MIDI channels (1-16), tempo events, and note events. Verify both Type 1 (multi-track master) and Type 0 (single stem) formats.

**Step 2: Run test to verify failure**
Run: `cmake --build build && ./build/orchestrator_tests --test=midi_writer`
Expected: FAIL.

**Step 3: Implement StandardMidiWriter**
Pure C++ compliant binary SMF encoder generating valid `.mid` files written to a temporary directory for instant dragging into Cubase.

**Step 4: Run test to verify it passes**
Run: `./build/orchestrator_tests --test=midi_writer`
Expected: PASS.

**Step 5: Commit**
```bash
git add src/MidiExport/* tests/TestMidiWriter.cpp
git commit -m "feat: implement Standard MIDI File Type 1/0 generator"
```

---

### Task 11: VST3 Plugin Processor & 16-Channel MIDI Dispatcher

**Files:**
- Create: `src/Plugin/PluginProcessor.h`
- Create: `src/Plugin/PluginProcessor.cpp`
- Test: `tests/TestPluginProcessor.cpp`

**Step 1: Write failing test for PluginProcessor MIDI pipeline**
Send incoming MIDI chord events to `processBlock()`, verify output `MidiBuffer` contains dispatched notes on designated channels (Ch 1-5 for Strings, 6-9 for Brass, 10-13 for Woodwinds, 14-16 for Percussion).

**Step 2: Run test to verify failure**
Run: `cmake --build build && ./build/orchestrator_tests --test=processor`
Expected: FAIL.

**Step 3: Implement PluginProcessor**
Connects `GraceWindowBuffer`, `ChordDetector`, `VoicingEngine`, `SequencerEngine`, and `ArticulationMap` to JUCE `AudioProcessor`. Zero allocation in `processBlock()`.

**Step 4: Run test to verify it passes**
Run: `./build/orchestrator_tests --test=processor`
Expected: PASS.

**Step 5: Commit**
```bash
git add src/Plugin/* tests/TestPluginProcessor.cpp
git commit -m "feat: implement VST3 plugin processor with 16-channel MIDI dispatching"
```

---

### Task 12: Modern Minimalist UI & Drag-and-Drop Handles

**Files:**
- Create: `ui/index.html`
- Create: `ui/css/style.css`
- Create: `ui/js/app.js`
- Create: `src/Plugin/PluginEditor.h`
- Create: `src/Plugin/PluginEditor.cpp`

**Step 1: Build the Modern Minimalist UI**
- Dark cinematic aesthetic with subtle glowing accents.
- Header: Real-time Chord Badge (`C min 9 / Eb`), active Mode/Scale, Transport, Master Drag & Drop handle.
- Section Matrix: Strings, Brass, Woodwinds, Percussion lanes with Mute/Solo, MIDI Ch (1-16), Articulation selectors, and Stem Drag buttons.
- Interactive Step Sequencer: 16-step grid with visual velocity bars and articulation tags.
- Modal & Randomizer Panel: Scale/Mode dropdown, Inversion Jitter slider, Humanize sliders.
- Preset Browser: Action Ostinato, Lyrical Adagio, Epic Fanfare, Suspense Mystery.

**Step 2: Connect UI to PluginProcessor**
Implement bidirectionally synced parameters via JSON bridge / lock-free IPC queue.

**Step 3: Verify UI rendering & responsiveness**
Test UI layout and event dispatching.

**Step 4: Commit**
```bash
git add ui/* src/Plugin/PluginEditor.*
git commit -m "feat: implement modern minimalist UI and drag-and-drop handles"
```

---

### Task 13: End-to-End Orchestrator Verification Suite & Presets

**Files:**
- Create: `tests/TestIntegrationE2E.cpp`
- Create: `presets/styles/action_ostinato.json`
- Create: `presets/styles/epic_brass_fanfare.json`
- Create: `presets/styles/lyrical_strings.json`
- Create: `presets/styles/suspense_mystery.json`

**Step 1: Write comprehensive end-to-end integration test**
Simulate complete composer workflow:
1. Input keyboard plays `C - Eb - G - Bb - D` (Cm9) with finger delay.
2. Grace window aggregates and detects `Cm9` root C.
3. Modal transformation shifts to `Dorian` (#6 = A natural).
4. Voicing engine spreads harmony across Strings, Brass, and Woodwinds in valid tessituras with acoustic pyramid.
5. Sequencer generates 16-step Action Ostinato with spiccato articulation CC triggers.
6. MIDI Writer generates valid Type 1 multi-track `.mid` ready for Cubase timeline drag.

**Step 2: Run test suite**
Run: `cmake --build build && ./build/orchestrator_tests --test=all`
Expected: ALL TESTS PASS with 100% success rate.

**Step 3: Commit**
```bash
git add tests/TestIntegrationE2E.cpp presets/*
git commit -m "test: add end-to-end verification suite and style presets"
```
