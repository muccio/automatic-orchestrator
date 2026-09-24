#include "PatternModel.h"

namespace Sequencer {

static TrackPattern makeOstinatoTrack(Harmonic::InstrumentId id, Harmonic::ArticulationType art) {
    TrackPattern tp;
    tp.instrument = id;
    tp.stepCount = 16;
    tp.stepDivision = 0.25; // 1/16th note
    tp.steps.resize(16);

    for (int i = 0; i < 16; ++i) {
        StepDefinition sd;
        sd.action = Harmonic::StepActionType::Ostinato;
        // Accent on downbeats
        sd.velocity = (i % 4 == 0) ? 105 : ((i % 2 == 0) ? 90 : 78);
        sd.gate = 0.6;
        sd.octaveOffset = 0;
        sd.articulation = art;
        tp.steps[i] = sd;
    }
    return tp;
}

static TrackPattern makeSustainTrack(Harmonic::InstrumentId id) {
    TrackPattern tp;
    tp.instrument = id;
    tp.stepCount = 16;
    tp.stepDivision = 0.25;
    tp.steps.resize(16);

    for (int i = 0; i < 16; ++i) {
        StepDefinition sd;
        sd.action = (i == 0) ? Harmonic::StepActionType::Sustain : Harmonic::StepActionType::Rest;
        sd.velocity = 85;
        sd.gate = 1.0;
        sd.octaveOffset = 0;
        sd.articulation = Harmonic::ArticulationType::Sustain;
        tp.steps[i] = sd;
    }
    return tp;
}

static TrackPattern makeArpTrack(Harmonic::InstrumentId id, bool up = true) {
    TrackPattern tp;
    tp.instrument = id;
    tp.stepCount = 16;
    tp.stepDivision = 0.25;
    tp.steps.resize(16);

    for (int i = 0; i < 16; ++i) {
        StepDefinition sd;
        sd.action = up ? Harmonic::StepActionType::ArpUp : Harmonic::StepActionType::ArpDown;
        sd.velocity = 80 + (i % 4) * 5;
        sd.gate = 0.8;
        sd.octaveOffset = 0;
        sd.articulation = Harmonic::ArticulationType::Staccato;
        tp.steps[i] = sd;
    }
    return tp;
}

OrchestralPattern createActionOstinatoPattern() {
    OrchestralPattern p;
    p.name = "Action Ostinato";
    p.styleCategory = "Action";

    // High Strings: driving spiccato ostinatos
    p.tracks[Harmonic::InstrumentId::Violins1] = makeOstinatoTrack(Harmonic::InstrumentId::Violins1, Harmonic::ArticulationType::Spiccato);
    p.tracks[Harmonic::InstrumentId::Violins2] = makeOstinatoTrack(Harmonic::InstrumentId::Violins2, Harmonic::ArticulationType::Spiccato);
    p.tracks[Harmonic::InstrumentId::Violas]   = makeOstinatoTrack(Harmonic::InstrumentId::Violas, Harmonic::ArticulationType::Spiccato);

    // Low Strings: solid marcato bass pulse
    p.tracks[Harmonic::InstrumentId::Cellos] = makeOstinatoTrack(Harmonic::InstrumentId::Cellos, Harmonic::ArticulationType::Marcato);
    p.tracks[Harmonic::InstrumentId::DoubleBasses] = makeOstinatoTrack(Harmonic::InstrumentId::DoubleBasses, Harmonic::ArticulationType::Marcato);

    // Brass: sustained chords and punchy horns
    p.tracks[Harmonic::InstrumentId::FrenchHorns] = makeSustainTrack(Harmonic::InstrumentId::FrenchHorns);
    p.tracks[Harmonic::InstrumentId::Trombones]   = makeSustainTrack(Harmonic::InstrumentId::Trombones);
    p.tracks[Harmonic::InstrumentId::Trumpets]    = makeOstinatoTrack(Harmonic::InstrumentId::Trumpets, Harmonic::ArticulationType::Staccato);
    p.tracks[Harmonic::InstrumentId::Tuba]        = makeOstinatoTrack(Harmonic::InstrumentId::Tuba, Harmonic::ArticulationType::Marcato);

    // Woodwinds: runs and arpeggiated movement
    p.tracks[Harmonic::InstrumentId::Flutes]    = makeArpTrack(Harmonic::InstrumentId::Flutes, true);
    p.tracks[Harmonic::InstrumentId::Clarinets] = makeArpTrack(Harmonic::InstrumentId::Clarinets, false);
    p.tracks[Harmonic::InstrumentId::Oboes]     = makeSustainTrack(Harmonic::InstrumentId::Oboes);
    p.tracks[Harmonic::InstrumentId::Bassoons]  = makeOstinatoTrack(Harmonic::InstrumentId::Bassoons, Harmonic::ArticulationType::Staccato);

    // Percussion: timpani accents on beats 1 and 3
    TrackPattern timp;
    timp.instrument = Harmonic::InstrumentId::Timpani;
    timp.stepCount = 16;
    timp.stepDivision = 0.25;
    timp.steps.resize(16);
    for (int i = 0; i < 16; ++i) {
        StepDefinition sd;
        sd.action = (i % 8 == 0) ? Harmonic::StepActionType::Ostinato : Harmonic::StepActionType::Rest;
        sd.velocity = 110;
        sd.gate = 0.8;
        sd.articulation = Harmonic::ArticulationType::Marcato;
        timp.steps[i] = sd;
    }
    p.tracks[Harmonic::InstrumentId::Timpani] = timp;

    return p;
}

OrchestralPattern createLyricalAdagioPattern() {
    OrchestralPattern p;
    p.name = "Lyrical Adagio";
    p.styleCategory = "Romantic";

    p.tracks[Harmonic::InstrumentId::Violins1]     = makeSustainTrack(Harmonic::InstrumentId::Violins1);
    p.tracks[Harmonic::InstrumentId::Violins2]     = makeSustainTrack(Harmonic::InstrumentId::Violins2);
    p.tracks[Harmonic::InstrumentId::Violas]       = makeSustainTrack(Harmonic::InstrumentId::Violas);
    p.tracks[Harmonic::InstrumentId::Cellos]       = makeSustainTrack(Harmonic::InstrumentId::Cellos);
    p.tracks[Harmonic::InstrumentId::DoubleBasses] = makeSustainTrack(Harmonic::InstrumentId::DoubleBasses);
    p.tracks[Harmonic::InstrumentId::FrenchHorns]  = makeSustainTrack(Harmonic::InstrumentId::FrenchHorns);
    p.tracks[Harmonic::InstrumentId::Flutes]       = makeSustainTrack(Harmonic::InstrumentId::Flutes);
    p.tracks[Harmonic::InstrumentId::Clarinets]    = makeSustainTrack(Harmonic::InstrumentId::Clarinets);

    return p;
}

OrchestralPattern createEpicFanfarePattern() {
    OrchestralPattern p;
    p.name = "Epic Brass Fanfare";
    p.styleCategory = "Fanfare";

    p.tracks[Harmonic::InstrumentId::Trumpets]    = makeOstinatoTrack(Harmonic::InstrumentId::Trumpets, Harmonic::ArticulationType::Marcato);
    p.tracks[Harmonic::InstrumentId::FrenchHorns] = makeOstinatoTrack(Harmonic::InstrumentId::FrenchHorns, Harmonic::ArticulationType::Marcato);
    p.tracks[Harmonic::InstrumentId::Trombones]   = makeSustainTrack(Harmonic::InstrumentId::Trombones);
    p.tracks[Harmonic::InstrumentId::Tuba]        = makeOstinatoTrack(Harmonic::InstrumentId::Tuba, Harmonic::ArticulationType::Marcato);
    p.tracks[Harmonic::InstrumentId::Timpani]     = makeOstinatoTrack(Harmonic::InstrumentId::Timpani, Harmonic::ArticulationType::Marcato);

    return p;
}

OrchestralPattern createSuspenseMysteryPattern() {
    OrchestralPattern p;
    p.name = "Suspense Mystery";
    p.styleCategory = "Thriller";

    p.tracks[Harmonic::InstrumentId::Violins1] = makeOstinatoTrack(Harmonic::InstrumentId::Violins1, Harmonic::ArticulationType::Tremolo);
    p.tracks[Harmonic::InstrumentId::Violins2] = makeOstinatoTrack(Harmonic::InstrumentId::Violins2, Harmonic::ArticulationType::Tremolo);
    p.tracks[Harmonic::InstrumentId::Violas]   = makeOstinatoTrack(Harmonic::InstrumentId::Violas, Harmonic::ArticulationType::Tremolo);
    p.tracks[Harmonic::InstrumentId::Cellos]   = makeSustainTrack(Harmonic::InstrumentId::Cellos);
    p.tracks[Harmonic::InstrumentId::Bassoons] = makeArpTrack(Harmonic::InstrumentId::Bassoons, false);

    return p;
}

} // namespace Sequencer
