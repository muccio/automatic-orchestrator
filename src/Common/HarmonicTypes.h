#pragma once

#include <string>
#include <vector>
#include <array>
#include <cstdint>

namespace Harmonic {

enum class ChordQuality {
    Unknown,
    SingleNote,
    PowerChord,
    MajorTriad,
    MinorTriad,
    DiminishedTriad,
    AugmentedTriad,
    Sus2,
    Sus4,
    Dominant7,
    Major7,
    Minor7,
    Diminished7,
    HalfDiminished7,
    MinorMajor7,
    Augmented7,
    AugmentedMajor7,
    SevenSus4,
    Major6,
    Minor6,
    Dominant9,
    Major9,
    Minor9,
    Add9,
    Eleventh,
    Sharp11,
    Thirteenth,
    AlteredDominant,
    Cluster,
    Quartal
};

enum class ScaleMode {
    Ionian,       // Major
    Dorian,       // Minor with #6
    Phrygian,     // Minor with b2
    Lydian,       // Major with #4
    Mixolydian,   // Major with b7
    Aeolian,      // Natural Minor
    Locrian,      // Half-diminished
    HarmonicMinor,
    MelodicMinor,
    WholeTone,
    Octatonic
};

enum class OrchestralSection {
    Strings,
    Brass,
    Woodwinds,
    Percussion,
    Keyboards,
    Guitars,
    Choir,
    Synths
};

enum class InstrumentId {
    // Strings
    Violins1,
    Violins2,
    Violas,
    Cellos,
    DoubleBasses,
    Harp,

    // Brass
    Trumpets,
    FrenchHorns,
    Trombones,
    Tuba,

    // Woodwinds
    Flutes,
    Oboes,
    Clarinets,
    Bassoons,

    // Percussion
    Timpani,
    OrchestralPerc,
    Celesta,

    // Keyboards
    Piano,
    ChurchOrgan,

    // Guitars & Bass
    AcousticGuitar,
    ElectricGuitar,
    BassGuitar,

    // Choir
    ChoirFull,

    // Synths
    SynthesizerLead,
    SynthesizerPad
};

enum class ArticulationType {
    Sustain,
    Staccato,
    Spiccato,
    Marcato,
    Tremolo,
    Pizzicato,
    Trill_m2,
    Trill_M2,
    Runs
};

enum class StepActionType {
    Rest,
    Sustain,
    Ostinato,
    ArpUp,
    ArpDown,
    ArpUpDown,
    ArpRandom,
    Runs
};

struct HarmonicFrame {
    int rootPitchClass = 0;           // 0 = C, 1 = C#, ... 11 = B
    int bassMidiNote = 36;            // Lowest active MIDI note
    ChordQuality quality = ChordQuality::Unknown;
    std::vector<int> pitches;         // All unique incoming active MIDI pitches, sorted
    std::vector<int> chordTones;      // Intervals from root (e.g. {0, 4, 7})
    ScaleMode activeMode = ScaleMode::Ionian;
    bool isSingleNote = false;
    bool isSlashChord = false;
    std::string chordName;
    double timestamp = 0.0;
};

struct VoiceAssignment {
    InstrumentId instrument;
    int midiPitch;
    int velocity;
    ArticulationType articulation;
    int midiChannel; // 1 to 16
};

struct OrchestralVoicing {
    std::vector<VoiceAssignment> voices;
    HarmonicFrame sourceHarmonic;
};

// Conversions & Helpers
std::string pitchClassToName(int pc, bool useSharps = true);
std::string noteNumberToName(int midiNote, bool showOctave = true);
std::string chordQualityToString(ChordQuality q);
std::string scaleModeToString(ScaleMode m);
std::string instrumentToString(InstrumentId id);
std::string articulationToString(ArticulationType art);
std::vector<int> getScaleModeIntervals(ScaleMode mode);
std::vector<int> getChordQualityIntervals(ChordQuality q);
OrchestralSection getInstrumentSection(InstrumentId id);
int getDefaultInstrumentChannel(InstrumentId id);

std::ostream& operator<<(std::ostream& os, ChordQuality q);
std::ostream& operator<<(std::ostream& os, ScaleMode m);
std::ostream& operator<<(std::ostream& os, InstrumentId id);
std::ostream& operator<<(std::ostream& os, ArticulationType art);
std::ostream& operator<<(std::ostream& os, StepActionType act);

} // namespace Harmonic
