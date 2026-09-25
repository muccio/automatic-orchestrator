#include "TestHarness.h"
#include "Orchestration/VoicingEngine.h"

TEST_CASE(VoicingEngine, OrchestralTessituras) {
    Orchestration::VoicingEngine engine;

    // Check tessituras boundaries
    auto dbRange = engine.getTessitura(Harmonic::InstrumentId::DoubleBasses);
    ASSERT_TRUE(dbRange.minPitch <= 24); // C1
    ASSERT_TRUE(dbRange.maxPitch >= 48); // C3

    auto vln1Range = engine.getTessitura(Harmonic::InstrumentId::Violins1);
    ASSERT_TRUE(vln1Range.minPitch <= 55); // G3
    ASSERT_TRUE(vln1Range.maxPitch >= 96); // C7
}

TEST_CASE(VoicingEngine, AcousticPyramidDistribution) {
    Orchestration::VoicingEngine engine;

    // Input: C Major triad (Root C, Bass C)
    Harmonic::HarmonicFrame cMaj;
    cMaj.rootPitchClass = 0; // C
    cMaj.bassMidiNote = 48;  // C3
    cMaj.quality = Harmonic::ChordQuality::MajorTriad;
    cMaj.pitches = {48, 52, 55};
    cMaj.chordTones = {0, 4, 7};

    auto voicing = engine.generateVoicing(cMaj, Orchestration::VoicingStyle::AcousticPyramid);

    // Verify all instruments are assigned
    ASSERT_EQ(voicing.voices.size(), 25);

    int bassPitch = -1;
    int celloPitch = -1;
    int vln1Pitch = -1;

    for (const auto& v : voicing.voices) {
        if (v.instrument == Harmonic::InstrumentId::DoubleBasses) bassPitch = v.midiPitch;
        if (v.instrument == Harmonic::InstrumentId::Cellos) celloPitch = v.midiPitch;
        if (v.instrument == Harmonic::InstrumentId::Violins1) vln1Pitch = v.midiPitch;

        // Verify all voices are within their real instrument tessitura
        auto range = engine.getTessitura(v.instrument);
        ASSERT_TRUE(v.midiPitch >= range.minPitch);
        ASSERT_TRUE(v.midiPitch <= range.maxPitch);

        // Verify all assigned pitches belong to C Major (C, E, G -> pc 0, 4, 7)
        int pc = v.midiPitch % 12;
        ASSERT_TRUE(pc == 0 || pc == 4 || pc == 7);
    }

    // Double basses must be lower than Cellos
    ASSERT_TRUE(bassPitch < celloPitch);
    // Violins 1 must be higher than Cellos
    ASSERT_TRUE(vln1Pitch > celloPitch);
}

TEST_CASE(VoicingEngine, VoiceLeadingContinuity) {
    Orchestration::VoicingEngine engine;

    // 1. First chord: C Major {C, E, G}
    Harmonic::HarmonicFrame cMaj;
    cMaj.rootPitchClass = 0;
    cMaj.bassMidiNote = 48;
    cMaj.quality = Harmonic::ChordQuality::MajorTriad;
    cMaj.pitches = {48, 52, 55};
    cMaj.chordTones = {0, 4, 7};
    auto v1 = engine.generateVoicing(cMaj, Orchestration::VoicingStyle::AcousticPyramid);

    // 2. Second chord: A Minor {A, C, E}
    Harmonic::HarmonicFrame aMin;
    aMin.rootPitchClass = 9;
    aMin.bassMidiNote = 45;
    aMin.quality = Harmonic::ChordQuality::MinorTriad;
    aMin.pitches = {45, 48, 52};
    aMin.chordTones = {0, 3, 7};
    auto v2 = engine.generateVoicing(aMin, Orchestration::VoicingStyle::AcousticPyramid);

    // Violins 1 pitch movement between Cmaj and Amin should be minimal (smooth voice leading, e.g. <= 4 semitones)
    int vln1_1 = -1, vln1_2 = -1;
    for (const auto& v : v1.voices) if (v.instrument == Harmonic::InstrumentId::Violins1) vln1_1 = v.midiPitch;
    for (const auto& v : v2.voices) if (v.instrument == Harmonic::InstrumentId::Violins1) vln1_2 = v.midiPitch;

    ASSERT_TRUE(std::abs(vln1_1 - vln1_2) <= 5); // Smooth voice leading!
}
