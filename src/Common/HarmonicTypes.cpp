#include "HarmonicTypes.h"
#include <sstream>

namespace Harmonic {

static const char* sharpNoteNames[12] = {
    "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
};

static const char* flatNoteNames[12] = {
    "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
};

std::string pitchClassToName(int pc, bool useSharps) {
    int normalized = ((pc % 12) + 12) % 12;
    return useSharps ? sharpNoteNames[normalized] : flatNoteNames[normalized];
}

std::string noteNumberToName(int midiNote, bool showOctave) {
    int pc = ((midiNote % 12) + 12) % 12;
    std::string name = pitchClassToName(pc, true);
    if (!showOctave) return name;
    int octave = (midiNote / 12) - 1; // 60 / 12 - 1 = 4 -> C4
    return name + std::to_string(octave);
}

std::string chordQualityToString(ChordQuality q) {
    switch (q) {
        case ChordQuality::SingleNote: return "Note";
        case ChordQuality::PowerChord: return "5";
        case ChordQuality::MajorTriad: return "Major";
        case ChordQuality::MinorTriad: return "Minor";
        case ChordQuality::DiminishedTriad: return "dim";
        case ChordQuality::AugmentedTriad: return "aug";
        case ChordQuality::Sus2: return "sus2";
        case ChordQuality::Sus4: return "sus4";
        case ChordQuality::Dominant7: return "7";
        case ChordQuality::Major7: return "Maj7";
        case ChordQuality::Minor7: return "Min7";
        case ChordQuality::Diminished7: return "dim7";
        case ChordQuality::HalfDiminished7: return "m7b5";
        case ChordQuality::MinorMajor7: return "mMaj7";
        case ChordQuality::Augmented7: return "aug7";
        case ChordQuality::AugmentedMajor7: return "augMaj7";
        case ChordQuality::SevenSus4: return "7sus4";
        case ChordQuality::Major6: return "6";
        case ChordQuality::Minor6: return "m6";
        case ChordQuality::Dominant9: return "9";
        case ChordQuality::Major9: return "Maj9";
        case ChordQuality::Minor9: return "Min9";
        case ChordQuality::Add9: return "add9";
        case ChordQuality::Eleventh: return "11";
        case ChordQuality::Sharp11: return "Maj7(#11)";
        case ChordQuality::Thirteenth: return "13";
        case ChordQuality::AlteredDominant: return "7alt";
        case ChordQuality::Cluster: return "Cluster";
        case ChordQuality::Quartal: return "Quartal";
        default: return "Unknown";
    }
}

std::string scaleModeToString(ScaleMode m) {
    switch (m) {
        case ScaleMode::Ionian: return "Ionian (Major)";
        case ScaleMode::Dorian: return "Dorian";
        case ScaleMode::Phrygian: return "Phrygian";
        case ScaleMode::Lydian: return "Lydian";
        case ScaleMode::Mixolydian: return "Mixolydian";
        case ScaleMode::Aeolian: return "Aeolian (Minor)";
        case ScaleMode::Locrian: return "Locrian";
        case ScaleMode::HarmonicMinor: return "Harmonic Minor";
        case ScaleMode::MelodicMinor: return "Melodic Minor";
        case ScaleMode::WholeTone: return "Whole-Tone";
        case ScaleMode::Octatonic: return "Octatonic (Diminished)";
        default: return "Unknown";
    }
}

std::string instrumentToString(InstrumentId id) {
    switch (id) {
        case InstrumentId::Violins1: return "Violins 1";
        case InstrumentId::Violins2: return "Violins 2";
        case InstrumentId::Violas: return "Violas";
        case InstrumentId::Cellos: return "Cellos";
        case InstrumentId::DoubleBasses: return "Double Basses";
        case InstrumentId::Harp: return "Harp";
        case InstrumentId::Trumpets: return "Trumpets";
        case InstrumentId::FrenchHorns: return "French Horns";
        case InstrumentId::Trombones: return "Trombones";
        case InstrumentId::Tuba: return "Tuba";
        case InstrumentId::Flutes: return "Flutes";
        case InstrumentId::Oboes: return "Oboes";
        case InstrumentId::Clarinets: return "Clarinets";
        case InstrumentId::Bassoons: return "Bassoons";
        case InstrumentId::Timpani: return "Timpani";
        case InstrumentId::OrchestralPerc: return "Percussion";
        case InstrumentId::Celesta: return "Celesta";
        case InstrumentId::Piano: return "Piano";
        case InstrumentId::ChurchOrgan: return "Church Organ";
        case InstrumentId::AcousticGuitar: return "Acoustic Guitar";
        case InstrumentId::ElectricGuitar: return "Electric Guitar";
        case InstrumentId::BassGuitar: return "Bass Guitar";
        case InstrumentId::ChoirFull: return "Choir";
        case InstrumentId::SynthesizerLead: return "Synth Lead";
        case InstrumentId::SynthesizerPad: return "Synth Pad";
        default: return "Instrument";
    }
}

std::string articulationToString(ArticulationType art) {
    switch (art) {
        case ArticulationType::Sustain: return "Sustain";
        case ArticulationType::Staccato: return "Staccato";
        case ArticulationType::Spiccato: return "Spiccato";
        case ArticulationType::Marcato: return "Marcato";
        case ArticulationType::Tremolo: return "Tremolo";
        case ArticulationType::Pizzicato: return "Pizzicato";
        case ArticulationType::Trill_m2: return "Trill m2";
        case ArticulationType::Trill_M2: return "Trill M2";
        case ArticulationType::Runs: return "Runs";
        default: return "Sustain";
    }
}

std::vector<int> getScaleModeIntervals(ScaleMode mode) {
    switch (mode) {
        case ScaleMode::Ionian:
            return {0, 2, 4, 5, 7, 9, 11};
        case ScaleMode::Dorian:
            return {0, 2, 3, 5, 7, 9, 10};
        case ScaleMode::Phrygian:
            return {0, 1, 3, 5, 7, 8, 10};
        case ScaleMode::Lydian:
            return {0, 2, 4, 6, 7, 9, 11};
        case ScaleMode::Mixolydian:
            return {0, 2, 4, 5, 7, 9, 10};
        case ScaleMode::Aeolian:
            return {0, 2, 3, 5, 7, 8, 10};
        case ScaleMode::Locrian:
            return {0, 1, 3, 5, 6, 8, 10};
        case ScaleMode::HarmonicMinor:
            return {0, 2, 3, 5, 7, 8, 11};
        case ScaleMode::MelodicMinor:
            return {0, 2, 3, 5, 7, 9, 11};
        case ScaleMode::WholeTone:
            return {0, 2, 4, 6, 8, 10};
        case ScaleMode::Octatonic:
            return {0, 1, 3, 4, 6, 7, 9, 10};
        default:
            return {0, 2, 4, 5, 7, 9, 11};
    }
}

std::vector<int> getChordQualityIntervals(ChordQuality q) {
    switch (q) {
        case ChordQuality::SingleNote: return {0};
        case ChordQuality::PowerChord: return {0, 7};
        case ChordQuality::MajorTriad: return {0, 4, 7};
        case ChordQuality::MinorTriad: return {0, 3, 7};
        case ChordQuality::DiminishedTriad: return {0, 3, 6};
        case ChordQuality::AugmentedTriad: return {0, 4, 8};
        case ChordQuality::Sus2: return {0, 2, 7};
        case ChordQuality::Sus4: return {0, 5, 7};
        case ChordQuality::Dominant7: return {0, 4, 7, 10};
        case ChordQuality::Major7: return {0, 4, 7, 11};
        case ChordQuality::Minor7: return {0, 3, 7, 10};
        case ChordQuality::Diminished7: return {0, 3, 6, 9};
        case ChordQuality::HalfDiminished7: return {0, 3, 6, 10};
        case ChordQuality::MinorMajor7: return {0, 3, 7, 11};
        case ChordQuality::Augmented7: return {0, 4, 8, 10};
        case ChordQuality::AugmentedMajor7: return {0, 4, 8, 11};
        case ChordQuality::SevenSus4: return {0, 5, 7, 10};
        case ChordQuality::Major6: return {0, 4, 7, 9};
        case ChordQuality::Minor6: return {0, 3, 7, 9};
        case ChordQuality::Dominant9: return {0, 4, 7, 10, 2};
        case ChordQuality::Major9: return {0, 4, 7, 11, 2};
        case ChordQuality::Minor9: return {0, 3, 7, 10, 2};
        case ChordQuality::Add9: return {0, 4, 7, 2};
        case ChordQuality::Eleventh: return {0, 4, 7, 10, 2, 5};
        case ChordQuality::Sharp11: return {0, 4, 7, 11, 6};
        case ChordQuality::Thirteenth: return {0, 4, 7, 10, 2, 9};
        default: return {0, 4, 7};
    }
}

OrchestralSection getInstrumentSection(InstrumentId id) {
    switch (id) {
        case InstrumentId::Violins1:
        case InstrumentId::Violins2:
        case InstrumentId::Violas:
        case InstrumentId::Cellos:
        case InstrumentId::DoubleBasses:
        case InstrumentId::Harp:
            return OrchestralSection::Strings;
        case InstrumentId::FrenchHorns:
        case InstrumentId::Trumpets:
        case InstrumentId::Trombones:
        case InstrumentId::Tuba:
            return OrchestralSection::Brass;
        case InstrumentId::Flutes:
        case InstrumentId::Oboes:
        case InstrumentId::Clarinets:
        case InstrumentId::Bassoons:
            return OrchestralSection::Woodwinds;
        case InstrumentId::Timpani:
        case InstrumentId::OrchestralPerc:
        case InstrumentId::Celesta:
            return OrchestralSection::Percussion;
        case InstrumentId::Piano:
        case InstrumentId::ChurchOrgan:
            return OrchestralSection::Keyboards;
        case InstrumentId::AcousticGuitar:
        case InstrumentId::ElectricGuitar:
        case InstrumentId::BassGuitar:
            return OrchestralSection::Guitars;
        case InstrumentId::ChoirFull:
            return OrchestralSection::Choir;
        case InstrumentId::SynthesizerLead:
        case InstrumentId::SynthesizerPad:
            return OrchestralSection::Synths;
        default:
            return OrchestralSection::Strings;
    }
}

int getDefaultInstrumentChannel(InstrumentId id) {
    switch (id) {
        case InstrumentId::Violins1: return 1;
        case InstrumentId::Violins2: return 2;
        case InstrumentId::Violas: return 3;
        case InstrumentId::Cellos: return 4;
        case InstrumentId::DoubleBasses: return 5;
        case InstrumentId::Trumpets: return 6;
        case InstrumentId::FrenchHorns: return 7;
        case InstrumentId::Trombones: return 8;
        case InstrumentId::Tuba: return 9;
        case InstrumentId::Flutes: return 10;
        case InstrumentId::Oboes: return 11;
        case InstrumentId::Clarinets: return 12;
        case InstrumentId::Bassoons: return 13;
        case InstrumentId::Timpani: return 14;
        case InstrumentId::OrchestralPerc: return 15;
        case InstrumentId::Harp: return 16;
        case InstrumentId::Celesta: return 15;
        case InstrumentId::Piano: return 1;
        case InstrumentId::ChurchOrgan: return 16;
        case InstrumentId::AcousticGuitar: return 2;
        case InstrumentId::ElectricGuitar: return 3;
        case InstrumentId::BassGuitar: return 4;
        case InstrumentId::ChoirFull: return 5;
        case InstrumentId::SynthesizerLead: return 6;
        case InstrumentId::SynthesizerPad: return 7;
        default: return 1;
    }
}
std::ostream& operator<<(std::ostream& os, ChordQuality q) {
    return os << chordQualityToString(q);
}

std::ostream& operator<<(std::ostream& os, ScaleMode m) {
    return os << scaleModeToString(m);
}

std::ostream& operator<<(std::ostream& os, InstrumentId id) {
    return os << instrumentToString(id);
}

std::ostream& operator<<(std::ostream& os, ArticulationType art) {
    return os << articulationToString(art);
}

std::ostream& operator<<(std::ostream& os, StepActionType act) {
    switch (act) {
        case StepActionType::Rest: return os << "Rest";
        case StepActionType::Sustain: return os << "Sustain";
        case StepActionType::Ostinato: return os << "Ostinato";
        case StepActionType::ArpUp: return os << "ArpUp";
        case StepActionType::ArpDown: return os << "ArpDown";
        case StepActionType::ArpUpDown: return os << "ArpUpDown";
        case StepActionType::ArpRandom: return os << "ArpRandom";
        case StepActionType::Runs: return os << "Runs";
        default: return os << "Action";
    }
}

} // namespace Harmonic
