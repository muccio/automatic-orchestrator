#include "TestHarness.h"
#include "Harmonic/GraceWindowBuffer.h"

TEST_CASE(GraceWindowBuffer, PolyphonicDebounce) {
    Harmonic::GraceWindowBuffer buffer;
    buffer.setGracePeriodMs(25.0); // 25ms window

    bool triggered = false;
    std::vector<int> capturedPitches;

    buffer.setOnChordReadyCallback([&](const std::vector<Harmonic::MidiNoteEvent>& notes) {
        triggered = true;
        capturedPitches.clear();
        for (const auto& n : notes) {
            capturedPitches.push_back(n.pitch);
        }
    });

    // t = 0ms: C4 (60) arrives
    buffer.handleNoteOn(60, 100, 0.0);
    ASSERT_FALSE(triggered); // Should not trigger immediately!

    // t = 10ms: E4 (64) arrives
    buffer.advanceTime(0.010);
    buffer.handleNoteOn(64, 95, 0.010);
    ASSERT_FALSE(triggered);

    // t = 20ms: G4 (67) arrives
    buffer.advanceTime(0.010);
    buffer.handleNoteOn(67, 105, 0.020);
    ASSERT_FALSE(triggered);

    // t = 46ms: Grace window (25ms after last note at 20ms = 45ms) has elapsed
    buffer.advanceTime(0.030); // reaches 50ms
    ASSERT_TRUE(triggered);
    ASSERT_EQ(capturedPitches.size(), 3);
    ASSERT_EQ(capturedPitches[0], 60);
    ASSERT_EQ(capturedPitches[1], 64);
    ASSERT_EQ(capturedPitches[2], 67);
}

TEST_CASE(GraceWindowBuffer, SustainPedalBehavior) {
    Harmonic::GraceWindowBuffer buffer;
    buffer.setGracePeriodMs(15.0);

    int triggerCount = 0;
    std::vector<int> lastPitches;
    buffer.setOnChordReadyCallback([&](const std::vector<Harmonic::MidiNoteEvent>& notes) {
        triggerCount++;
        lastPitches.clear();
        for (const auto& n : notes) lastPitches.push_back(n.pitch);
    });

    // Press sustain pedal (CC 64 = 127)
    buffer.handleController(64, 127, 0.0);

    // Play chord C-E-G
    buffer.handleNoteOn(60, 100, 0.0);
    buffer.handleNoteOn(64, 100, 0.005);
    buffer.handleNoteOn(67, 100, 0.010);
    buffer.advanceTime(0.030); // 30ms

    ASSERT_EQ(triggerCount, 1);
    ASSERT_EQ(lastPitches.size(), 3);

    // Release fingers (NoteOff) while sustain pedal is held
    buffer.handleNoteOff(60, 0.035);
    buffer.handleNoteOff(64, 0.035);
    buffer.handleNoteOff(67, 0.035);
    buffer.advanceTime(0.030); // 65ms

    // Sounding notes must NOT be wiped because pedal is held!
    auto sounding = buffer.getActiveSoundingNotes();
    ASSERT_EQ(sounding.size(), 3);

    // Release sustain pedal (CC 64 = 0)
    buffer.handleController(64, 0, 0.070);
    buffer.advanceTime(0.020); // 90ms

    // Now all held notes should be cleared
    sounding = buffer.getActiveSoundingNotes();
    ASSERT_EQ(sounding.size(), 0);
}
