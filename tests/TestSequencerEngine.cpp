#include "TestHarness.h"
#include "Sequencer/PatternModel.h"
#include "Sequencer/SequencerEngine.h"
#include "Orchestration/VoicingEngine.h"

TEST_CASE(SequencerEngine, PatternModelDefaults) {
    auto pattern = Sequencer::createActionOstinatoPattern();
    ASSERT_EQ(pattern.name, "Action Ostinato");
    ASSERT_TRUE(pattern.tracks.find(Harmonic::InstrumentId::Violins1) != pattern.tracks.end());

    const auto& vlnTrack = pattern.tracks[Harmonic::InstrumentId::Violins1];
    ASSERT_EQ(vlnTrack.steps.size(), 16);
    ASSERT_EQ(vlnTrack.steps[0].action, Harmonic::StepActionType::Ostinato);
    ASSERT_EQ(vlnTrack.steps[0].articulation, Harmonic::ArticulationType::Spiccato);
}

TEST_CASE(SequencerEngine, StepGenerationAndMidiEvents) {
    Sequencer::SequencerEngine seq;
    auto pattern = Sequencer::createActionOstinatoPattern();
    seq.setPattern(pattern);
    seq.setTempo(120.0);

    // Setup input chord C Major
    Harmonic::HarmonicFrame cMaj;
    cMaj.rootPitchClass = 0;
    cMaj.bassMidiNote = 48;
    cMaj.quality = Harmonic::ChordQuality::MajorTriad;
    cMaj.pitches = {48, 52, 55};
    cMaj.chordTones = {0, 4, 7};

    Orchestration::VoicingEngine ve;
    auto voicing = ve.generateVoicing(cMaj);
    seq.updateVoicing(voicing);

    // Render a 1/16th step slice at sampleRate 44100, bufferSize 512
    // At 120 BPM: 1 beat = 0.5s = 22050 samples. 1/16th = 0.125s = 5512.5 samples.
    // Advancing 5513 samples should produce NoteOn events for ostinato tracks!
    std::vector<Sequencer::ScheduledMidiEvent> midiEvents;
    seq.processBlock(5513, 44100.0, 0.0, true, midiEvents);

    ASSERT_TRUE(!midiEvents.empty());

    // Check that we have NoteOn for Violins 1
    bool hasVln1 = false;
    for (const auto& ev : midiEvents) {
        if (ev.channel == 1 && ev.isNoteOn) {
            hasVln1 = true;
            ASSERT_TRUE(ev.velocity > 0);
            ASSERT_TRUE(ev.pitch >= 55); // within violin range
        }
    }
    ASSERT_TRUE(hasVln1);
}

TEST_CASE(SequencerEngine, NoteLengthAndEmptyTrackEditing) {
    Sequencer::SequencerEngine seq;
    // Test editing an empty/unpopulated track (e.g. Tuba)
    seq.setTrackStep(Harmonic::InstrumentId::Tuba, 0, true, 0, 110, Harmonic::ArticulationType::Marcato);
    seq.setTrackStepLength(Harmonic::InstrumentId::Tuba, 0, 4); // 4 steps long
    seq.setTrackArticulation(Harmonic::InstrumentId::Tuba, Harmonic::ArticulationType::Marcato);
    seq.setTrackPan(Harmonic::InstrumentId::Tuba, -0.25f);

    auto p = seq.getPattern();
    ASSERT_TRUE(p.tracks.find(Harmonic::InstrumentId::Tuba) != p.tracks.end());
    const auto& tubaTrk = p.tracks[Harmonic::InstrumentId::Tuba];
    ASSERT_EQ(tubaTrk.steps[0].lengthSteps, 4);
    ASSERT_EQ(tubaTrk.articulation, Harmonic::ArticulationType::Marcato);
    ASSERT_TRUE(std::abs(tubaTrk.pan - (-0.25f)) < 0.001f);

    // Test addTrack and removeTrack
    seq.addTrack(Harmonic::InstrumentId::Flutes, "Solo Concert Flute", Harmonic::OrchestralSection::Woodwinds, 10, Harmonic::ArticulationType::Sustain);
    p = seq.getPattern();
    ASSERT_TRUE(p.tracks.find(Harmonic::InstrumentId::Flutes) != p.tracks.end());
    ASSERT_EQ(p.tracks[Harmonic::InstrumentId::Flutes].midiChannel, 10);

    seq.removeTrack(Harmonic::InstrumentId::Flutes);
    p = seq.getPattern();
    ASSERT_TRUE(p.tracks.find(Harmonic::InstrumentId::Flutes) == p.tracks.end());
}

TEST_CASE(SequencerEngine, PatternJsonSerialization) {
    auto original = Sequencer::createActionOstinatoPattern();
    original.bpm = 138.0;
    original.tracks[Harmonic::InstrumentId::Violins1].steps[0].lengthSteps = 3;
    original.tracks[Harmonic::InstrumentId::Violins1].pan = 0.5f;

    std::string jsonStr = original.toJson();
    ASSERT_TRUE(!jsonStr.empty());
    ASSERT_TRUE(jsonStr.find("Action Ostinato") != std::string::npos);
    ASSERT_TRUE(jsonStr.find("\"lengthSteps\": 3") != std::string::npos);

    auto restored = Sequencer::OrchestralPattern::fromJson(jsonStr);
    ASSERT_EQ(restored.name, original.name);
    ASSERT_EQ(restored.bpm, 138.0);
    ASSERT_EQ(restored.tracks.size(), original.tracks.size());
    ASSERT_EQ(restored.tracks[Harmonic::InstrumentId::Violins1].steps[0].lengthSteps, 3);
    ASSERT_TRUE(std::abs(restored.tracks[Harmonic::InstrumentId::Violins1].pan - 0.5f) < 0.001f);
}

