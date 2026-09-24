#pragma once

#include <vector>
#include <functional>
#include <algorithm>
#include <array>

namespace Harmonic {

struct MidiNoteEvent {
    int pitch = 0;       // 0-127
    int velocity = 0;    // 1-127 for NoteOn
    double timestamp = 0.0;
};

class GraceWindowBuffer {
public:
    using ChordReadyCallback = std::function<void(const std::vector<MidiNoteEvent>&)>;

    GraceWindowBuffer();

    void setGracePeriodMs(double ms) { gracePeriodMs = ms; }
    double getGracePeriodMs() const { return gracePeriodMs; }

    void handleNoteOn(int pitch, int velocity, double timestamp);
    void handleNoteOff(int pitch, double timestamp);
    void handleController(int controllerNumber, int value, double timestamp);

    void advanceTime(double deltaSeconds);
    void reset();

    void setOnChordReadyCallback(ChordReadyCallback cb) { onChordReady = cb; }

    std::vector<MidiNoteEvent> getActiveSoundingNotes() const;

private:
    double gracePeriodMs = 25.0;
    double currentTime = 0.0;
    double lastNoteArrival = -1.0;
    bool windowActive = false;
    bool sustainPedalDown = false;

    // Keys physically held down by fingers
    std::array<bool, 128> physicallyHeld{};
    std::array<int, 128> physicalVelocity{};

    // Notes sustained by CC64 pedal
    std::array<bool, 128> pedalSustained{};

    // Last dispatched chord
    std::vector<MidiNoteEvent> lastDispatchedChord;

    ChordReadyCallback onChordReady;

    void evaluateAndDispatch();
};

} // namespace Harmonic
