#include "SequencerEngine.h"
#include <cmath>
#include <algorithm>

namespace Sequencer {

SequencerEngine::SequencerEngine() {
    activePattern = createActionOstinatoPattern();
}

void SequencerEngine::updateVoicing(const Harmonic::OrchestralVoicing& voicing) {
    currentVoicing = voicing;
}

int SequencerEngine::computeArpPitch(Harmonic::InstrumentId inst, Harmonic::StepActionType action, int basePitch) {
    const auto& pitches = currentVoicing.sourceHarmonic.pitches;
    if (pitches.empty()) return basePitch;

    int idx = arpIndex[inst];
    if (action == Harmonic::StepActionType::ArpUp) {
        int pitch = pitches[idx % pitches.size()];
        arpIndex[inst] = (idx + 1) % pitches.size();
        return pitch;
    } else if (action == Harmonic::StepActionType::ArpDown) {
        int pitch = pitches[(pitches.size() - 1 - (idx % pitches.size())) % pitches.size()];
        arpIndex[inst] = (idx + 1) % pitches.size();
        return pitch;
    }
    return basePitch;
}

void SequencerEngine::stopAllNotes(std::vector<ScheduledMidiEvent>& outEvents) {
    for (auto& [inst, state] : activeNotes) {
        if (state.active) {
            ScheduledMidiEvent off;
            off.sampleOffset = 0;
            off.channel = state.channel;
            off.pitch = state.pitch;
            off.velocity = 0;
            off.isNoteOn = false;
            outEvents.push_back(off);
            state.active = false;
        }
    }
}

void SequencerEngine::processBlock(int numSamples,
                                  double sampleRate,
                                  double hostPpqPosition,
                                  bool isHostPlaying,
                                  std::vector<ScheduledMidiEvent>& outEvents) {
    if (currentVoicing.voices.empty()) {
        stopAllNotes(outEvents);
        return;
    }

    double currentPpq = isHostPlaying ? hostPpqPosition : internalPpq;

    // Advance active note durations and emit NoteOffs if expired
    for (auto& [inst, state] : activeNotes) {
        if (state.active) {
            if (state.remainingSamples <= numSamples) {
                ScheduledMidiEvent off;
                off.sampleOffset = std::max(0, state.remainingSamples);
                off.channel = state.channel;
                off.pitch = state.pitch;
                off.velocity = 0;
                off.isNoteOn = false;
                outEvents.push_back(off);
                state.active = false;
            } else {
                state.remainingSamples -= numSamples;
            }
        }
    }

    // Step calculation (1/16th = 0.25 beat)
    double stepDivision = 0.25;
    int step = static_cast<int>(std::floor(currentPpq / stepDivision)) % 16;
    if (step < 0) step += 16;

    bool isNewStep = (step != currentStep);
    currentStep = step;

    if (isNewStep) {
        // Find assigned voices by instrument
        std::map<Harmonic::InstrumentId, Harmonic::VoiceAssignment> voiceMap;
        for (const auto& v : currentVoicing.voices) {
            voiceMap[v.instrument] = v;
        }

        double secondsPerBeat = 60.0 / tempoBpm;
        double stepDurationSeconds = stepDivision * secondsPerBeat;
        int stepDurationSamples = static_cast<int>(stepDurationSeconds * sampleRate);

        for (const auto& [inst, track] : activePattern.tracks) {
            if (track.steps.empty()) continue;
            const auto& stepDef = track.steps[step % track.steps.size()];

            if (stepDef.action == Harmonic::StepActionType::Rest) {
                // Terminate any active note for this instrument
                if (activeNotes[inst].active) {
                    ScheduledMidiEvent off;
                    off.sampleOffset = 0;
                    off.channel = activeNotes[inst].channel;
                    off.pitch = activeNotes[inst].pitch;
                    off.velocity = 0;
                    off.isNoteOn = false;
                    outEvents.push_back(off);
                    activeNotes[inst].active = false;
                }
                continue;
            }

            if (voiceMap.find(inst) == voiceMap.end()) continue;
            const auto& voice = voiceMap[inst];

            int targetPitch = voice.midiPitch + (stepDef.octaveOffset * 12);
            if (stepDef.action == Harmonic::StepActionType::ArpUp || stepDef.action == Harmonic::StepActionType::ArpDown) {
                targetPitch = computeArpPitch(inst, stepDef.action, targetPitch);
            }
            targetPitch = std::clamp(targetPitch, 12, 120);

            int gateSamples = static_cast<int>(stepDurationSamples * std::clamp(stepDef.gate, 0.1, 1.0));

            // Stop prior sounding note if still active
            if (activeNotes[inst].active) {
                ScheduledMidiEvent off;
                off.sampleOffset = 0;
                off.channel = activeNotes[inst].channel;
                off.pitch = activeNotes[inst].pitch;
                off.velocity = 0;
                off.isNoteOn = false;
                outEvents.push_back(off);
            }

            // Emit NoteOn
            ScheduledMidiEvent on;
            on.sampleOffset = 0;
            on.channel = voice.midiChannel;
            on.pitch = targetPitch;
            on.velocity = std::clamp(stepDef.velocity, 1, 127);
            on.isNoteOn = true;
            on.articulation = stepDef.articulation;
            outEvents.push_back(on);

            activeNotes[inst] = {targetPitch, voice.midiChannel, gateSamples, true};
        }
    }

    // Advance internal clock
    double deltaSeconds = static_cast<double>(numSamples) / sampleRate;
    double deltaPpq = (deltaSeconds / (60.0 / tempoBpm));
    internalPpq += deltaPpq;
}

} // namespace Sequencer
