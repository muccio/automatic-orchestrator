#include "TestHarness.h"
#include "Orchestration/HarmonicRandomizer.h"
#include "Orchestration/VoicingEngine.h"

TEST_CASE(HarmonicRandomizer, HumanizeVelocityAndTiming) {
    Orchestration::HarmonicRandomizer randomizer;
    randomizer.setSeed(42); // Deterministic test seed
    randomizer.setVelocityHumanizeAmount(10); // +/- 10
    randomizer.setTimingHumanizeMs(8.0);     // +/- 8ms

    Harmonic::VoiceAssignment va;
    va.instrument = Harmonic::InstrumentId::Violins1;
    va.midiPitch = 72;
    va.velocity = 80;
    va.midiChannel = 1;

    int vMod = randomizer.applyVelocityHumanize(va.velocity);
    ASSERT_TRUE(vMod >= 70 && vMod <= 90);

    double dt = randomizer.applyTimingHumanize();
    ASSERT_TRUE(std::abs(dt) <= 0.008 + 1e-4);
}

TEST_CASE(HarmonicRandomizer, TensionInjection) {
    Orchestration::HarmonicRandomizer randomizer;
    randomizer.setSeed(12345);
    randomizer.setTensionInjectionChance(1.0); // 100% inject tension

    Harmonic::HarmonicFrame cMaj;
    cMaj.rootPitchClass = 0; // C
    cMaj.bassMidiNote = 48;
    cMaj.quality = Harmonic::ChordQuality::MajorTriad;
    cMaj.pitches = {48, 52, 55};
    cMaj.chordTones = {0, 4, 7};
    cMaj.activeMode = Harmonic::ScaleMode::Ionian;

    Orchestration::VoicingEngine ve;
    auto voicing = ve.generateVoicing(cMaj);

    auto randomizedVoicing = randomizer.processVoicing(voicing, cMaj);

    // At least one upper voice should now carry an allowed scale extension (e.g. D = 9th, A = 6th, or B = Maj7)
    bool hasExtension = false;
    for (const auto& v : randomizedVoicing.voices) {
        int pc = v.midiPitch % 12;
        if (pc == 2 || pc == 9 || pc == 11) { // 9th, 6th, or Maj7
            hasExtension = true;
            break;
        }
    }

    ASSERT_TRUE(hasExtension);
}
