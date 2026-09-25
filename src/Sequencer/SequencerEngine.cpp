#include "SequencerEngine.h"
#include <cmath>
#include <algorithm>

namespace Sequencer {

SequencerEngine::SequencerEngine() {
    activePattern = createActionOstinatoPattern();
}

void SequencerEngine::setPattern(const OrchestralPattern& pattern) {
    std::lock_guard<std::mutex> lock(patternMutex);
    activePattern = pattern;
    tempoBpm = pattern.bpm > 0 ? pattern.bpm : 120.0;
}

OrchestralPattern SequencerEngine::getPattern() const {
    std::lock_guard<std::mutex> lock(patternMutex);
    return activePattern;
}

void SequencerEngine::updateVoicing(const Harmonic::OrchestralVoicing& voicing) {
    currentVoicing = voicing;
}

TrackPattern& SequencerEngine::ensureTrackExistsLocked(Harmonic::InstrumentId inst) {
    auto it = activePattern.tracks.find(inst);
    if (it != activePattern.tracks.end()) {
        return it->second;
    }
    TrackPattern tp;
    tp.instrument = inst;
    tp.trackName = Harmonic::instrumentToString(inst);
    tp.section = Harmonic::getInstrumentSection(inst);
    tp.midiChannel = Harmonic::getDefaultInstrumentChannel(inst);
    tp.articulation = Harmonic::ArticulationType::Sustain;
    tp.arrangerMode = "Top";
    tp.octaveOffset = 0;
    tp.volume = 0.85f;
    tp.pan = 0.0f;
    tp.isMuted = false;
    tp.isSolo = false;
    tp.stepCount = 16;
    tp.stepDivision = 0.25;
    tp.cc1Curve = std::vector<int>(16, 75);
    tp.steps.resize(16);
    for (int i = 0; i < 16; ++i) {
        tp.steps[i].active = false;
        tp.steps[i].action = Harmonic::StepActionType::Rest;
        tp.steps[i].articulation = tp.articulation;
        tp.steps[i].lengthSteps = 1;
        tp.steps[i].velocity = 90;
        tp.steps[i].gate = 0.85;
    }
    activePattern.tracks[inst] = tp;
    return activePattern.tracks[inst];
}

void SequencerEngine::setTrackStep(Harmonic::InstrumentId inst, int stepIndex, bool active, int stepOffset, int velocity, Harmonic::ArticulationType art) {
    std::lock_guard<std::mutex> lock(patternMutex);
    auto& track = ensureTrackExistsLocked(inst);
    if (stepIndex >= 0 && stepIndex < (int)track.steps.size()) {
        track.steps[stepIndex].active = active;
        track.steps[stepIndex].stepOffset = stepOffset;
        track.steps[stepIndex].extraOffsets.clear();
        track.steps[stepIndex].velocity = std::clamp(velocity, 1, 127);
        track.steps[stepIndex].articulation = art;
        track.steps[stepIndex].action = active ? Harmonic::StepActionType::Ostinato : Harmonic::StepActionType::Rest;
    }
}

void SequencerEngine::setTrackStepWithExtras(Harmonic::InstrumentId inst, int stepIndex, bool active, int stepOffset, const std::vector<int>& extraOffsets, int velocity, Harmonic::ArticulationType art) {
    std::lock_guard<std::mutex> lock(patternMutex);
    auto& track = ensureTrackExistsLocked(inst);
    if (stepIndex >= 0 && stepIndex < (int)track.steps.size()) {
        track.steps[stepIndex].active = active;
        track.steps[stepIndex].stepOffset = stepOffset;
        track.steps[stepIndex].extraOffsets = extraOffsets;
        track.steps[stepIndex].velocity = std::clamp(velocity, 1, 127);
        track.steps[stepIndex].articulation = art;
        track.steps[stepIndex].action = active ? Harmonic::StepActionType::Ostinato : Harmonic::StepActionType::Rest;
    }
}

void SequencerEngine::addTrackStepOffset(Harmonic::InstrumentId inst, int stepIndex, int offset, int velocity, Harmonic::ArticulationType art) {
    std::lock_guard<std::mutex> lock(patternMutex);
    auto& track = ensureTrackExistsLocked(inst);
    if (stepIndex >= 0 && stepIndex < (int)track.steps.size()) {
        auto& stp = track.steps[stepIndex];
        if (!stp.active) {
            stp.active = true;
            stp.stepOffset = offset;
            stp.extraOffsets.clear();
        } else {
            if (stp.stepOffset != offset && std::find(stp.extraOffsets.begin(), stp.extraOffsets.end(), offset) == stp.extraOffsets.end()) {
                stp.extraOffsets.push_back(offset);
            }
        }
        stp.velocity = std::clamp(velocity, 1, 127);
        stp.articulation = art;
        stp.action = Harmonic::StepActionType::Ostinato;
    }
}

void SequencerEngine::removeTrackStepOffset(Harmonic::InstrumentId inst, int stepIndex, int offset) {
    std::lock_guard<std::mutex> lock(patternMutex);
    auto& track = ensureTrackExistsLocked(inst);
    if (stepIndex >= 0 && stepIndex < (int)track.steps.size()) {
        auto& stp = track.steps[stepIndex];
        if (stp.stepOffset == offset) {
            if (!stp.extraOffsets.empty()) {
                stp.stepOffset = stp.extraOffsets.front();
                stp.extraOffsets.erase(stp.extraOffsets.begin());
            } else {
                stp.active = false;
                stp.action = Harmonic::StepActionType::Rest;
            }
        } else {
            auto it = std::find(stp.extraOffsets.begin(), stp.extraOffsets.end(), offset);
            if (it != stp.extraOffsets.end()) {
                stp.extraOffsets.erase(it);
            }
        }
    }
}

void SequencerEngine::setTrackStepLength(Harmonic::InstrumentId inst, int stepIndex, int lengthSteps) {
    std::lock_guard<std::mutex> lock(patternMutex);
    auto& track = ensureTrackExistsLocked(inst);
    if (stepIndex >= 0 && stepIndex < (int)track.steps.size()) {
        track.steps[stepIndex].lengthSteps = std::clamp(lengthSteps, 1, 16 - stepIndex);
    }
}

void SequencerEngine::setTrackArticulation(Harmonic::InstrumentId inst, Harmonic::ArticulationType art) {
    std::lock_guard<std::mutex> lock(patternMutex);
    auto& track = ensureTrackExistsLocked(inst);
    track.articulation = art;
    for (auto& s : track.steps) {
        s.articulation = art;
    }
}

void SequencerEngine::setTrackMode(Harmonic::InstrumentId inst, const std::string& mode) {
    std::lock_guard<std::mutex> lock(patternMutex);
    auto& track = ensureTrackExistsLocked(inst);
    track.arrangerMode = mode;
}

void SequencerEngine::setTrackOctave(Harmonic::InstrumentId inst, int octave) {
    std::lock_guard<std::mutex> lock(patternMutex);
    auto& track = ensureTrackExistsLocked(inst);
    track.octaveOffset = std::clamp(octave, -2, 2);
}

void SequencerEngine::setTrackVolume(Harmonic::InstrumentId inst, float vol) {
    std::lock_guard<std::mutex> lock(patternMutex);
    auto& track = ensureTrackExistsLocked(inst);
    track.volume = std::clamp(vol, 0.0f, 1.0f);
}

void SequencerEngine::setTrackPan(Harmonic::InstrumentId inst, float pan) {
    std::lock_guard<std::mutex> lock(patternMutex);
    auto& track = ensureTrackExistsLocked(inst);
    track.pan = std::clamp(pan, -1.0f, 1.0f);
}

void SequencerEngine::setTrackMute(Harmonic::InstrumentId inst, bool mute) {
    std::lock_guard<std::mutex> lock(patternMutex);
    auto& track = ensureTrackExistsLocked(inst);
    track.isMuted = mute;
}

void SequencerEngine::setTrackSolo(Harmonic::InstrumentId inst, bool solo) {
    std::lock_guard<std::mutex> lock(patternMutex);
    auto& track = ensureTrackExistsLocked(inst);
    track.isSolo = solo;
}

void SequencerEngine::setTrackCc1(Harmonic::InstrumentId inst, int stepIndex, int cc1Val) {
    std::lock_guard<std::mutex> lock(patternMutex);
    auto& track = ensureTrackExistsLocked(inst);
    if (stepIndex >= 0 && stepIndex < (int)track.cc1Curve.size()) {
        track.cc1Curve[stepIndex] = std::clamp(cc1Val, 0, 127);
    }
}

void SequencerEngine::addTrack(Harmonic::InstrumentId inst, const std::string& name, Harmonic::OrchestralSection sec, int channel, Harmonic::ArticulationType art) {
    std::lock_guard<std::mutex> lock(patternMutex);
    auto& track = ensureTrackExistsLocked(inst);
    if (!name.empty()) track.trackName = name;
    track.section = sec;
    track.midiChannel = (channel >= 1 && channel <= 16) ? channel : Harmonic::getDefaultInstrumentChannel(inst);
    track.articulation = art;
}

void SequencerEngine::removeTrack(Harmonic::InstrumentId inst) {
    std::lock_guard<std::mutex> lock(patternMutex);
    activePattern.tracks.erase(inst);
}

int SequencerEngine::computeRelativeStepPitch(Harmonic::InstrumentId inst,
                                             const TrackPattern& track,
                                             int stepOffset,
                                             int basePitch,
                                             int stepOctave) {
    // Collect unique pitch classes from current harmonic frame
    std::vector<int> chordPcs;
    for (int p : currentVoicing.sourceHarmonic.pitches) {
        chordPcs.push_back((p % 12 + 12) % 12);
    }
    if (chordPcs.empty()) {
        chordPcs = {0, 4, 7}; // Default C Major triad
    }
    std::sort(chordPcs.begin(), chordPcs.end());
    chordPcs.erase(std::unique(chordPcs.begin(), chordPcs.end()), chordPcs.end());

    int workingBase = basePitch;

    // Apply Arranger Mode
    if (track.arrangerMode == "Top" && !currentVoicing.sourceHarmonic.pitches.empty()) {
        int maxP = currentVoicing.sourceHarmonic.pitches.back();
        // Shift octave to match basePitch octave register
        int targetOctave = basePitch / 12;
        workingBase = (targetOctave * 12) + (maxP % 12);
    } else if (track.arrangerMode == "Lowest" && !currentVoicing.sourceHarmonic.pitches.empty()) {
        int minP = currentVoicing.sourceHarmonic.pitches.front();
        int targetOctave = basePitch / 12;
        workingBase = (targetOctave * 12) + (minP % 12);
    } else if (track.arrangerMode == "Root") {
        int rootPc = currentVoicing.sourceHarmonic.rootPitchClass;
        int targetOctave = basePitch / 12;
        workingBase = (targetOctave * 12) + rootPc;
    } else if (track.arrangerMode == "Arp Up" || track.arrangerMode == "Arp Down") {
        workingBase = computeArpPitch(inst, (track.arrangerMode == "Arp Up" ? Harmonic::StepActionType::ArpUp : Harmonic::StepActionType::ArpDown), workingBase);
    }

    // Now apply stepOffset relative to chord tones
    int offset = stepOffset;
    int calculatedPitch = workingBase;

    if (offset != 0 && !chordPcs.empty()) {
        // Build ladder of chord pitches covering full register
        std::vector<int> pitchLadder;
        int startOctave = (workingBase / 12) - 3;
        for (int oct = startOctave; oct <= startOctave + 6; ++oct) {
            for (int pc : chordPcs) {
                pitchLadder.push_back(oct * 12 + pc);
            }
        }
        std::sort(pitchLadder.begin(), pitchLadder.end());
        pitchLadder.erase(std::unique(pitchLadder.begin(), pitchLadder.end()), pitchLadder.end());

        // Find closest element in ladder to workingBase
        auto it = std::lower_bound(pitchLadder.begin(), pitchLadder.end(), workingBase);
        int idx = static_cast<int>(std::distance(pitchLadder.begin(), it));
        if (idx >= (int)pitchLadder.size()) idx = (int)pitchLadder.size() - 1;

        int targetIdx = std::clamp(idx + offset, 0, (int)pitchLadder.size() - 1);
        calculatedPitch = pitchLadder[targetIdx];
    }

    // Add track octave and step octave
    calculatedPitch += (track.octaveOffset * 12) + (stepOctave * 12);
    return std::clamp(calculatedPitch, 12, 127);
}

int SequencerEngine::computeRelativeStepPitch(Harmonic::InstrumentId inst,
                                             const TrackPattern& track,
                                             const StepDefinition& stepDef,
                                             int basePitch) {
    return computeRelativeStepPitch(inst, track, stepDef.stepOffset, basePitch, stepDef.octaveOffset);
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
    for (auto& [inst, states] : activeNotes) {
        for (auto& state : states) {
            if (state.active) {
                ScheduledMidiEvent off;
                off.sampleOffset = 0;
                off.channel = state.channel;
                off.pitch = state.pitch;
                off.velocity = 0;
                off.isNoteOn = false;
                off.instrument = inst;
                outEvents.push_back(off);
                state.active = false;
            }
        }
        states.clear();
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
    for (auto& [inst, states] : activeNotes) {
        for (auto it = states.begin(); it != states.end(); ) {
            if (it->active) {
                if (it->remainingSamples <= numSamples) {
                    ScheduledMidiEvent off;
                    off.sampleOffset = std::max(0, it->remainingSamples);
                    off.channel = it->channel;
                    off.pitch = it->pitch;
                    off.velocity = 0;
                    off.isNoteOn = false;
                    off.instrument = inst;
                    outEvents.push_back(off);
                    it = states.erase(it);
                    continue;
                } else {
                    it->remainingSamples -= numSamples;
                }
            }
            ++it;
        }
    }

    // Step calculation (1/16th = 0.25 beat)
    double stepDivision = 0.25;
    int step = static_cast<int>(std::floor(currentPpq / stepDivision)) % 16;
    if (step < 0) step += 16;

    bool isNewStep = (step != currentStep);
    currentStep = step;

    if (isNewStep) {
        std::lock_guard<std::mutex> lock(patternMutex);

        // Find assigned voices by instrument
        std::map<Harmonic::InstrumentId, Harmonic::VoiceAssignment> voiceMap;
        for (const auto& v : currentVoicing.voices) {
            voiceMap[v.instrument] = v;
        }

        double secondsPerBeat = 60.0 / tempoBpm;
        double stepDurationSeconds = stepDivision * secondsPerBeat;
        int stepDurationSamples = static_cast<int>(stepDurationSeconds * sampleRate);

        // Check if any track is soloed
        bool anySolo = false;
        for (const auto& [inst, track] : activePattern.tracks) {
            if (track.isSolo) { anySolo = true; break; }
        }

        for (const auto& [inst, track] : activePattern.tracks) {
            if (track.steps.empty()) continue;

            // Mute / Solo check
            if (anySolo) {
                if (!track.isSolo) continue;
            } else if (track.isMuted) {
                continue;
            }

            const auto& stepDef = track.steps[step % track.steps.size()];

            if (voiceMap.find(inst) == voiceMap.end()) continue;
            const auto& voice = voiceMap[inst];

            // Send CC1 dynamics automation if configured
            if (!track.cc1Curve.empty()) {
                int cc1Val = track.cc1Curve[step % track.cc1Curve.size()];
                ScheduledMidiEvent cc;
                cc.sampleOffset = 0;
                cc.channel = voice.midiChannel;
                cc.pitch = 1; // CC1 (Modulation)
                cc.velocity = std::clamp(cc1Val, 0, 127);
                cc.isNoteOn = false;
                cc.isController = true;
                cc.instrument = inst;
                outEvents.push_back(cc);
            }

            if (stepDef.action == Harmonic::StepActionType::Rest) {
                // Terminate any active notes for this instrument on explicit rest
                if (activeNotes.find(inst) != activeNotes.end()) {
                    for (auto& st : activeNotes[inst]) {
                        if (st.active) {
                            ScheduledMidiEvent off;
                            off.sampleOffset = 0;
                            off.channel = st.channel;
                            off.pitch = st.pitch;
                            off.velocity = 0;
                            off.isNoteOn = false;
                            off.instrument = inst;
                            outEvents.push_back(off);
                            st.active = false;
                        }
                    }
                    activeNotes[inst].clear();
                }
                continue;
            }

            if (!stepDef.active) {
                // Inactive step: If an active note is already ringing across this step, let it ring!
                continue;
            }

            int lengthSteps = std::clamp(stepDef.lengthSteps, 1, 16);
            int gateSamples = static_cast<int>(stepDurationSamples * lengthSteps * std::clamp(stepDef.gate, 0.1, 1.0));

            // Apply track volume scaling to velocity
            int scaledVel = static_cast<int>(stepDef.velocity * track.volume);
            scaledVel = std::clamp(scaledVel, 1, 127);

            // Stop prior sounding notes if still active before triggering new ones
            if (activeNotes.find(inst) != activeNotes.end()) {
                for (auto& st : activeNotes[inst]) {
                    if (st.active) {
                        ScheduledMidiEvent off;
                        off.sampleOffset = 0;
                        off.channel = st.channel;
                        off.pitch = st.pitch;
                        off.velocity = 0;
                        off.isNoteOn = false;
                        off.instrument = inst;
                        outEvents.push_back(off);
                        st.active = false;
                    }
                }
                activeNotes[inst].clear();
            }

            // Collect all offsets for polyphony (single note, dyad, or triad)
            std::vector<int> allOffsets = { stepDef.stepOffset };
            for (int eo : stepDef.extraOffsets) {
                allOffsets.push_back(eo);
            }

            for (int offVal : allOffsets) {
                int targetPitch = computeRelativeStepPitch(inst, track, offVal, voice.midiPitch, stepDef.octaveOffset);

                // Emit NoteOn
                ScheduledMidiEvent on;
                on.sampleOffset = 0;
                on.channel = voice.midiChannel;
                on.pitch = targetPitch;
                on.velocity = scaledVel;
                on.isNoteOn = true;
                on.isController = false;
                on.articulation = stepDef.articulation;
                on.instrument = inst;
                outEvents.push_back(on);

                activeNotes[inst].push_back({targetPitch, voice.midiChannel, gateSamples, true});
            }
        }
    }

    // Advance internal clock
    double deltaSeconds = static_cast<double>(numSamples) / sampleRate;
    double deltaPpq = (deltaSeconds / (60.0 / tempoBpm));
    internalPpq += deltaPpq;
}

} // namespace Sequencer
