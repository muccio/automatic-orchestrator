#include "TestHarness.h"
#include "Common/HarmonicTypes.h"

TEST_CASE(HarmonicTypes, NoteConversions) {
    ASSERT_EQ(Harmonic::pitchClassToName(0), "C");
    ASSERT_EQ(Harmonic::pitchClassToName(4), "E");
    ASSERT_EQ(Harmonic::pitchClassToName(7), "G");
    ASSERT_EQ(Harmonic::pitchClassToName(9), "A");
    ASSERT_EQ(Harmonic::pitchClassToName(11), "B");

    ASSERT_EQ(Harmonic::noteNumberToName(60), "C4"); // Middle C
    ASSERT_EQ(Harmonic::noteNumberToName(36), "C2");
    ASSERT_EQ(Harmonic::noteNumberToName(64), "E4");
    ASSERT_EQ(Harmonic::noteNumberToName(67), "G4");
}

TEST_CASE(HarmonicTypes, ChordQualityStrings) {
    ASSERT_EQ(Harmonic::chordQualityToString(Harmonic::ChordQuality::MajorTriad), "Major");
    ASSERT_EQ(Harmonic::chordQualityToString(Harmonic::ChordQuality::MinorTriad), "Minor");
    ASSERT_EQ(Harmonic::chordQualityToString(Harmonic::ChordQuality::Dominant7), "7");
    ASSERT_EQ(Harmonic::chordQualityToString(Harmonic::ChordQuality::Major7), "Maj7");
    ASSERT_EQ(Harmonic::chordQualityToString(Harmonic::ChordQuality::Minor7), "Min7");
    ASSERT_EQ(Harmonic::chordQualityToString(Harmonic::ChordQuality::HalfDiminished7), "m7b5");
    ASSERT_EQ(Harmonic::chordQualityToString(Harmonic::ChordQuality::Diminished7), "dim7");
}

TEST_CASE(HarmonicTypes, ScaleModeIntervals) {
    auto ionian = Harmonic::getScaleModeIntervals(Harmonic::ScaleMode::Ionian);
    ASSERT_EQ(ionian.size(), 7);
    ASSERT_EQ(ionian[1], 2); // Whole step
    ASSERT_EQ(ionian[2], 4); // Major 3rd
    ASSERT_EQ(ionian[3], 5); // Perfect 4th
    ASSERT_EQ(ionian[4], 7); // Perfect 5th

    auto dorian = Harmonic::getScaleModeIntervals(Harmonic::ScaleMode::Dorian);
    ASSERT_EQ(dorian[2], 3); // Minor 3rd
    ASSERT_EQ(dorian[5], 9); // Major 6th
}
