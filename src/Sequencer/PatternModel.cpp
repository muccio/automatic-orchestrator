#include "PatternModel.h"
#include <sstream>
#include <fstream>
#include <cctype>
#include <iomanip>


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
    tp.section = Harmonic::getInstrumentSection(id);
    tp.midiChannel = Harmonic::getDefaultInstrumentChannel(id);
    tp.articulation = art;
    tp.arrangerMode = mode;
    tp.octaveOffset = octave;
    tp.volume = volume;
    tp.pan = 0.0f;
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
        sd.lengthSteps = 1;

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
            sd.gate = (art == Harmonic::ArticulationType::Sustain) ? 1.0 : 0.85;
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

static std::string escapeJson(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\b') o << "\\b";
        else if (c == '\f') o << "\\f";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else o << c;
    }
    return o.str();
}

struct SimpleJson {
    enum Type { Null, Bool, Number, String, Array, Object };
    Type type = Null;
    bool bVal = false;
    double nVal = 0.0;
    std::string sVal;
    std::vector<SimpleJson> arr;
    std::map<std::string, SimpleJson> obj;

    const SimpleJson& operator[](const std::string& k) const {
        static const SimpleJson empty;
        if (type != Object) return empty;
        auto it = obj.find(k);
        return it != obj.end() ? it->second : empty;
    }

    const SimpleJson& operator[](size_t idx) const {
        static const SimpleJson empty;
        if (type != Array || idx >= arr.size()) return empty;
        return arr[idx];
    }

    bool asBool(bool def = false) const { return type == Bool ? bVal : def; }
    int asInt(int def = 0) const { return type == Number ? static_cast<int>(nVal) : def; }
    double asDouble(double def = 0.0) const { return type == Number ? nVal : def; }
    std::string asString(const std::string& def = "") const { return type == String ? sVal : def; }
};

static void skipWs(const std::string& s, size_t& i) {
    while (i < s.size() && (std::isspace(static_cast<unsigned char>(s[i])) || s[i] == '\0')) {
        ++i;
    }
}

static SimpleJson parseJson(const std::string& s, size_t& i);

static std::string parseString(const std::string& s, size_t& i) {
    std::string res;
    if (i >= s.size() || s[i] != '"') return res;
    ++i;
    while (i < s.size() && s[i] != '"') {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i;
            if (s[i] == 'n') res += '\n';
            else if (s[i] == 'r') res += '\r';
            else if (s[i] == 't') res += '\t';
            else if (s[i] == '"') res += '"';
            else if (s[i] == '\\') res += '\\';
            else res += s[i];
        } else {
            res += s[i];
        }
        ++i;
    }
    if (i < s.size() && s[i] == '"') ++i;
    return res;
}

static SimpleJson parseJson(const std::string& s, size_t& i) {
    skipWs(s, i);
    SimpleJson val;
    if (i >= s.size()) return val;

    char c = s[i];
    if (c == '{') {
        val.type = SimpleJson::Object;
        ++i;
        while (i < s.size()) {
            skipWs(s, i);
            if (i < s.size() && s[i] == '}') { ++i; break; }
            std::string key = parseString(s, i);
            skipWs(s, i);
            if (i < s.size() && s[i] == ':') ++i;
            skipWs(s, i);
            val.obj[key] = parseJson(s, i);
            skipWs(s, i);
            if (i < s.size() && s[i] == ',') ++i;
            else if (i < s.size() && s[i] == '}') { ++i; break; }
        }
    } else if (c == '[') {
        val.type = SimpleJson::Array;
        ++i;
        while (i < s.size()) {
            skipWs(s, i);
            if (i < s.size() && s[i] == ']') { ++i; break; }
            val.arr.push_back(parseJson(s, i));
            skipWs(s, i);
            if (i < s.size() && s[i] == ',') ++i;
            else if (i < s.size() && s[i] == ']') { ++i; break; }
        }
    } else if (c == '"') {
        val.type = SimpleJson::String;
        val.sVal = parseString(s, i);
    } else if (c == 't' && s.compare(i, 4, "true") == 0) {
        val.type = SimpleJson::Bool;
        val.bVal = true;
        i += 4;
    } else if (c == 'f' && s.compare(i, 5, "false") == 0) {
        val.type = SimpleJson::Bool;
        val.bVal = false;
        i += 5;
    } else if (c == 'n' && s.compare(i, 4, "null") == 0) {
        val.type = SimpleJson::Null;
        i += 4;
    } else if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
        val.type = SimpleJson::Number;
        size_t start = i;
        if (s[i] == '-') ++i;
        while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.' || s[i] == 'e' || s[i] == 'E' || s[i] == '+' || s[i] == '-')) {
            ++i;
        }
        try {
            val.nVal = std::stod(s.substr(start, i - start));
        } catch (...) {
            val.nVal = 0.0;
        }
    }
    return val;
}

