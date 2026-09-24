#include "PatternModel.h"

namespace Sequencer {

static std::vector<int> makeDefaultCc1Curve(int peakVal = 100, int minVal = 45) {
    std::vector<int> curve(16);
    // Smooth natural orchestral dynamics swell across 16 steps
    for (int i = 0; i < 16; ++i) {
        double phase = (double)i / 15.0; // 0.0 to 1.0
        // Bell shape curve peaking around step 10
        double factor = std::sin(phase * 3.14159265);
        curve[i] = minVal + static_cast<int>((peakVal - minVal) * factor);
    }
    return curve;
}

static TrackPattern makeConfiguredTrack(Harmonic::InstrumentId id,
                                       const std::string& name,
                                       Harmonic::ArticulationType art,
                                       const std::string& mode,
                                       int octave,
                                       float volume,
                                       const std::vector<int>& stepPitches,
                                       Harmonic::StepActionType action = Harmonic::StepActionType::Ostinato,
                                       int peakCc1 = 95) {
    TrackPattern tp;
    tp.instrument = id;
    tp.trackName = name;
    tp.articulation = art;
    tp.arrangerMode = mode;
    tp.octaveOffset = octave;
    tp.volume = volume;
    tp.isMuted = false;
    tp.isSolo = false;
    tp.stepCount = 16;
    tp.stepDivision = 0.25; // 1/16th note
    tp.cc1Curve = makeDefaultCc1Curve(peakCc1, 40);
    tp.steps.resize(16);

    for (int i = 0; i < 16; ++i) {
        StepDefinition sd;
        sd.action = action;
        sd.articulation = art;
        sd.octaveOffset = 0;

        int pitchOffset = 0;
        if (!stepPitches.empty()) {
            pitchOffset = stepPitches[i % stepPitches.size()];
        }

        if (pitchOffset == -99) { // -99 indicates rest
            sd.active = false;
            sd.action = Harmonic::StepActionType::Rest;
            sd.velocity = 0;
        } else {
            sd.active = true;
            sd.stepOffset = pitchOffset;
            sd.velocity = (i % 4 == 0) ? 105 : ((i % 2 == 0) ? 92 : 82);
            sd.gate = (art == Harmonic::ArticulationType::Sustain) ? 1.0 : 0.6;
        }
        tp.steps[i] = sd;
    }
    return tp;
}

OrchestralPattern createActionOstinatoPattern() {
    OrchestralPattern p;
    p.name = "Action Ostinato";
    p.styleCategory = "Action";
    p.bpm = 130.0;
    p.timeSigNumerator = 4;
    p.timeSigDenominator = 4;
    p.barLength = 2;

    // STRINGS (Matching Hollywood Orchestrator Reference Image 2!)
    // 1st Violin Spiccato: Top voice, Octave +1, Volume 0.85
    p.tracks[Harmonic::InstrumentId::Violins1] =
        makeConfiguredTrack(Harmonic::InstrumentId::Violins1, "1st Violin Spiccato",
                            Harmonic::ArticulationType::Spiccato, "Top", 1, 0.85f,
                            {2, 2, 2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 3, 2}, Harmonic::StepActionType::Ostinato, 110);

    // 2nd Violin Spiccato: Lowest/Counter, Octave +1, Volume 0.9 (Visible in reference grid!)
    p.tracks[Harmonic::InstrumentId::Violins2] =
        makeConfiguredTrack(Harmonic::InstrumentId::Violins2, "2nd Violin Spiccato",
                            Harmonic::ArticulationType::Spiccato, "Lowest", 1, 0.90f,
                            {2, 1, 2, 3, 2, 1, 0, -1, 2, 1, 2, 3, 2, 1, 0, 1}, Harmonic::StepActionType::Ostinato, 105);

    // Viola Spiccato: Lowest, Octave 0, Volume 0.85
    p.tracks[Harmonic::InstrumentId::Violas] =
        makeConfiguredTrack(Harmonic::InstrumentId::Violas, "Viola Spiccato",
                            Harmonic::ArticulationType::Spiccato, "Lowest", 0, 0.85f,
                            {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 2, 1}, Harmonic::StepActionType::Ostinato, 95);

    // Celli Legato: Top, Octave 0, Volume 0.90
    p.tracks[Harmonic::InstrumentId::Cellos] =
        makeConfiguredTrack(Harmonic::InstrumentId::Cellos, "Celli Legato",
                            Harmonic::ArticulationType::Sustain, "Top", 0, 0.90f,
                            {0, -99, 0, -99, 1, -99, 1, -99, 0, -99, 0, -99, -1, -99, -1, -99}, Harmonic::StepActionType::Ostinato, 100);

    // Basses Sustain: Lowest, Octave -2, Volume 0.95
    p.tracks[Harmonic::InstrumentId::DoubleBasses] =
        makeConfiguredTrack(Harmonic::InstrumentId::DoubleBasses, "Basses Sustain",
                            Harmonic::ArticulationType::Sustain, "Lowest", -2, 0.95f,
                            {0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -2, -2, -2, -2, -2, -2}, Harmonic::StepActionType::Sustain, 115);

    // BRASS
    p.tracks[Harmonic::InstrumentId::FrenchHorns] =
        makeConfiguredTrack(Harmonic::InstrumentId::FrenchHorns, "French Horns",
                            Harmonic::ArticulationType::Sustain, "Chord", 0, 0.85f,
                            {1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2}, Harmonic::StepActionType::Sustain, 100);

    p.tracks[Harmonic::InstrumentId::Trumpets] =
        makeConfiguredTrack(Harmonic::InstrumentId::Trumpets, "Trumpets",
                            Harmonic::ArticulationType::Staccato, "Top", 1, 0.80f,
                            {2, -99, 2, 3, 2, -99, 2, 4, 3, -99, 2, 3, 2, 1, 2, 3}, Harmonic::StepActionType::Ostinato, 105);

    p.tracks[Harmonic::InstrumentId::Trombones] =
        makeConfiguredTrack(Harmonic::InstrumentId::Trombones, "Trombones",
                            Harmonic::ArticulationType::Sustain, "Lowest", -1, 0.85f,
                            {0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1}, Harmonic::StepActionType::Sustain, 95);

    p.tracks[Harmonic::InstrumentId::Tuba] =
        makeConfiguredTrack(Harmonic::InstrumentId::Tuba, "Tuba",
                            Harmonic::ArticulationType::Marcato, "Root", -2, 0.90f,
                            {0, -99, -99, -99, 0, -99, -99, -99, -1, -99, -99, -99, -1, -99, -99, -99}, Harmonic::StepActionType::Ostinato, 110);

    // WOODWINDS
    p.tracks[Harmonic::InstrumentId::Flutes] =
        makeConfiguredTrack(Harmonic::InstrumentId::Flutes, "Flutes",
                            Harmonic::ArticulationType::Runs, "Arp Up", 2, 0.80f,
                            {1, 2, 3, 4, 3, 2, 1, 2, 3, 4, 5, 4, 3, 2, 3, 4}, Harmonic::StepActionType::ArpUp, 90);

    p.tracks[Harmonic::InstrumentId::Oboes] =
        makeConfiguredTrack(Harmonic::InstrumentId::Oboes, "Oboes",
                            Harmonic::ArticulationType::Sustain, "Top", 1, 0.75f,
                            {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3}, Harmonic::StepActionType::Sustain, 85);

    p.tracks[Harmonic::InstrumentId::Clarinets] =
        makeConfiguredTrack(Harmonic::InstrumentId::Clarinets, "Clarinets",
                            Harmonic::ArticulationType::Staccato, "Arp Down", 1, 0.75f,
                            {4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 3, 2, 1, 0}, Harmonic::StepActionType::ArpDown, 85);

    p.tracks[Harmonic::InstrumentId::Bassoons] =
        makeConfiguredTrack(Harmonic::InstrumentId::Bassoons, "Bassoons",
                            Harmonic::ArticulationType::Staccato, "Lowest", -1, 0.80f,
                            {0, 1, 0, 1, 0, 1, 0, 1, -1, 0, -1, 0, -1, 0, -1, 0}, Harmonic::StepActionType::Ostinato, 90);

    // PERCUSSION
    p.tracks[Harmonic::InstrumentId::Timpani] =
        makeConfiguredTrack(Harmonic::InstrumentId::Timpani, "Timpani",
                            Harmonic::ArticulationType::Marcato, "Root", -1, 0.95f,
                            {0, -99, -99, -99, 0, -99, -99, -99, 0, -99, 0, -99, 0, 0, 0, 0}, Harmonic::StepActionType::Ostinato, 120);

    p.tracks[Harmonic::InstrumentId::OrchestralPerc] =
        makeConfiguredTrack(Harmonic::InstrumentId::OrchestralPerc, "Percussion",
                            Harmonic::ArticulationType::Staccato, "Root", 0, 0.85f,
                            {0, -99, 0, -99, 0, -99, 0, -99, 0, 0, 0, -99, 0, -99, 0, 0}, Harmonic::StepActionType::Ostinato, 110);

    return p;
}

OrchestralPattern createEpicFanfarePattern() {
    OrchestralPattern p;
    p.name = "Epic Brass Fanfare";
    p.styleCategory = "Fanfare";
    p.bpm = 115.0;
    p.timeSigNumerator = 4;
    p.timeSigDenominator = 4;
    p.barLength = 2;

    p.tracks[Harmonic::InstrumentId::Trumpets] =
        makeConfiguredTrack(Harmonic::InstrumentId::Trumpets, "Trumpets",
                            Harmonic::ArticulationType::Marcato, "Top", 1, 0.95f,
                            {0, 0, 2, 2, 4, 4, 3, 2, 4, -99, 3, -99, 2, 1, 0, 2}, Harmonic::StepActionType::Ostinato, 120);

    p.tracks[Harmonic::InstrumentId::FrenchHorns] =
        makeConfiguredTrack(Harmonic::InstrumentId::FrenchHorns, "French Horns",
                            Harmonic::ArticulationType::Marcato, "Chord", 0, 0.90f,
                            {1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2}, Harmonic::StepActionType::Ostinato, 115);

    p.tracks[Harmonic::InstrumentId::Trombones] =
        makeConfiguredTrack(Harmonic::InstrumentId::Trombones, "Trombones",
                            Harmonic::ArticulationType::Marcato, "Lowest", -1, 0.90f,
                            {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, -1, -1, -1, -1}, Harmonic::StepActionType::Ostinato, 110);

    p.tracks[Harmonic::InstrumentId::Tuba] =
        makeConfiguredTrack(Harmonic::InstrumentId::Tuba, "Tuba",
                            Harmonic::ArticulationType::Marcato, "Root", -2, 0.95f,
                            {0, -99, 0, -99, 0, -99, 0, -99, -1, -99, -1, -99, -1, -99, 0, -99}, Harmonic::StepActionType::Ostinato, 125);

    // Strings supporting
    p.tracks[Harmonic::InstrumentId::Violins1] =
        makeConfiguredTrack(Harmonic::InstrumentId::Violins1, "1st Violin Legato",
                            Harmonic::ArticulationType::Tremolo, "Top", 1, 0.75f,
                            {4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 4, 4, 3, 2}, Harmonic::StepActionType::Sustain, 90);

    p.tracks[Harmonic::InstrumentId::Violins2] =
        makeConfiguredTrack(Harmonic::InstrumentId::Violins2, "2nd Violin Spiccato",
                            Harmonic::ArticulationType::Tremolo, "Top", 1, 0.70f,
                            {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0}, Harmonic::StepActionType::Sustain, 85);

    p.tracks[Harmonic::InstrumentId::Violas] =
        makeConfiguredTrack(Harmonic::InstrumentId::Violas, "Viola Spiccato",
                            Harmonic::ArticulationType::Tremolo, "Lowest", 0, 0.70f,
                            {1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 0, -1}, Harmonic::StepActionType::Sustain, 85);

    p.tracks[Harmonic::InstrumentId::Cellos] =
        makeConfiguredTrack(Harmonic::InstrumentId::Cellos, "Celli Legato",
                            Harmonic::ArticulationType::Marcato, "Lowest", -1, 0.85f,
                            {0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1}, Harmonic::StepActionType::Ostinato, 105);

    p.tracks[Harmonic::InstrumentId::DoubleBasses] =
        makeConfiguredTrack(Harmonic::InstrumentId::DoubleBasses, "Basses Sustain",
                            Harmonic::ArticulationType::Marcato, "Lowest", -2, 0.90f,
                            {0, -99, 0, -99, 0, -99, 0, -99, -1, -99, -1, -99, -1, -99, -1, -99}, Harmonic::StepActionType::Ostinato, 115);

    p.tracks[Harmonic::InstrumentId::Timpani] =
        makeConfiguredTrack(Harmonic::InstrumentId::Timpani, "Timpani",
                            Harmonic::ArticulationType::Marcato, "Root", -1, 1.0f,
                            {0, 0, 0, -99, 0, 0, 0, -99, 0, 0, 0, 0, 0, 0, 0, 0}, Harmonic::StepActionType::Ostinato, 127);

    return p;
}

OrchestralPattern createLyricalAdagioPattern() {
    OrchestralPattern p;
    p.name = "Lyrical Adagio";
    p.styleCategory = "Romantic";
    p.bpm = 72.0;
    p.timeSigNumerator = 4;
    p.timeSigDenominator = 4;
    p.barLength = 2;

    p.tracks[Harmonic::InstrumentId::Violins1] =
        makeConfiguredTrack(Harmonic::InstrumentId::Violins1, "1st Violin Legato",
                            Harmonic::ArticulationType::Sustain, "Top", 1, 0.90f,
                            {3, 3, 3, 3, 4, 4, 4, 4, 3, 3, 2, 2, 1, 1, 2, 3}, Harmonic::StepActionType::Sustain, 90);

    p.tracks[Harmonic::InstrumentId::Violins2] =
        makeConfiguredTrack(Harmonic::InstrumentId::Violins2, "2nd Violin Spiccato",
                            Harmonic::ArticulationType::Sustain, "Top", 1, 0.80f,
                            {1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 0, 0, -1, -1, 0, 1}, Harmonic::StepActionType::Sustain, 80);

    p.tracks[Harmonic::InstrumentId::Violas] =
        makeConfiguredTrack(Harmonic::InstrumentId::Violas, "Viola Spiccato",
                            Harmonic::ArticulationType::Sustain, "Lowest", 0, 0.80f,
                            {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, -1, -1, -2, -2, -1, 0}, Harmonic::StepActionType::Sustain, 80);

    p.tracks[Harmonic::InstrumentId::Cellos] =
        makeConfiguredTrack(Harmonic::InstrumentId::Cellos, "Celli Legato",
                            Harmonic::ArticulationType::Sustain, "Lowest", -1, 0.85f,
                            {0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1}, Harmonic::StepActionType::Sustain, 85);

    p.tracks[Harmonic::InstrumentId::DoubleBasses] =
        makeConfiguredTrack(Harmonic::InstrumentId::DoubleBasses, "Basses Sustain",
                            Harmonic::ArticulationType::Sustain, "Lowest", -2, 0.85f,
                            {0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -2, -2, -2, -2, -2, -2}, Harmonic::StepActionType::Sustain, 90);

    p.tracks[Harmonic::InstrumentId::FrenchHorns] =
        makeConfiguredTrack(Harmonic::InstrumentId::FrenchHorns, "French Horns",
                            Harmonic::ArticulationType::Sustain, "Chord", 0, 0.70f,
                            {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1}, Harmonic::StepActionType::Sustain, 75);

    p.tracks[Harmonic::InstrumentId::Flutes] =
        makeConfiguredTrack(Harmonic::InstrumentId::Flutes, "Flutes",
                            Harmonic::ArticulationType::Sustain, "Top", 2, 0.75f,
                            {5, 5, 5, 5, 6, 6, 6, 6, 5, 5, 4, 4, 3, 3, 4, 5}, Harmonic::StepActionType::Sustain, 80);

    p.tracks[Harmonic::InstrumentId::Clarinets] =
        makeConfiguredTrack(Harmonic::InstrumentId::Clarinets, "Clarinets",
                            Harmonic::ArticulationType::Sustain, "Chord", 1, 0.70f,
                            {2, 2, 2, 2, 3, 3, 3, 3, 2, 2, 1, 1, 0, 0, 1, 2}, Harmonic::StepActionType::Sustain, 70);

    return p;
}

OrchestralPattern createSuspenseMysteryPattern() {
    OrchestralPattern p;
    p.name = "Suspense Mystery";
    p.styleCategory = "Thriller";
    p.bpm = 90.0;
    p.timeSigNumerator = 4;
    p.timeSigDenominator = 4;
    p.barLength = 2;

    p.tracks[Harmonic::InstrumentId::Violins1] =
        makeConfiguredTrack(Harmonic::InstrumentId::Violins1, "1st Violin Legato",
                            Harmonic::ArticulationType::Tremolo, "Top", 1, 0.85f,
                            {3, 3, 4, 3, 2, 3, 4, 5, 4, 3, 2, 1, 2, 3, 4, 3}, Harmonic::StepActionType::Ostinato, 95);

    p.tracks[Harmonic::InstrumentId::Violins2] =
        makeConfiguredTrack(Harmonic::InstrumentId::Violins2, "2nd Violin Spiccato",
                            Harmonic::ArticulationType::Tremolo, "Top", 1, 0.80f,
                            {1, 1, 2, 1, 0, 1, 2, 3, 2, 1, 0, -1, 0, 1, 2, 1}, Harmonic::StepActionType::Ostinato, 90);

    p.tracks[Harmonic::InstrumentId::Violas] =
        makeConfiguredTrack(Harmonic::InstrumentId::Violas, "Viola Spiccato",
                            Harmonic::ArticulationType::Pizzicato, "Lowest", 0, 0.80f,
                            {0, -99, 1, -99, 0, -99, -1, -99, 0, -99, 1, -99, 0, -99, -1, -99}, Harmonic::StepActionType::Ostinato, 85);

    p.tracks[Harmonic::InstrumentId::Cellos] =
        makeConfiguredTrack(Harmonic::InstrumentId::Cellos, "Celli Legato",
                            Harmonic::ArticulationType::Sustain, "Lowest", -1, 0.85f,
                            {0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -2, -2, -2, -2}, Harmonic::StepActionType::Sustain, 90);

    p.tracks[Harmonic::InstrumentId::Bassoons] =
        makeConfiguredTrack(Harmonic::InstrumentId::Bassoons, "Bassoons",
                            Harmonic::ArticulationType::Staccato, "Arp Down", -1, 0.75f,
                            {2, 1, 0, -1, 2, 1, 0, -1, 2, 1, 0, -1, 1, 0, -1, -2}, Harmonic::StepActionType::ArpDown, 80);

    p.tracks[Harmonic::InstrumentId::DoubleBasses] =
        makeConfiguredTrack(Harmonic::InstrumentId::DoubleBasses, "Basses Sustain",
                            Harmonic::ArticulationType::Sustain, "Lowest", -2, 0.85f,
                            {0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -2, -2, -2, -2, -2, -2}, Harmonic::StepActionType::Sustain, 95);

    return p;
}

OrchestralPattern createWarDrumsPattern() {
    OrchestralPattern p;
    p.name = "War Drums & Percussion";
    p.styleCategory = "Action";
    p.bpm = 140.0;
    p.timeSigNumerator = 4;
    p.timeSigDenominator = 4;
    p.barLength = 2;

    p.tracks[Harmonic::InstrumentId::Timpani] =
        makeConfiguredTrack(Harmonic::InstrumentId::Timpani, "Timpani",
                            Harmonic::ArticulationType::Marcato, "Root", -1, 1.0f,
                            {0, 0, -99, 0, 0, -99, 0, 0, 0, 0, 0, 0, 0, -99, 0, 0}, Harmonic::StepActionType::Ostinato, 127);

    p.tracks[Harmonic::InstrumentId::OrchestralPerc] =
        makeConfiguredTrack(Harmonic::InstrumentId::OrchestralPerc, "Percussion",
                            Harmonic::ArticulationType::Marcato, "Root", 0, 0.95f,
                            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, Harmonic::StepActionType::Ostinato, 120);

    p.tracks[Harmonic::InstrumentId::Cellos] =
        makeConfiguredTrack(Harmonic::InstrumentId::Cellos, "Celli Legato",
                            Harmonic::ArticulationType::Marcato, "Root", -1, 0.90f,
                            {0, -99, 0, -99, 0, -99, 0, -99, 0, 0, 0, -99, 0, -99, 0, 0}, Harmonic::StepActionType::Ostinato, 115);

    p.tracks[Harmonic::InstrumentId::DoubleBasses] =
        makeConfiguredTrack(Harmonic::InstrumentId::DoubleBasses, "Basses Sustain",
                            Harmonic::ArticulationType::Marcato, "Root", -2, 0.95f,
                            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, Harmonic::StepActionType::Ostinato, 120);

    p.tracks[Harmonic::InstrumentId::FrenchHorns] =
        makeConfiguredTrack(Harmonic::InstrumentId::FrenchHorns, "French Horns",
                            Harmonic::ArticulationType::Marcato, "Chord", 0, 0.90f,
                            {1, -99, 1, -99, 2, -99, 2, -99, 1, 1, 1, -99, 2, 2, 2, 2}, Harmonic::StepActionType::Ostinato, 110);

    return p;
}

OrchestralPattern createFantasyAdventurePattern() {
    OrchestralPattern p;
    p.name = "Fantasy Adventure";
    p.styleCategory = "Adventure";
    p.bpm = 124.0;
    p.timeSigNumerator = 4;
    p.timeSigDenominator = 4;
    p.barLength = 2;

    p.tracks[Harmonic::InstrumentId::Violins1] =
        makeConfiguredTrack(Harmonic::InstrumentId::Violins1, "1st Violin Legato",
                            Harmonic::ArticulationType::Spiccato, "Top", 1, 0.90f,
                            {1, 2, 3, 2, 1, 0, 1, 2, 3, 4, 3, 2, 1, 2, 3, 4}, Harmonic::StepActionType::Ostinato, 105);

    p.tracks[Harmonic::InstrumentId::Violins2] =
        makeConfiguredTrack(Harmonic::InstrumentId::Violins2, "2nd Violin Spiccato",
                            Harmonic::ArticulationType::Spiccato, "Top", 1, 0.85f,
                            {0, 1, 2, 1, 0, -1, 0, 1, 2, 3, 2, 1, 0, 1, 2, 3}, Harmonic::StepActionType::Ostinato, 100);

    p.tracks[Harmonic::InstrumentId::Flutes] =
        makeConfiguredTrack(Harmonic::InstrumentId::Flutes, "Flutes",
                            Harmonic::ArticulationType::Runs, "Arp Up", 2, 0.85f,
                            {2, 3, 4, 5, 4, 3, 2, 3, 4, 5, 6, 5, 4, 3, 4, 5}, Harmonic::StepActionType::ArpUp, 95);

    p.tracks[Harmonic::InstrumentId::FrenchHorns] =
        makeConfiguredTrack(Harmonic::InstrumentId::FrenchHorns, "French Horns",
                            Harmonic::ArticulationType::Sustain, "Chord", 0, 0.85f,
                            {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1}, Harmonic::StepActionType::Sustain, 95);

    p.tracks[Harmonic::InstrumentId::Cellos] =
        makeConfiguredTrack(Harmonic::InstrumentId::Cellos, "Celli Legato",
                            Harmonic::ArticulationType::Marcato, "Lowest", -1, 0.85f,
                            {0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1}, Harmonic::StepActionType::Ostinato, 100);

    p.tracks[Harmonic::InstrumentId::DoubleBasses] =
        makeConfiguredTrack(Harmonic::InstrumentId::DoubleBasses, "Basses Sustain",
                            Harmonic::ArticulationType::Marcato, "Lowest", -2, 0.90f,
                            {0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -2, -2, -2, -2, -2, -2}, Harmonic::StepActionType::Sustain, 110);

    return p;
}

} // namespace Sequencer
