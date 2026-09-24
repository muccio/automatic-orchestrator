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

---

## Cubase Track Archive & Kontakt 8 Quick-Start (1-Click Setup)

Pre-configured Steinberg Cubase Track Archives and Expression Maps are included in `cubase/`:

### 1. Import Ready-to-Test Orchestral Tracks
In Cubase:
1. Go to **File > Import > Track Archive (.xml)**.
2. Select:
   - `cubase/Hollywood_Orchestrator_Kontakt8_CSS_CSB_CSW.xml` (Individual Instrument Tracks pre-loaded with Kontakt 8 VST3 and Expression Maps)
   - OR `cubase/Hollywood_Orchestrator_Kontakt8_MultiOut.xml` (Multi-Timbral Kontakt 8 instance + 15 MIDI Tracks)
3. Cubase immediately creates the entire 16-channel orchestral template, already routed to Kontakt 8 (`/Library/Audio/Plug-Ins/VST3/Kontakt 8.vst3`)!

### 2. Expression Maps (CSS, CSB, CSW)
In the Cubase Inspector for each track, the corresponding Expression Map is ready:
- **Cinematic Studio Strings (CSS)**: `cubase/expression_maps/Cinematic_Studio_Strings.expressionmap`
- **Cinematic Studio Brass (CSB)**: `cubase/expression_maps/Cinematic_Studio_Brass.expressionmap`
- **Cinematic Studio Woodwinds (CSW)**: `cubase/expression_maps/Cinematic_Studio_Woodwinds.expressionmap`
Installed locally to: `~/Library/Application Support/Steinberg/Expression Maps/`

All articulation switches automatically output **CC58** commands:
| Articulation | CSS CC58 | CSB CC58 | CSW CC58 |
|---|---|---|---|
| Sustain / Legato | 0 | 0 | 0 |
| Repetitions | - | 10 | 10 |
| Spiccato / Staccatissimo | 20 / 30 | 20 | 20 |
| Staccato | 40 | 40 | 40 |
| Pizzicato / Rips / Trills | 60 | 55 (Rips) | 60 (Trills) |
| Tremolo / Flutter Tongue | 80 | 115 | 115 |
| Marcato | 100 | 100 | 100 |

### 3. MIDI Routing Guide
1. Create an Instrument Track loaded with **Hollywood Orchestrator** (installed to `~/Library/Audio/Plug-Ins/VST3/Hollywood Orchestrator.vst3`).
2. The imported Kontakt 8 tracks will listen to `Hollywood Orchestrator - MIDI Out` across Channels 1-16:
   - **Ch 1-5**: CSS Violins 1, Violins 2, Violas, Cellos, Double Basses
   - **Ch 6-9**: CSB Trumpets, French Horns, Trombones, Tuba
   - **Ch 10-13**: CSW Flutes, Oboes, Clarinets, Bassoons
   - **Ch 14-15**: Timpani & Orchestral Percussion
   - **Ch 16**: Hollywood Orchestrator Input / Master
3. Play any chord or progression: voices are assigned dynamically according to acoustic tessituras!
4. Drag the **DRAG MASTER MIDI** handle straight onto the Cubase timeline to export multi-track MIDI.

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
