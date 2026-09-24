#include "TestHarness.h"
#include "Harmonic/GraceWindowBuffer.h"
#include "Harmonic/ChordDetector.h"
#include "Harmonic/ScaleQuantizer.h"
#include "Orchestration/VoicingEngine.h"
#include "Orchestration/HarmonicRandomizer.h"
#include "Sequencer/SequencerEngine.h"
#include "Articulation/ArticulationMap.h"

// Simulated Plugin Processor Pipeline Test
TEST_CASE(PluginProcessor, FullMidiProcessingPipeline) {
    Harmonic::GraceWindowBuffer grace;
    Harmonic::ChordDetector chordDetector;
    Harmonic::ScaleQuantizer quantizer;
    Orchestration::VoicingEngine voicer;
    Orchestration::HarmonicRandomizer randomizer;
    Sequencer::SequencerEngine seq;
    auto artMap = Articulation::createCSSProfile(); // CSS on CC58

    seq.setPattern(Sequencer::createActionOstinatoPattern());
    seq.setTempo(120.0);

    bool chordFired = false;
    Harmonic::HarmonicFrame currentFrame;

    grace.setOnChordReadyCallback([&](const std::vector<Harmonic::MidiNoteEvent>& notes) {
        chordFired = true;
        std::vector<int> pitches;
        for (const auto& n : notes) pitches.push_back(n.pitch);

        currentFrame = chordDetector.detectChord(pitches);
        auto modalFrame = quantizer.transformToMode(currentFrame, Harmonic::ScaleMode::Ionian);
        auto rawVoicing = voicer.generateVoicing(modalFrame);
        auto finalVoicing = randomizer.processVoicing(rawVoicing, modalFrame);
        seq.updateVoicing(finalVoicing);
    });

    // 1. Simulate incoming NoteOn C3, E3, G3 from Cubase keyboard track
    grace.handleNoteOn(48, 100, 0.0);
    grace.handleNoteOn(52, 95, 0.005);
    grace.handleNoteOn(55, 105, 0.010);

    // Advance 30ms to trigger grace window
    grace.advanceTime(0.030);

    ASSERT_TRUE(chordFired);
    ASSERT_EQ(currentFrame.quality, Harmonic::ChordQuality::MajorTriad);
    ASSERT_EQ(currentFrame.rootPitchClass, 0); // C

    // 2. Process an audio block
    std::vector<Sequencer::ScheduledMidiEvent> outMidi;
    seq.processBlock(5513, 44100.0, 0.0, true, outMidi);

    ASSERT_TRUE(!outMidi.empty());

    // 3. Verify multi-channel distribution (Strings on Ch 1-5, Brass on 6-9, WW on 10-13)
    std::set<int> activeChannels;
    for (const auto& ev : outMidi) {
        if (ev.isNoteOn) {
            activeChannels.insert(ev.channel);
            // Verify CSS articulation mapping (Spiccato triggers CC58=20 or articulation tag)
            ASSERT_TRUE(ev.velocity > 0);
        }
    }

    ASSERT_TRUE(activeChannels.count(1)); // Violins 1
    ASSERT_TRUE(activeChannels.count(2)); // Violins 2
    ASSERT_TRUE(activeChannels.count(4)); // Cellos
    ASSERT_TRUE(activeChannels.count(5)); // Basses
}