std::string OrchestralPattern::toJson() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"name\": \"" << escapeJson(name) << "\",\n";
    ss << "  \"styleCategory\": \"" << escapeJson(styleCategory) << "\",\n";
    ss << "  \"bpm\": " << bpm << ",\n";
    ss << "  \"timeSigNumerator\": " << timeSigNumerator << ",\n";
    ss << "  \"timeSigDenominator\": " << timeSigDenominator << ",\n";
    ss << "  \"barLength\": " << barLength << ",\n";
    ss << "  \"tracks\": [\n";
    size_t tIdx = 0;
    for (const auto& [inst, trk] : tracks) {
        ss << "    {\n";
        ss << "      \"instrument\": " << static_cast<int>(trk.instrument) << ",\n";
        ss << "      \"trackName\": \"" << escapeJson(trk.trackName) << "\",\n";
        ss << "      \"section\": " << static_cast<int>(trk.section) << ",\n";
        ss << "      \"midiChannel\": " << trk.midiChannel << ",\n";
        ss << "      \"articulation\": " << static_cast<int>(trk.articulation) << ",\n";
        ss << "      \"arrangerMode\": \"" << escapeJson(trk.arrangerMode) << "\",\n";
        ss << "      \"octaveOffset\": " << trk.octaveOffset << ",\n";
        ss << "      \"volume\": " << trk.volume << ",\n";
        ss << "      \"pan\": " << trk.pan << ",\n";
        ss << "      \"isMuted\": " << (trk.isMuted ? "true" : "false") << ",\n";
        ss << "      \"isSolo\": " << (trk.isSolo ? "true" : "false") << ",\n";
        ss << "      \"stepCount\": " << trk.stepCount << ",\n";
        ss << "      \"stepDivision\": " << trk.stepDivision << ",\n";
        ss << "      \"cc1Curve\": [";
        for (size_t c = 0; c < trk.cc1Curve.size(); ++c) {
            ss << trk.cc1Curve[c] << (c + 1 < trk.cc1Curve.size() ? ", " : "");
        }
        ss << "],\n";
        ss << "      \"steps\": [\n";
        for (size_t s = 0; s < trk.steps.size(); ++s) {
            const auto& stp = trk.steps[s];
            ss << "        {\"active\": " << (stp.active ? "true" : "false")
               << ", \"stepOffset\": " << stp.stepOffset;
            if (!stp.extraOffsets.empty()) {
                ss << ", \"extraOffsets\": [";
                for (size_t e = 0; e < stp.extraOffsets.size(); ++e) {
                    ss << stp.extraOffsets[e] << (e + 1 < stp.extraOffsets.size() ? ", " : "");
                }
                ss << "]";
            }
            ss << ", \"lengthSteps\": " << stp.lengthSteps
               << ", \"velocity\": " << stp.velocity
               << ", \"gate\": " << stp.gate
               << ", \"octaveOffset\": " << stp.octaveOffset
               << ", \"action\": " << static_cast<int>(stp.action)
               << ", \"articulation\": " << static_cast<int>(stp.articulation)
               << "}" << (s + 1 < trk.steps.size() ? ",\n" : "\n");
        }
        ss << "      ]\n";
        ss << "    }" << (++tIdx < tracks.size() ? ",\n" : "\n");
    }
    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

