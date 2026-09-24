# Design Document: Hollywood Orchestrator Clone (VST3/macOS)

**Date:** 2026-09-24  
**Author:** Antigravity  
**Target:** macOS (Apple Silicon ARM64 & Intel x86_64), Steinberg Cubase (VST3 & AU), Universal Orchestral Libraries

---

## 1. Overview & Vision
A native VST3 MIDI Instrument/FX plugin designed as a universal orchestral arrangement engine inspired by EastWest Hollywood Orchestrator. 

Unlike the closed proprietary original, this engine acts as an open **MIDI Source & Multiplexer**, receiving chord/note inputs from a MIDI keyboard or DAW track and real-time orchestrating them across 16 MIDI channels for any sample library (Cinematic Studio Series, Spitfire BBCSO, EastWest OPUS, VSL, Kontakt, etc.). It features full articulation mapping, intelligent chord/slash-chord recognition, dynamic voice-leading, pattern sequencing, modal transformation/randomization, and native timeline drag-and-drop.

---

## 2. Architecture & Tech Stack

```
                        +---------------------------------------+
                        |       MIDI Keyboard / DAW Track       |
                        +---------------------------------------+
                                            |
                                            v (Raw MIDI Events)
+-------------------------------------------------------------------------------------+
|                      HOLLYWOOD ORCHESTRATOR CLONE (C++20 / JUCE)                     |
|                                                                                     |
|   +-----------------------------------------------------------------------------+   |
|   | 1. Input Processing Layer                                                   |   |
|   |    - Adaptive Grace Window Buffer (10-40ms sample-accurate debounce)        |   |
|   |    - Bass Note Extractor (Slash Chords) & Pitch Class Normalizer            |   |
|   |    - Tonal Center & Chord Quality Classifier (50+ chord structures)         |   |
|   +-----------------------------------------------------------------------------+   |
|                                          |                                          |
|                                          v (Emits HarmonicFrame)                    |
|   +-----------------------------------------------------------------------------+   |
|   | 2. Orchestration & Voicing Engine                                           |   |
|   |    - Acoustic Pyramid, Drop-2, Drop-4, and Divisi algorithms                |   |
|   |    - Tessitura / Register limiters per instrument section                   |   |
|   |    - Voice Leading Minimizer (prevents unnatural register jumps)            |   |
|   +-----------------------------------------------------------------------------+   |
|                                          |                                          |
|                                          v (Allocated Orchestral Pitches)           |
|   +-----------------------------------------------------------------------------+   |
|   | 3. Step Sequencer & Harmonic Modulator                                      |   |
|   |    - Host Sync via AudioPlayHead (BPM, Meter, PPQ) & Free-Play mode         |   |
|   |    - Per-section lanes: Sustain, Ostinato, Arpeggio, Rest, Runs             |   |
|   |    - Real-time Modal Shifter (Dorian, Phrygian, Lydian, Whole-Tone, etc.)   |   |
|   |    - Musical Randomizer (Inversion Jitter, Extensions, Humanize)            |   |
|   +-----------------------------------------------------------------------------+   |
|                                          |                                          |
|                                          v (Scheduled Note & Articulation Events)   |
|   +-----------------------------------------------------------------------------+   |
|   | 4. Universal Articulation & Output Layer                                    |   |
|   |    - Target Profile Translator (Keyswitches, CC58, UACC CC32, Program Ch.)  |   |
|   |    - 16-Channel MIDI Dispatcher to Cubase Instrument Tracks                 |   |
|   |    - Standard MIDI File Generator (.mid Type 1 Multi-track & Type 0 Stem)   |   |
|   +-----------------------------------------------------------------------------+   |
|                                          |                                          |
+------------------------------------------|------------------------------------------+
                                           |
            +------------------------------+------------------------------+
            | (Live Performance Stream)                                   | (Drag & Drop Export)
            v                                                             v
+-------------------------------+                             +------------------------+
| Cubase Multi-Track MIDI In    |                             | Cubase Timeline Tracks |
| Ch 1-5:  Strings Section      |                             | Drag master or stem    |
| Ch 6-9:  Brass Section        |                             | clips directly onto    |
| Ch 10-13: Woodwinds Section   |                             | tracks                 |
| Ch 14-16: Percussion & Aux    |                             +------------------------+
+-------------------------------+
```

### Components
1. **Core Audio/MIDI Engine**: C++20 built on JUCE Framework (`juce_audio_processors`, `juce_audio_basics`, `juce_gui_basics`). Zero-allocation audio thread.
2. **User Interface**: Modern, minimalist UI via JUCE Webview (TypeScript, React, Tailwind CSS) providing seamless 60fps vector sequencers and drag-and-drop handles.
3. **Preset Engine**: JSON-based library profiles and pattern presets.

---

## 3. Harmonic Input Recognition Engine

### 3.1 Adaptive Grace Window
* Staggers incoming NoteOn events within an adjustable buffer (10–40ms, default 25ms).
* Gathers polyphonic arrivals before triggering chord re-evaluations, eliminating chord-recognition fluttering.
* Handles Sustain Pedal (CC64) and Legato finger transitions smoothly.

