#include "TestHarness.h"
#include "Harmonic/ScaleQuantizer.h"

TEST_CASE(ScaleQuantizer, ModalTransformations) {
    Harmonic::ScaleQuantizer quantizer;

    // Base chord: C Major {C4 (60), E4 (64), G4 (67)}
    Harmonic::HarmonicFrame cMaj;
    cMaj.rootPitchClass = 0; // C
    cMaj.bassMidiNote = 60;
    cMaj.quality = Harmonic::ChordQuality::MajorTriad;
    cMaj.pitches = {60, 64, 67};
    cMaj.chordTones = {0, 4, 7};

    // Transform to C Dorian (Minor 3rd: E -> Eb = 63)
    auto dorianFrame = quantizer.transformToMode(cMaj, Harmonic::ScaleMode::Dorian);
    ASSERT_EQ(dorianFrame.rootPitchClass, 0);
    ASSERT_EQ(dorianFrame.pitches.size(), 3);
    ASSERT_EQ(dorianFrame.pitches[0], 60); // C
    ASSERT_EQ(dorianFrame.pitches[1], 63); // Eb (quantized from E 64)
    ASSERT_EQ(dorianFrame.pitches[2], 67); // G
    ASSERT_EQ(dorianFrame.quality, Harmonic::ChordQuality::MinorTriad);

    // Transform to C Whole-Tone ({0, 2, 4, 6, 8, 10}: G 67 (interval 7) quantizes to G# 68 or F# 66)
    auto wtFrame = quantizer.transformToMode(cMaj, Harmonic::ScaleMode::WholeTone);
    ASSERT_EQ(wtFrame.rootPitchClass, 0);
    // 60 (0) -> 60; 64 (4) -> 64; 67 (7) -> 68 (8) or 66 (6)
    ASSERT_TRUE(wtFrame.pitches[2] == 66 || wtFrame.pitches[2] == 68);
}

TEST_CASE(ScaleQuantizer, NoteQuantization) {
    Harmonic::ScaleQuantizer quantizer;

    // In C Major (Ionian), F# (66) should snap to F (65) or G (67)
    int snapped = quantizer.quantizePitch(66, 0, Harmonic::ScaleMode::Ionian);
    ASSERT_TRUE(snapped == 65 || snapped == 67);

    // In C Lydian, F# (66) IS in the scale (augmented 4th), so it stays 66!
    int lydianNote = quantizer.quantizePitch(66, 0, Harmonic::ScaleMode::Lydian);
    ASSERT_EQ(lydianNote, 66);

    // In C Phrygian, Db (61) IS in the scale (minor 2nd), so it stays 61!
    int phrygianNote = quantizer.quantizePitch(61, 0, Harmonic::ScaleMode::Phrygian);
    ASSERT_EQ(phrygianNote, 61);
}