OrchestralPattern OrchestralPattern::fromJson(const std::string& jsonStr) {
    size_t idx = 0;
    SimpleJson root = parseJson(jsonStr, idx);
    if (root.type != SimpleJson::Object) {
        return createActionOstinatoPattern();
    }

    OrchestralPattern p;
    p.name = root["name"].asString("Custom Pattern");
    p.styleCategory = root["styleCategory"].asString("Custom");
    p.bpm = root["bpm"].asDouble(120.0);
    p.timeSigNumerator = root["timeSigNumerator"].asInt(4);
    p.timeSigDenominator = root["timeSigDenominator"].asInt(4);
    p.barLength = root["barLength"].asInt(2);

    const auto& trksNode = root["tracks"];
    if (trksNode.type == SimpleJson::Array) {
        for (const auto& trkNode : trksNode.arr) {
            TrackPattern tp;
            tp.instrument = static_cast<Harmonic::InstrumentId>(trkNode["instrument"].asInt(0));
            tp.trackName = trkNode["trackName"].asString(Harmonic::instrumentToString(tp.instrument));
            tp.section = static_cast<Harmonic::OrchestralSection>(trkNode["section"].asInt(static_cast<int>(Harmonic::getInstrumentSection(tp.instrument))));
            tp.midiChannel = trkNode["midiChannel"].asInt(Harmonic::getDefaultInstrumentChannel(tp.instrument));
            tp.articulation = static_cast<Harmonic::ArticulationType>(trkNode["articulation"].asInt(1));
            tp.arrangerMode = trkNode["arrangerMode"].asString("Top");
            tp.octaveOffset = trkNode["octaveOffset"].asInt(0);
            tp.volume = static_cast<float>(trkNode["volume"].asDouble(0.85));
            tp.pan = static_cast<float>(trkNode["pan"].asDouble(0.0));
            tp.isMuted = trkNode["isMuted"].asBool(false);
            tp.isSolo = trkNode["isSolo"].asBool(false);
            tp.stepCount = trkNode["stepCount"].asInt(16);
            tp.stepDivision = trkNode["stepDivision"].asDouble(0.25);

            const auto& cc1Node = trkNode["cc1Curve"];
            if (cc1Node.type == SimpleJson::Array) {
                for (const auto& c : cc1Node.arr) {
                    tp.cc1Curve.push_back(c.asInt(64));
                }
            }
            if (tp.cc1Curve.empty()) {
                tp.cc1Curve = makeDefaultCc1Curve(95, 40);
            }

            const auto& stepsNode = trkNode["steps"];
            if (stepsNode.type == SimpleJson::Array) {
                for (const auto& sNode : stepsNode.arr) {
                    StepDefinition sd;
                    sd.active = sNode["active"].asBool(true);
                    sd.stepOffset = sNode["stepOffset"].asInt(0);
                    const auto& extraArr = sNode["extraOffsets"];
                    if (extraArr.type == SimpleJson::Array) {
                        for (const auto& eo : extraArr.arr) {
                            sd.extraOffsets.push_back(eo.asInt(0));
                        }
                    }
                    sd.lengthSteps = std::clamp(sNode["lengthSteps"].asInt(1), 1, 16);
                    sd.velocity = sNode["velocity"].asInt(90);
                    sd.gate = sNode["gate"].asDouble(0.85);
                    sd.octaveOffset = sNode["octaveOffset"].asInt(0);
                    sd.action = static_cast<Harmonic::StepActionType>(sNode["action"].asInt(0));
                    sd.articulation = static_cast<Harmonic::ArticulationType>(sNode["articulation"].asInt(static_cast<int>(tp.articulation)));
                    tp.steps.push_back(sd);
                }
            }
            if (tp.steps.empty()) {
                tp.steps.resize(16);
            }
            p.tracks[tp.instrument] = tp;
        }
    }
    return p;
}

bool OrchestralPattern::saveToFile(const std::string& filePath) const {
    std::ofstream out(filePath);
    if (!out.is_open()) return false;
    out << toJson();
    return true;
}

OrchestralPattern OrchestralPattern::loadFromFile(const std::string& filePath) {
    std::ifstream in(filePath);
    if (!in.is_open()) return createActionOstinatoPattern();
    std::stringstream buffer;
    buffer << in.rdbuf();
    return fromJson(buffer.str());
}

} // namespace Sequencer
