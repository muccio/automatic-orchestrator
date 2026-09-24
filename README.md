# Automatic Orchestrator (VST3 / Standalone for macOS)

A powerful, open-architecture orchestral arrangement engine and real-time MIDI source plugin for macOS and Steinberg Cubase, built with the full step arranger architecture of EastWest Hollywood Orchestrator.

Unlike closed proprietary orchestrators tied to specific sample libraries, **Automatic Orchestrator** acts as a **universal MIDI multiplexer**, generating 16-channel orchestral arrangements for **any** virtual instrument library (Cinematic Studio Series, Spitfire Audio BBCSO/Abbey Road, EastWest OPUS, VSL Synchron, Kontakt Factory, etc.).

---

## Key Features

- **Genuine Hollywood Orchestrator Arranger Architecture**:
  - **4 Orchestral Section Tabs**: `WOODWINDS`, `BRASS`, `PERCUSSION`, `STRINGS` with global section Mute/Solo.
  - **Left Instrument Rack**: For each active section instrument:
    - Articulation selector (Sustain/Legato, Spiccato, Staccato, Marcato, Tremolo, Pizzicato, Runs).
    - Individual Mute [M] and Solo [S] controls.
    - Arranger Mode dropdown (`Top`, `Lowest`, `Chord`, `Root`, `Arp Up`, `Arp Down`).
    - Octave selector (`-2`, `-1`, `0`, `+1`, `+2`).
    - Volume rotary slider and Stem MIDI drag handle.
  - **Right Step Arranger / Piano Roll Matrix**:
    - **18 Relative Pitch Rows**: `Steps +9` through `Steps +1`, `Lowest` (Root), and `Steps -1` through `Steps -8`.
    - **16/32-Step Columns**: Bar 1 and Bar 2 with real-time animated sweeping playhead.
    - Interactive note editing: Click any cell to add, move, or toggle notes at that step and pitch offset!
    - Voice 1 / Voice 2 toggle, Note Grid selector (`1/16`, `1/16T`, `1/8`, `1/4`), Pencil, Eraser, and Clear tools.
  - **CC1 Dynamics / Modulation Automation Lane**:
    - Located directly underneath the step grid.
    - Real-time interactive curve / bar editor to draw orchestral dynamics swells and crescendos.
    - Emits sample-accurate CC1 events during playback and writes them to exported MIDI.
  - **Live Preset Synchronization**:
    - Changing presets (`Action Ostinato`, `Epic Brass Fanfare`, `Lyrical Adagio`, `Suspense Mystery`, `War Drums`, `Fantasy Adventure`) instantly updates all instrument rows, displays actual notes on the grid, and sets CC1 dynamics.

- **Real-Time Harmonic Recognition**:
  - High-precision polyphonic detection of single notes, power chords, triadic harmonies, 7ths, 9ths, 11ths, #11, 13ths, and altered dominants.
  - Displays the true played chord (e.g. `Cmaj7`, `Dm9`, `G7`, `F/G`) without destructively quantizing it.
  - **Adaptive Grace Window (10-40ms)**: Sample-accurate debouncing that groups simultaneous keystrokes without harmonic fluttering.
  - Full **CC64 Sustain Pedal** and legato transition support.

- **Timeline Drag-and-Drop MIDI**:
  - **Master Drag**: Generates a Standard MIDI File (.mid Type 1) with 16 separate named tracks ready to drop directly into Cubase.
  - **Stem Drag**: Export individual instrument tracks as single-track MIDI clips.
  - Single-latch mouse drag gesture ensures flawless drag-and-drop into Cubase timeline.
  - Built-in default voicing ensures immediate MIDI export even before a live chord is played!

- **Universal Articulation Mapping**:
  - Pre-mapped profiles for:
    - **Cinematic Studio Series** (CSS CC58 protocol)
    - **Spitfire Audio** (UACC CC32 standard)
    - **EastWest Hollywood Opus** (Keyswitches)
    - **VSL Synchron** (Keyswitches)
    - **Kontakt Factory**

---

## Cubase Track Archive & Kontakt 8 Quick-Start (1-Click Setup)

Pre-configured Steinberg Cubase Track Archives and Expression Maps are included in `cubase/`:

### 1. Import Ready-to-Test Orchestral Tracks
In Cubase:
1. Go to **File > Import > Track Archive (.xml)**.
2. Select:
   - `cubase/Hollywood_Orchestrator_Kontakt8_CSS_CSB_CSW.xml` (Individual Instrument Tracks pre-loaded with Kontakt 8 VST3 and Expression Maps)
   - OR `cubase/Hollywood_Orchestrator_Kontakt8_MultiOut.xml` (Multi-Timbral Kontakt 8 instance + 15 MIDI Tracks)
3. Cubase immediately creates the entire 16-channel orchestral template, already routed to Kontakt 8!

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
1. Create an Instrument Track loaded with **Automatic Orchestrator** (installed to `~/Library/Audio/Plug-Ins/VST3/Automatic Orchestrator.vst3`).
2. The imported Kontakt 8 tracks listen to `Automatic Orchestrator - MIDI Out` across Channels 1-16:
   - **Ch 1-5**: CSS Violins 1, Violins 2, Violas, Cellos, Double Basses
   - **Ch 6-9**: CSB Trumpets, French Horns, Trombones, Tuba
   - **Ch 10-13**: CSW Flutes, Oboes, Clarinets, Bassoons
   - **Ch 14-15**: Timpani & Orchestral Percussion
   - **Ch 16**: Automatic Orchestrator Input / Master
3. Play any chord or progression: voices are assigned dynamically according to acoustic tessituras and the step arranger pattern!
4. Drag the **DRAG MASTER MIDI** handle straight onto the Cubase timeline to export multi-track MIDI with notes, CC1 dynamics, and articulations.

---

## Build from Source

Requirements: CMake 3.20+, C++20 compiler (`clang++`), JUCE 7+.

```bash
# Configure
DEVELOPER_DIR=/Library/Developer/CommandLineTools cmake -B build -S .

# Run Full Test Suite (27 tests)
DEVELOPER_DIR=/Library/Developer/CommandLineTools cmake --build build --target orchestrator_tests -j8
./build/orchestrator_tests

# Build VST3 and Standalone
DEVELOPER_DIR=/Library/Developer/CommandLineTools cmake --build build --target AutomaticOrchestrator_VST3 AutomaticOrchestrator_Standalone -j8
```

---

## License & Author
Developed by OrchestratorLab. Open-source under MIT License.
