#pragma once

#include "Common/HarmonicTypes.h"

namespace Orchestration {

struct Tessitura {
    int minPitch;    // Lowest playable MIDI pitch
    int maxPitch;    // Highest playable MIDI pitch
    int sweetMin;    // Optimal sweet spot bottom
    int sweetMax;    // Optimal sweet spot top
    int defaultOctave; // Reference octave placement
};

inline Tessitura getInstrumentTessitura(Harmonic::InstrumentId id) {
    using namespace Harmonic;
    switch (id) {
        // Strings
        case InstrumentId::DoubleBasses:
            return {24, 53, 28, 48, 2}; // C1 - F3 (sweet: E1 - C3)
        case InstrumentId::Cellos:
            return {36, 72, 36, 60, 3}; // C2 - C5 (sweet: C2 - C4)
        case InstrumentId::Violas:
            return {48, 84, 48, 72, 4}; // C3 - C6 (sweet: C3 - C5)
        case InstrumentId::Violins2:
            return {55, 96, 55, 79, 4}; // G3 - C7 (sweet: G3 - G5)
        case InstrumentId::Violins1:
            return {55, 103, 60, 88, 5}; // G3 - G7 (sweet: C4 - E6)

        // Brass
        case InstrumentId::Tuba:
            return {28, 58, 28, 50, 2}; // E1 - Bb3
        case InstrumentId::Trombones:
            return {40, 72, 40, 64, 3}; // E2 - C5
        case InstrumentId::FrenchHorns:
            return {41, 77, 48, 69, 3}; // F2 - F5
        case InstrumentId::Trumpets:
            return {52, 86, 55, 79, 4}; // E3 - D6

        // Woodwinds
        case InstrumentId::Bassoons:
            return {34, 70, 36, 60, 3}; // Bb1 - Bb4
        case InstrumentId::Clarinets:
            return {50, 91, 53, 81, 4}; // D3 - G6
        case InstrumentId::Oboes:
            return {58, 89, 58, 81, 4}; // Bb3 - F6
        case InstrumentId::Flutes:
            return {60, 98, 65, 91, 5}; // C4 - D7

        // Percussion
        case InstrumentId::Timpani:
            return {36, 55, 36, 53, 2}; // C2 - G3
        case InstrumentId::OrchestralPerc:
            return {36, 84, 48, 72, 3};
        case InstrumentId::Celesta:
            return {48, 96, 60, 96, 5}; // C3 - C7

        // Keyboards & Harp
        case InstrumentId::Harp:
            return {24, 103, 36, 96, 4}; // C1 - G7
        case InstrumentId::Piano:
            return {21, 108, 36, 84, 4}; // A0 - C8
        case InstrumentId::ChurchOrgan:
            return {24, 96, 36, 84, 3};  // C1 - C7

        // Guitars & Bass
        case InstrumentId::AcousticGuitar:
            return {40, 83, 45, 76, 3};  // E2 - B5
        case InstrumentId::ElectricGuitar:
            return {40, 88, 45, 81, 3};  // E2 - E6
        case InstrumentId::BassGuitar:
            return {28, 55, 28, 48, 2};  // E1 - G3

        // Choir
        case InstrumentId::ChoirFull:
            return {36, 84, 48, 77, 4};  // C2 - C6

        // Synths
        case InstrumentId::SynthesizerLead:
            return {36, 96, 60, 88, 4};
        case InstrumentId::SynthesizerPad:
            return {36, 96, 48, 72, 3};

        default:
            return {48, 72, 48, 72, 4};
    }
}

inline int getInstrumentDefaultMidiChannel(Harmonic::InstrumentId id) {
    return Harmonic::getDefaultInstrumentChannel(id);
}

} // namespace Orchestration
