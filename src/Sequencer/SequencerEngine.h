#pragma once

#include "Common/HarmonicTypes.h"
#include "PatternModel.h"
#include <vector>
#include <map>
#include <mutex>

namespace Sequencer {

struct ScheduledMidiEvent {
    int sampleOffset = 0; // offset within current audio block (0..numSamples-1)
    int channel = 1;      // 1..16
    int pitch = 60;
    int velocity = 100;
    bool isNoteOn = true;
    bool isController = false; // if true, pitch is CC number and velocity is CC value
    Harmonic::ArticulationType articulation = Harmonic::ArticulationType::Sustain;
    Harmonic::InstrumentId instrument = Harmonic::InstrumentId::Violins1;
};

class SequencerEngine {
public:
    SequencerEngine();

    void setPattern(const OrchestralPattern& pattern);
    OrchestralPattern getPattern() const;

    void setTempo(double bpm) { tempoBpm = bpm; }
    double getTempo() const { return tempoBpm; }

    void updateVoicing(const Harmonic::OrchestralVoicing& voicing);

    // Multi-Bar Pattern Management
    void setPatternBarLength(int newBars);
    int getPatternBarLength() const;
    int getTotalSteps() const;
    void copyBar1ToAllBars();

    // Dynamic Track & Step Editing
    void setTrackStep(Harmonic::InstrumentId inst, int stepIndex, bool active, int stepOffset, int velocity, Harmonic::ArticulationType art);
    void setTrackStepWithExtras(Harmonic::InstrumentId inst, int stepIndex, bool active, int stepOffset, const std::vector<int>& extraOffsets, int velocity, Harmonic::ArticulationType art);
    void addTrackStepOffset(Harmonic::InstrumentId inst, int stepIndex, int offset, int velocity, Harmonic::ArticulationType art);
    void removeTrackStepOffset(Harmonic::InstrumentId inst, int stepIndex, int offset);
    void setTrackStepLength(Harmonic::InstrumentId inst, int stepIndex, int lengthSteps);
    void setTrackArticulation(Harmonic::InstrumentId inst, Harmonic::ArticulationType art);
    void setTrackMode(Harmonic::InstrumentId inst, const std::string& mode);
    void setTrackOctave(Harmonic::InstrumentId inst, int octave);
    void setTrackVolume(Harmonic::InstrumentId inst, float vol);
    void setTrackPan(Harmonic::InstrumentId inst, float pan);
    void setTrackMute(Harmonic::InstrumentId inst, bool mute);
    void setTrackSolo(Harmonic::InstrumentId inst, bool solo);
    void setTrackCc1(Harmonic::InstrumentId inst, int stepIndex, int cc1Val);

    void addTrack(Harmonic::InstrumentId inst, const std::string& name, Harmonic::OrchestralSection sec, int channel, Harmonic::ArticulationType art);
    void removeTrack(Harmonic::InstrumentId inst);

    int getCurrentStep() const { return currentStep; }

    // Audio block execution
    void processBlock(int numSamples,
                      double sampleRate,
                      double hostPpqPosition,
                      bool isHostPlaying,
                      std::vector<ScheduledMidiEvent>& outEvents);

    void stopAllNotes(std::vector<ScheduledMidiEvent>& outEvents);

private:
    mutable std::mutex patternMutex;
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

    std::map<Harmonic::InstrumentId, std::vector<ActiveNoteState>> activeNotes;
    std::map<Harmonic::InstrumentId, int> arpIndex;

    int computeRelativeStepPitch(Harmonic::InstrumentId inst, const TrackPattern& track, int stepOffset, int basePitch, int stepOctave = 0);
    int computeRelativeStepPitch(Harmonic::InstrumentId inst, const TrackPattern& track, const StepDefinition& stepDef, int basePitch);
    int computeArpPitch(Harmonic::InstrumentId inst, Harmonic::StepActionType action, int basePitch);
    TrackPattern& ensureTrackExistsLocked(Harmonic::InstrumentId inst);
};

} // namespace Sequencer
