#include "TestHarness.h"
#include "Harmonic/ChordDetector.h"

TEST_CASE(ChordDetector, SingleNoteAndPowerChord) {
    Harmonic::ChordDetector detector;

    // Single note C4
    auto f1 = detector.detectChord({60});
    ASSERT_TRUE(f1.isSingleNote);
    ASSERT_EQ(f1.rootPitchClass, 0); // C
    ASSERT_EQ(f1.bassMidiNote, 60);

    // Power chord C3 + G3
    auto f2 = detector.detectChord({48, 55});
    ASSERT_EQ(f2.quality, Harmonic::ChordQuality::PowerChord);
    ASSERT_EQ(f2.rootPitchClass, 0); // C
}

TEST_CASE(ChordDetector, TriadsAndInversions) {
    Harmonic::ChordDetector detector;

    // C Major root position (C3, E3, G3)
    auto cMaj = detector.detectChord({48, 52, 55});
    ASSERT_EQ(cMaj.quality, Harmonic::ChordQuality::MajorTriad);
    ASSERT_EQ(cMaj.rootPitchClass, 0);
    ASSERT_EQ(cMaj.chordName, "C");
    ASSERT_FALSE(cMaj.isSlashChord);

    // C Major 1st inversion (E3, G3, C4) -> C/E
    auto cOverE = detector.detectChord({52, 55, 60});
    ASSERT_EQ(cOverE.quality, Harmonic::ChordQuality::MajorTriad);
    ASSERT_EQ(cOverE.rootPitchClass, 0);
    ASSERT_TRUE(cOverE.isSlashChord);
    ASSERT_EQ(cOverE.chordName, "C/E");

    // D Minor root position (D3, F3, A3)
    auto dMin = detector.detectChord({50, 53, 57});
    ASSERT_EQ(dMin.quality, Harmonic::ChordQuality::MinorTriad);
    ASSERT_EQ(dMin.rootPitchClass, 2); // D
    ASSERT_EQ(dMin.chordName, "Dm");

    // D Minor 1st inversion (F3, A3, D4) -> Dm/F
    auto dOverF = detector.detectChord({53, 57, 62});
    ASSERT_EQ(dOverF.quality, Harmonic::ChordQuality::MinorTriad);
    ASSERT_EQ(dOverF.rootPitchClass, 2);
    ASSERT_TRUE(dOverF.isSlashChord);
    ASSERT_EQ(dOverF.chordName, "Dm/F");

    // B Diminished (B2, D3, F3)
    auto bDim = detector.detectChord({47, 50, 53});
    ASSERT_EQ(bDim.quality, Harmonic::ChordQuality::DiminishedTriad);
    ASSERT_EQ(bDim.rootPitchClass, 11); // B

    // G Augmented (G3, B3, D#4)
    auto gAug = detector.detectChord({55, 59, 63});
    ASSERT_EQ(gAug.quality, Harmonic::ChordQuality::AugmentedTriad);
    ASSERT_EQ(gAug.rootPitchClass, 7); // G

    // D Sus4 (D3, G3, A3)
    auto dSus4 = detector.detectChord({50, 55, 57});
    ASSERT_EQ(dSus4.quality, Harmonic::ChordQuality::Sus4);
    ASSERT_EQ(dSus4.rootPitchClass, 2);

    // D Sus2 (D3, E3, A3)
    auto dSus2 = detector.detectChord({50, 52, 57});
    ASSERT_EQ(dSus2.quality, Harmonic::ChordQuality::Sus2);
    ASSERT_EQ(dSus2.rootPitchClass, 2);
}

