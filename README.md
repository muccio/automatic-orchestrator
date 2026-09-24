# Hollywood Orchestrator (VST3 / Standalone for macOS)

A powerful, open-architecture orchestral arrangement engine and real-time MIDI source plugin for macOS and Steinberg Cubase, inspired by EastWest Hollywood Orchestrator.

Unlike closed proprietary orchestrators tied to specific sample libraries, this engine acts as a **universal MIDI multiplexer**, generating 16-channel orchestral arrangements for **any** virtual instrument library (Cinematic Studio Series, Spitfire Audio BBCSO/Abbey Road, EastWest OPUS, VSL Synchron, Kontakt Factory, etc.).

---

## Key Features

- **Real-Time Polyphonic Chord Recognition**:
  - Detects single notes, power chords, triadic harmonies, 7ths, 9ths, 11ths, #11, 13ths, and altered dominants.
  - **Slash Chord & Inversion Engine**: Extracts the true bass note to maintain authentic voice distribution (e.g. `C/E`, `Dm/F`, `Bb/C`, `G7/B`).
  - **Adaptive Grace Window (10-40ms)**: Sample-accurate debouncing that groups simultaneous keystrokes without harmonic fluttering.
  - Full **CC64 Sustain Pedal** and legato transition support.

- **Acoustic Orchestration & Dynamic Voicing**:
  - Built-in realistic acoustic tessituras for 14 instruments (Violins 1/2, Violas, Cellos, Double Basses, Trumpets, French Horns, Trombones, Tuba, Flutes, Oboes, Clarinets, Bassoons, Timpani, Percussion).
  - Voicing styles: **Acoustic Pyramid**, **Drop-2**, **Drop-4**, **Close**, **Open**.
  - **Voice Leading Minimizer**: Eliminates unnatural octave jumping between chord changes.

- **Modal Transformation & Harmonic Randomization**:
  - Real-time modal shifter: Ionian, Dorian, Phrygian, Lydian (#11), Mixolydian, Aeolian, Harmonic Minor, Whole-Tone, Octatonic.
  - **Musical Inversion Jitter**: Periodic organic shifts of chord inversions.
  - **Modal Color / Tension Injection**: Adds subtle 9ths, #11ths, or 6ths to woodwinds and upper strings.
  - **Humanize**: Micro-timing (0-15ms) and organic velocity variation.

- **Universal Articulation Mapping**:
  - Pre-mapped profiles for:
    - **Cinematic Studio Series** (CSS CC58 protocol)
    - **Spitfire Audio** (UACC CC32 standard)
    - **EastWest Hollywood Opus** (Keyswitches)
    - **VSL Synchron** (Keyswitches)
    - **Kontakt Factory**

- **Timeline Drag-and-Drop MIDI**:
  - **Master Drag**: Generates a Standard MIDI File (.mid Type 1) with 16 separate named tracks ready to drop directly into Cubase.
  - **Stem Drag**: Export individual instrument tracks (e.g. just *Violins 1* or just *Horns*) as single-track MIDI clips.

- **Modern Minimalist UI**:
  - High-visibility glowing chord & bass badge.
  - Interactive 16-step sequencer matrix.
  - Dual implementation: Native C++ JUCE vector GUI + Modern Webview HTML5/CSS/JS interface.

---

## Cubase Routing Guide

1. Place `Hollywood Orchestrator.vst3` into:
   ```
   ~/Library/Audio/Plug-Ins/VST3/
   ```
2. In Cubase, create an Instrument Track loaded with **Hollywood Orchestrator**.
3. On your orchestral library tracks (e.g., CSS Violins, BBCSO Horns, CineBrass Trombones):
   - Set the track's **MIDI Input** to `Hollywood Orchestrator - MIDI Out`.
   - Set the track's **MIDI Channel**:
     - **Ch 1**: Violins 1
     - **Ch 2**: Violins 2
     - **Ch 3**: Violas
     - **Ch 4**: Cellos
     - **Ch 5**: Double Basses
     - **Ch 6**: Trumpets
     - **Ch 7**: French Horns
     - **Ch 8**: Trombones
     - **Ch 9**: Tuba
     - **Ch 10**: Flutes
     - **Ch 11**: Oboes
     - **Ch 12**: Clarinets
     - **Ch 13**: Bassoons
     - **Ch 14**: Timpani
     - **Ch 15**: Percussion
4. Play a chord on your MIDI controller: the engine will orchestrate the parts in real time!
5. Drag the **DRAG MASTER MIDI** button straight onto the Cubase Project Window timeline to create editable MIDI clips.

---

## Build from Source

Requirements: CMake 3.20+, C++20 compiler (`clang++`), JUCE 7+.

```bash
# Configure
DEVELOPER_DIR=/Library/Developer/CommandLineTools cmake -B build -S .

# Run Full Test Suite (26 tests)
DEVELOPER_DIR=/Library/Developer/CommandLineTools cmake --build build --target orchestrator_tests
./build/orchestrator_tests --all

# Build VST3 and Standalone
DEVELOPER_DIR=/Library/Developer/CommandLineTools cmake --build build --target HollywoodOrchestrator_VST3 HollywoodOrchestrator_Standalone
```

---

## License

MIT License.
