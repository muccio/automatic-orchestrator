#include "TestHarness.h"
#include "Harmonic/GraceWindowBuffer.h"
#include "Harmonic/ChordDetector.h"
#include "Harmonic/ScaleQuantizer.h"
#include "Orchestration/VoicingEngine.h"
#include "Orchestration/HarmonicRandomizer.h"
#include "Sequencer/SequencerEngine.h"
#include "Articulation/ArticulationMap.h"
#include "MidiExport/StandardMidiWriter.h"
#include <fstream>

TEST_CASE(IntegrationE2E, CompleteCinematicWorkflow) {
    // 1. Initialize complete engine pipeline
    Harmonic::GraceWindowBuffer grace;
    Harmonic::ChordDetector detector;
    Harmonic::ScaleQuantizer quantizer;
    Orchestration::VoicingEngine voicer;
    Orchestration::HarmonicRandomizer randomizer;
    Sequencer::SequencerEngine sequencer;
    MidiExport::StandardMidiWriter midiWriter;

    sequencer.setPattern(Sequencer::createActionOstinatoPattern());
    sequencer.setTempo(128.0); // 128 BPM Action tempo

    std::vector<Harmonic::HarmonicFrame> detectedProgression;

    grace.setOnChordReadyCallback([&](const std::vector<Harmonic::MidiNoteEvent>& notes) {
        if (notes.empty()) return;
        std::vector<int> pitches;
        for (const auto& n : notes) pitches.push_back(n.pitch);
        auto frame = detector.detectChord(pitches);
        detectedProgression.push_back(frame);
    });

    // -------------------------------------------------------------
    // Chord 1: C min 9 with human finger delay (48, 51, 55, 58, 62)
    // -------------------------------------------------------------
    grace.handleNoteOn(48, 90, 0.000);  // C3
    grace.handleNoteOn(51, 95, 0.008);  // Eb3
    grace.handleNoteOn(55, 100, 0.015); // G3
    grace.handleNoteOn(58, 88, 0.022);  // Bb3
    grace.handleNoteOn(62, 92, 0.024);  // D4
    grace.advanceTime(0.040); // Grace window elapses

    ASSERT_EQ(detectedProgression.size(), 1);
    ASSERT_EQ(detectedProgression.back().quality, Harmonic::ChordQuality::Minor9);
    ASSERT_EQ(detectedProgression.back().rootPitchClass, 0); // C

    // Apply modal transformation: C Dorian (#6th = A natural)
    auto dorianFrame = quantizer.transformToMode(detectedProgression.back(), Harmonic::ScaleMode::Dorian);
    ASSERT_EQ(dorianFrame.rootPitchClass, 0);

    // Voicing Generation
    auto v1 = voicer.generateVoicing(dorianFrame, Orchestration::VoicingStyle::AcousticPyramid);
    ASSERT_EQ(v1.voices.size(), 25);

    // Verify all instruments are in valid tessituras
    for (const auto& voice : v1.voices) {
        auto tess = voicer.getTessitura(voice.instrument);
        ASSERT_TRUE(voice.midiPitch >= tess.minPitch);
        ASSERT_TRUE(voice.midiPitch <= tess.maxPitch);
    }

    // -------------------------------------------------------------
    // Chord 2: Ab Maj 7 (44, 48, 51, 55)
    // -------------------------------------------------------------
    grace.handleNoteOff(48, 0.050);
    grace.handleNoteOff(51, 0.050);
    grace.handleNoteOff(55, 0.050);
    grace.handleNoteOff(58, 0.050);
    grace.handleNoteOff(62, 0.050);

    grace.handleNoteOn(44, 95, 0.060); // Ab2
    grace.handleNoteOn(48, 90, 0.065); // C3
    grace.handleNoteOn(51, 98, 0.070); // Eb3
    grace.handleNoteOn(55, 88, 0.075); // G3
    grace.advanceTime(0.040);

    ASSERT_EQ(detectedProgression.size(), 2);
    ASSERT_EQ(detectedProgression.back().quality, Harmonic::ChordQuality::Major7);
    ASSERT_EQ(detectedProgression.back().rootPitchClass, 8); // Ab

    auto v2 = voicer.generateVoicing(detectedProgression.back(), Orchestration::VoicingStyle::AcousticPyramid);

    // -------------------------------------------------------------
    // Chord 3: F min 7 (41, 44, 48, 51)
    // -------------------------------------------------------------
    grace.handleNoteOff(44, 0.110);
    grace.handleNoteOff(48, 0.110);
    grace.handleNoteOff(51, 0.110);
    grace.handleNoteOff(55, 0.110);

    grace.handleNoteOn(41, 100, 0.120); // F2
    grace.handleNoteOn(44, 92, 0.125);  // Ab2
    grace.handleNoteOn(48, 96, 0.130);  // C3
    grace.handleNoteOn(51, 90, 0.132);  // Eb3
    grace.advanceTime(0.040);

    ASSERT_EQ(detectedProgression.size(), 3);
    ASSERT_EQ(detectedProgression.back().quality, Harmonic::ChordQuality::Minor7);
    ASSERT_EQ(detectedProgression.back().rootPitchClass, 5); // F

    auto v3 = voicer.generateVoicing(detectedProgression.back(), Orchestration::VoicingStyle::AcousticPyramid);

    // -------------------------------------------------------------
    // Chord 4: G7 / B Slash Chord (47 in bass, 55, 59, 62, 65 in right hand)
    // -------------------------------------------------------------
    grace.handleNoteOff(41, 0.180);
    grace.handleNoteOff(44, 0.180);
    grace.handleNoteOff(48, 0.180);
    grace.handleNoteOff(51, 0.180);

    grace.handleNoteOn(47, 105, 0.190); // B2 in bass
    grace.handleNoteOn(55, 95, 0.195);  // G3
    grace.handleNoteOn(59, 90, 0.198);  // B3
    grace.handleNoteOn(62, 92, 0.200);  // D4
    grace.handleNoteOn(65, 88, 0.202);  // F4
    grace.advanceTime(0.040);

    ASSERT_EQ(detectedProgression.size(), 4);
    ASSERT_TRUE(detectedProgression.back().isSlashChord);
    ASSERT_EQ(detectedProgression.back().rootPitchClass, 7); // G
    ASSERT_EQ(detectedProgression.back().bassMidiNote, 47);   // B

    auto v4 = voicer.generateVoicing(detectedProgression.back(), Orchestration::VoicingStyle::AcousticPyramid);

    // -------------------------------------------------------------
    // Sequencer and Standard MIDI File Export
    // -------------------------------------------------------------
    sequencer.updateVoicing(v1);

    std::vector<Sequencer::ScheduledMidiEvent> blockEvents;
    sequencer.processBlock(5513, 44100.0, 0.0, true, blockEvents);
    ASSERT_TRUE(!blockEvents.empty());

    // Export Master Multi-Track MIDI (.mid Type 1)
    std::string masterPath = "/tmp/Cubase_HollywoodOrch_Master_E2E.mid";
    bool masterOk = midiWriter.exportMidiFile(sequencer.getPattern(), v1, 128.0, 4, masterPath);
    ASSERT_TRUE(masterOk);

    // Verify binary SMF validity
    std::ifstream masterFile(masterPath, std::ios::binary);
    ASSERT_TRUE(masterFile.is_open());
    char tag[4];
    masterFile.read(tag, 4);
    ASSERT_TRUE(tag[0] == 'M' && tag[1] == 'T' && tag[2] == 'h' && tag[3] == 'd');

    // Export Stem MIDI (.mid Type 0) for French Horns
    std::string hornStemPath = "/tmp/Cubase_HollywoodOrch_Horns_Stem_E2E.mid";
    bool hornOk = midiWriter.exportMidiFile(sequencer.getPattern(), v1, 128.0, 4, hornStemPath, Harmonic::InstrumentId::FrenchHorns);
    ASSERT_TRUE(hornOk);

    std::ifstream hornFile(hornStemPath, std::ios::binary);
    ASSERT_TRUE(hornFile.is_open());
    hornFile.read(tag, 4);
    ASSERT_TRUE(tag[0] == 'M' && tag[1] == 'T' && tag[2] == 'h' && tag[3] == 'd');
}