### 3.2 Chord Taxonomy & Root Detection
* **Bass Separation**: Lowest note registered as explicit bass note (supporting inverted/slash chords like `C/E`, `G/B`, `Dm/F`).
* **Root & Pitch Class Analysis**:
  * Unisons, Octaves, Single melodic notes
  * 5th Power Chords
  * Triads: Major, Minor, Diminished, Augmented, Sus2, Sus4
  * 7ths: Dominant 7, Maj7, Min7, Dim7, Half-dim (m7b5), MinMaj7, 7sus4, Aug7
  * 6ths: Maj6, Min6
  * Extensions: 9th, Maj9, Min9, Add9, 11th, #11 (Lydian), 13th, Altered dominants (b9, #9, b5, #5)
  * Clusters & Quartal Voicings

### 3.3 Output Harmonic Data Structure
```cpp
struct HarmonicFrame {
    int rootPitchClass;           // 0 = C, 1 = C#, ...
    int bassMidiNote;             // Lowest MIDI pitch
    ChordQuality quality;         // TriadMajor, Min7, Dom7, etc.
    std::vector<int> pitches;     // Raw incoming MIDI pitches
    std::vector<int> chordTones;  // Extended scale degree intervals
    int activeScaleMode;          // Ionian, Dorian, Phrygian, etc.
    bool isSingleNote;
    bool isSlashChord;
    double sampleTimestamp;
};
```

---

## 4. Orchestration & Voice-Leading Engine

### 4.1 Orchestral Registers (Tessituras)
* **Strings**: Contrabass ($C_1 - C_3$), Celli ($C_2 - G_4$), Violas ($C_3 - A_5$), Violins 2 ($G_3 - D_6$), Violins 1 ($G_3 - C_7$).
* **Brass**: Tuba ($D_1 - F_3$), Trombones ($E_2 - B_4$), Horns ($F_2 - F_5$), Trumpets ($F_3 - C_6$).
* **Woodwinds**: Bassoons ($B\flat_1 - E\flat_4$), Clarinets ($D_3 - G_6$), Oboes ($B\flat_3 - G_6$), Flutes ($C_4 - D_7$).
* **Percussion**: Timpani (tuned to root/fifth), Orchestral accents.

### 4.2 Voicing Strategies
* **Acoustic Pyramid**: Root/bass in lowest register with octave doublings, open spacing in low-mid register ($C_2 - C_4$) to avoid mud, dense chord colors and melody in mid-high register.
* **Drop-2 & Drop-4**: Hollywood film-score style wide open voicings.
* **Smart Divisi**: Automatic doubling of root or fifth when section count exceeds chord member count.
* **Continuous Voice-Leading**: Minimizes interval movement between chord changes using least-action distance heuristics.

---

## 5. Pattern Sequencer, Modal Shifting & Randomizer

### 5.1 Pattern Sequencer
* 16/32-step grid per section or per instrument track.
* Supported step actions:
  * `Sustain`: Long sustained pad / chord.
  * `Ostinato`: Rhythmic pulses ($1/8, 1/16$, triplets) with dynamic accent curve.
  * `Arpeggio`: Up, Down, Divergent, Convergent arpeggiator modes.
  * `Runs`: Fast scalar orchestral flourish leading to next bar.
  * `Rest`: NoteOff with sample-accurate gate length.
* Per-step controls: Velocity ($0-127$), Gate ($10-100\%$), Pitch/Octave offset ($-24$ to $+24$).

### 5.2 Modal Transformation & Harmonic Randomization
* **Real-time Modal Shifter**: Modulates chord notes to ecclesiastical modes (Ionian, Dorian, Phrygian, Lydian, Mixolydian, Aeolian, Locrian) or cinematic scales (Harmonic Minor, Whole-Tone, Octatonic).
* **Controlled Randomizer**:
  * Inversion Jitter: Musically shifts inversions every $N$ bars.
  * Tension Injections: Dynamically adds 9ths or #11ths based on mode rules.
  * Humanize: Subtle micro-timing (0–20ms) and velocity variation (0–15%).

---

## 6. Drag-and-Drop MIDI System

* **Master Drag**: Encapsulates the entire multi-section arrangement into a Standard MIDI File (.mid Type 1) with named tracks and designated channels (1-16).
* **Stem Drag**: Encapsulates individual instrument tracks (e.g., just *Violins 1* or just *Horns*) as a single-track MIDI clip.
* Native drag-and-drop into Cubase Project timeline for instant offline arrangement editing.

---

## 7. Universal Articulation Mapping Engine

Translates abstract musical expressions into target library triggers:
* **Keyswitch**: Configurable NoteOn pitch, velocity, and pre-delay buffer.
* **MIDI CC**: Controller number and threshold (e.g. CSS CC58, Spitfire UACC CC32).
* **Program Change**: Standard PC commands.
* **Bundled Profiles**: Cinematic Studio Series, Spitfire BBCSO, EastWest Hollywood Opus, VSL Synchron, Kontakt Factory.

---

## 8. Verification & Test Plan

1. **Unit Tests (C++ / Catch2 or GoogleTest)**:
   * Chord recognition test suite: validates 50+ chord types, inversions, and slash chords with simulated finger delays (grace window).
   * Voice leading test suite: asserts pitch continuity and tessitura limits across chord sequences.
   * MIDI File Generator test: confirms valid binary .mid Type 1 and Type 0 output with proper tempo and PPQ headers.
2. **Integration Verification**:
   * Host transport synchronization test against DAW PPQ and tempo fluctuations.
   * MIDI Dispatching test verifying correct channel routing across all 16 MIDI channels.
   * Articulation translator test verifying correct CC and Keyswitch output.
