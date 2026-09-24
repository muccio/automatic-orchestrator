#include "GraceWindowBuffer.h"

namespace Harmonic {

GraceWindowBuffer::GraceWindowBuffer() {
    reset();
}

void GraceWindowBuffer::reset() {
    physicallyHeld.fill(false);
    physicalVelocity.fill(0);
    pedalSustained.fill(false);
    lastDispatchedChord.clear();
    lastNoteArrival = -1.0;
    windowActive = false;
    sustainPedalDown = false;
    currentTime = 0.0;
}

void GraceWindowBuffer::handleNoteOn(int pitch, int velocity, double timestamp) {
    if (pitch < 0 || pitch > 127) return;

    currentTime = std::max(currentTime, timestamp);
    physicallyHeld[pitch] = true;
    physicalVelocity[pitch] = velocity;
    pedalSustained[pitch] = false; // Fresh key press

    lastNoteArrival = currentTime;
    windowActive = true;
}

void GraceWindowBuffer::handleNoteOff(int pitch, double timestamp) {
    if (pitch < 0 || pitch > 127) return;

    currentTime = std::max(currentTime, timestamp);
    physicallyHeld[pitch] = false;

    if (sustainPedalDown) {
        pedalSustained[pitch] = true;
    } else {
        pedalSustained[pitch] = false;
        // Key released without pedal: re-evaluate if no window active
        if (!windowActive) {
            evaluateAndDispatch();
        }
    }
}

void GraceWindowBuffer::handleController(int controllerNumber, int value, double timestamp) {
    currentTime = std::max(currentTime, timestamp);

    if (controllerNumber == 64) { // Sustain Pedal
        bool wasDown = sustainPedalDown;
        sustainPedalDown = (value >= 64);

        if (wasDown && !sustainPedalDown) {
            // Pedal released: clear all pedal-sustained notes that are not physically held
            for (int i = 0; i < 128; ++i) {
                if (pedalSustained[i] && !physicallyHeld[i]) {
                    pedalSustained[i] = false;
                }
            }
            if (!windowActive) {
                evaluateAndDispatch();
            }
        }
    }
}

void GraceWindowBuffer::advanceTime(double deltaSeconds) {
    currentTime += deltaSeconds;

    if (windowActive) {
        double elapsedSinceLastNoteMs = (currentTime - lastNoteArrival) * 1000.0;
        if (elapsedSinceLastNoteMs >= gracePeriodMs) {
            windowActive = false;
            evaluateAndDispatch();
        }
    }
}

std::vector<MidiNoteEvent> GraceWindowBuffer::getActiveSoundingNotes() const {
    std::vector<MidiNoteEvent> active;
    active.reserve(16);
    for (int p = 0; p < 128; ++p) {
        if (physicallyHeld[p] || pedalSustained[p]) {
            active.push_back({p, physicalVelocity[p] > 0 ? physicalVelocity[p] : 80, currentTime});
        }
    }
    return active;
}

void GraceWindowBuffer::evaluateAndDispatch() {
    auto currentNotes = getActiveSoundingNotes();

    // Check if distinct from lastDispatchedChord
    bool changed = (currentNotes.size() != lastDispatchedChord.size());
    if (!changed) {
        for (size_t i = 0; i < currentNotes.size(); ++i) {
            if (currentNotes[i].pitch != lastDispatchedChord[i].pitch) {
                changed = true;
                break;
            }
        }
    }

    if (changed || !currentNotes.empty()) {
        lastDispatchedChord = currentNotes;
        if (onChordReady) {
            onChordReady(currentNotes);
        }
    }
}

} // namespace Harmonic
