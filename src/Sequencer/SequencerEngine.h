#pragma once

#include "Common/HarmonicTypes.h"
#include "PatternModel.h"
#include <vector>
#include <map>

namespace Sequencer {

struct ScheduledMidiEvent {
    int sampleOffset = 0; // offset within current audio block (0..numSamples-1)
    int channel = 1;      // 1..16
    int pitch = 60;
    int velocity = 100;
    bool isNoteOn = true;
    Harmonic::ArticulationType articulation = Harmonic::ArticulationType::Sustain;
};

class SequencerEngine {
public:
    SequencerEngine();

    void setPattern(const OrchestralPattern& pattern) { activePattern = pattern; }
    const OrchestralPattern& getPattern() const { return activePattern; }

    void setTempo(double bpm) { tempoBpm = bpm; }
    double getTempo() const { return tempoBpm; }

    void updateVoicing(const Harmonic::OrchestralVoicing& voicing);

    // Audio block execution
    void processBlock(int numSamples,
                      double sampleRate,
                      double hostPpqPosition,
                      bool isHostPlaying,
                      std::vector<ScheduledMidiEvent>& outEvents);

    void stopAllNotes(std::vector<ScheduledMidiEvent>& outEvents);

private:
    OrchestralPattern activePattern;
    Harmonic::OrchestralVoicing currentVoicing;
    double tempoBpm = 120.0;
    double internalPpq = 0.0;
    int currentStep = -1;

    struct ActiveNoteState {
        int pitch = -1;
        int channel = 1;
        int remainingSamples = 0;
        bool active = false;
    };

    std::map<Harmonic::InstrumentId, ActiveNoteState> activeNotes;
    std::map<Harmonic::InstrumentId, int> arpIndex;

    int computeArpPitch(Harmonic::InstrumentId inst, Harmonic::StepActionType action, int basePitch);
};

} // namespace Sequencer