TEST_CASE(ChordDetector, SeventhChords) {
    Harmonic::ChordDetector detector;

    // G7 Dominant (G3, B3, D4, F4)
    auto g7 = detector.detectChord({55, 59, 62, 65});
    ASSERT_EQ(g7.quality, Harmonic::ChordQuality::Dominant7);
    ASSERT_EQ(g7.rootPitchClass, 7); // G
    ASSERT_EQ(g7.chordName, "G7");

    // C Maj7 (C3, E3, G3, B3)
    auto cMaj7 = detector.detectChord({48, 52, 55, 59});
    ASSERT_EQ(cMaj7.quality, Harmonic::ChordQuality::Major7);
    ASSERT_EQ(cMaj7.rootPitchClass, 0); // C
    ASSERT_EQ(cMaj7.chordName, "CMaj7");

    // C Maj7 open voicing with deep bass (C2, C3, G3, E4, B4)
    auto cMaj7Open = detector.detectChord({36, 48, 55, 64, 71});
    ASSERT_EQ(cMaj7Open.quality, Harmonic::ChordQuality::Major7);
    ASSERT_EQ(cMaj7Open.rootPitchClass, 0);
    ASSERT_EQ(cMaj7Open.chordName, "CMaj7");

    // C Maj7 no-5th voicing (C3, E3, B3)
    auto cMaj7No5th = detector.detectChord({48, 52, 59});
    ASSERT_EQ(cMaj7No5th.quality, Harmonic::ChordQuality::Major7);
    ASSERT_EQ(cMaj7No5th.rootPitchClass, 0);
    ASSERT_EQ(cMaj7No5th.chordName, "CMaj7");

    // A Min7 (A2, C3, E3, G3)
    auto aMin7 = detector.detectChord({45, 48, 52, 55});
    ASSERT_EQ(aMin7.quality, Harmonic::ChordQuality::Minor7);
    ASSERT_EQ(aMin7.rootPitchClass, 9); // A
    ASSERT_EQ(aMin7.chordName, "Am7");

    // B Half-Diminished m7b5 (B2, D3, F3, A3)
    auto bHalfDim = detector.detectChord({47, 50, 53, 57});
    ASSERT_EQ(bHalfDim.quality, Harmonic::ChordQuality::HalfDiminished7);
    ASSERT_EQ(bHalfDim.rootPitchClass, 11); // B
    ASSERT_EQ(bHalfDim.chordName, "Bm7b5");

    // C Full Diminished 7 (C3, Eb3, Gb3, A3/Bbb3)
    auto cDim7 = detector.detectChord({48, 51, 54, 57});
    ASSERT_EQ(cDim7.quality, Harmonic::ChordQuality::Diminished7);
    ASSERT_EQ(cDim7.rootPitchClass, 0);
}

TEST_CASE(ChordDetector, ExtendedChordsAndSlashChords) {
    Harmonic::ChordDetector detector;

    // C Add9 (C3, E3, G3, D4)
    auto cAdd9 = detector.detectChord({48, 52, 55, 62});
    ASSERT_EQ(cAdd9.quality, Harmonic::ChordQuality::Add9);
    ASSERT_EQ(cAdd9.rootPitchClass, 0);

    // C Maj9 (C3, E3, G3, B3, D4)
    auto cMaj9 = detector.detectChord({48, 52, 55, 59, 62});
    ASSERT_EQ(cMaj9.quality, Harmonic::ChordQuality::Major9);
    ASSERT_EQ(cMaj9.rootPitchClass, 0);

    // A Min9 (A2, C3, E3, G3, B3)
    auto aMin9 = detector.detectChord({45, 48, 52, 55, 59});
    ASSERT_EQ(aMin9.quality, Harmonic::ChordQuality::Minor9);
    ASSERT_EQ(aMin9.rootPitchClass, 9);

    // Bb/C slash chord (C2 in bass, Bb3, D4, F4 in right hand)
    auto bbOverC = detector.detectChord({36, 58, 62, 65});
    ASSERT_EQ(bbOverC.rootPitchClass, 10); // Bb
    ASSERT_EQ(bbOverC.bassMidiNote, 36);   // C
    ASSERT_TRUE(bbOverC.isSlashChord);
    ASSERT_EQ(bbOverC.chordName, "Bb/C");
}
