#include "StandardMidiWriter.h"
#include <fstream>
#include <algorithm>
#include <map>

namespace MidiExport {

void writeVLQ(std::vector<uint8_t>& buffer, uint32_t value) {
    uint32_t bufferVal = value & 0x7F;
    while ((value >>= 7) > 0) {
        bufferVal <<= 8;
        bufferVal |= 0x80;
        bufferVal += (value & 0x7F);
    }

    while (true) {
        buffer.push_back(static_cast<uint8_t>(bufferVal & 0xFF));
        if (bufferVal & 0x80) {
            bufferVal >>= 8;
        } else {
            break;
        }
    }
}

void writeBigEndian16(std::vector<uint8_t>& buffer, uint16_t value) {
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

void writeBigEndian32(std::vector<uint8_t>& buffer, uint32_t value) {
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

void StandardMidiWriter::buildConductorTrack(std::vector<uint8_t>& trackBytes, double bpm) {
    trackBytes.clear();

    // Delta 0: Track Name "Master Conductor"
    std::string trackName = "Master Conductor";
    writeVLQ(trackBytes, 0);
    trackBytes.push_back(0xFF);
    trackBytes.push_back(0x03);
    writeVLQ(trackBytes, static_cast<uint32_t>(trackName.size()));
    trackBytes.insert(trackBytes.end(), trackName.begin(), trackName.end());

    // Delta 0: Time Signature 4/4
    writeVLQ(trackBytes, 0);
    trackBytes.push_back(0xFF);
    trackBytes.push_back(0x58);
    trackBytes.push_back(0x04);
    trackBytes.push_back(0x04); // Numerator: 4
    trackBytes.push_back(0x02); // Denominator: 2^2 = 4
    trackBytes.push_back(0x18); // MIDI clocks per click: 24
    trackBytes.push_back(0x08); // 32nd notes in quarter note: 8

    // Delta 0: Set Tempo (microseconds per quarter note = 60,000,000 / BPM)
    uint32_t usPerQuarter = static_cast<uint32_t>(60000000.0 / bpm);
    writeVLQ(trackBytes, 0);
    trackBytes.push_back(0xFF);
    trackBytes.push_back(0x51);
    trackBytes.push_back(0x03);
    trackBytes.push_back(static_cast<uint8_t>((usPerQuarter >> 16) & 0xFF));
    trackBytes.push_back(static_cast<uint8_t>((usPerQuarter >> 8) & 0xFF));
    trackBytes.push_back(static_cast<uint8_t>(usPerQuarter & 0xFF));

    // End of Track
    writeVLQ(trackBytes, 0);
    trackBytes.push_back(0xFF);
    trackBytes.push_back(0x2F);
    trackBytes.push_back(0x00);
}

void StandardMidiWriter::buildInstrumentTrack(std::vector<uint8_t>& trackBytes,
                                             Harmonic::InstrumentId inst,
                                             const Sequencer::TrackPattern& trackPattern,
                                             const Harmonic::VoiceAssignment& voice,
                                             const Harmonic::HarmonicFrame& harmonic,
                                             int numBars) {
    trackBytes.clear();

    // Delta 0: Track Name
    std::string trackName = trackPattern.trackName.empty() ? Harmonic::instrumentToString(inst) : trackPattern.trackName;
    writeVLQ(trackBytes, 0);
    trackBytes.push_back(0xFF);
    trackBytes.push_back(0x03);
    writeVLQ(trackBytes, static_cast<uint32_t>(trackName.size()));
    trackBytes.insert(trackBytes.end(), trackName.begin(), trackName.end());

    uint8_t ch = static_cast<uint8_t>(std::clamp(voice.midiChannel - 1, 0, 15));

    // Events timeline in absolute ticks
    struct MidiTickEvent {
        uint32_t tick;
        uint8_t status;
        uint8_t data1;
        uint8_t data2;
        int priority; // NoteOff (1) before CC (2) before NoteOn (3) at identical tick
    };
    std::vector<MidiTickEvent> timeline;

    uint32_t ticksPerStep = ticksPerQuarter / 4; // 1/16th note = 120 ticks
    uint32_t totalSteps = numBars * 16;

    // Collect chord pitch classes
    std::vector<int> chordPcs;
    if (!harmonic.chordTones.empty()) {
        for (int interval : harmonic.chordTones) {
            chordPcs.push_back((harmonic.rootPitchClass + interval) % 12);
        }
    } else if (!harmonic.pitches.empty()) {
        for (int p : harmonic.pitches) {
            chordPcs.push_back((p % 12 + 12) % 12);
        }
    } else {
        chordPcs = {0, 4, 7};
    }
    std::sort(chordPcs.begin(), chordPcs.end());
    chordPcs.erase(std::unique(chordPcs.begin(), chordPcs.end()), chordPcs.end());

    for (uint32_t s = 0; s < totalSteps; ++s) {
        if (trackPattern.steps.empty()) break;
        const auto& step = trackPattern.steps[s % trackPattern.steps.size()];
        uint32_t stepStartTick = s * ticksPerStep;

        // Export CC1 dynamics event if present
        if (!trackPattern.cc1Curve.empty()) {
            uint8_t ccVal = static_cast<uint8_t>(std::clamp(trackPattern.cc1Curve[s % trackPattern.cc1Curve.size()], 0, 127));
            timeline.push_back({stepStartTick, static_cast<uint8_t>(0xB0 | ch), 1, ccVal, 2});
        }

        if (!step.active || step.action == Harmonic::StepActionType::Rest) continue;

        uint32_t lenSteps = static_cast<uint32_t>(std::clamp(step.lengthSteps, 1, 16));
        uint32_t totalNoteTicks = ticksPerStep * lenSteps;
        uint32_t gateTicks = static_cast<uint32_t>(totalNoteTicks * std::clamp(step.gate, 0.1, 1.0));
        uint32_t stepEndTick = stepStartTick + gateTicks;

        // Calculate pitch based on arrangerMode and stepOffset
        int workingBase = voice.midiPitch;
        if (trackPattern.arrangerMode == "Top" && !chordPcs.empty()) {
            int topPc = (!harmonic.chordTones.empty())
                ? ((harmonic.rootPitchClass + harmonic.chordTones.back()) % 12)
                : ((!harmonic.pitches.empty()) ? (harmonic.pitches.back() % 12) : chordPcs.back());
            workingBase = (voice.midiPitch / 12) * 12 + topPc;
        } else if (trackPattern.arrangerMode == "Lowest" && !chordPcs.empty()) {
            int lowPc = (harmonic.bassMidiNote % 12 + 12) % 12;
            workingBase = (voice.midiPitch / 12) * 12 + lowPc;
        } else if (trackPattern.arrangerMode == "Root") {
            workingBase = (voice.midiPitch / 12) * 12 + harmonic.rootPitchClass;
        }

        std::vector<int> allOffsets = { step.stepOffset };
        for (int eo : step.extraOffsets) {
            allOffsets.push_back(eo);
        }

        std::vector<int> pitchLadder;
        if (!chordPcs.empty()) {
            for (int oct = 1; oct <= 9; ++oct) {
                for (int pc : chordPcs) {
                    int p = oct * 12 + pc;
                    if (p >= 12 && p <= 127) {
                        pitchLadder.push_back(p);
                    }
                }
            }
            std::sort(pitchLadder.begin(), pitchLadder.end());
            pitchLadder.erase(std::unique(pitchLadder.begin(), pitchLadder.end()), pitchLadder.end());
        }

        for (int offVal : allOffsets) {
            int calculatedPitch = workingBase;
            if (offVal != 0 && !pitchLadder.empty()) {
                auto it = std::lower_bound(pitchLadder.begin(), pitchLadder.end(), workingBase);
                int idx = static_cast<int>(std::distance(pitchLadder.begin(), it));
                if (idx >= (int)pitchLadder.size()) idx = (int)pitchLadder.size() - 1;
                int targetIdx = std::clamp(idx + offVal, 0, (int)pitchLadder.size() - 1);
                calculatedPitch = pitchLadder[targetIdx];
            }

            calculatedPitch += (trackPattern.octaveOffset * 12) + (step.octaveOffset * 12);
            uint8_t pitch = static_cast<uint8_t>(std::clamp(calculatedPitch, 12, 127));
            uint8_t vel = static_cast<uint8_t>(std::clamp(static_cast<int>(step.velocity * trackPattern.volume), 1, 127));

            // NoteOn
            timeline.push_back({stepStartTick, static_cast<uint8_t>(0x90 | ch), pitch, vel, 3});
            // NoteOff
            timeline.push_back({stepEndTick, static_cast<uint8_t>(0x80 | ch), pitch, 0, 1});
        }
    }

    std::sort(timeline.begin(), timeline.end(), [](const MidiTickEvent& a, const MidiTickEvent& b) {
        if (a.tick != b.tick) return a.tick < b.tick;
        return a.priority < b.priority;
    });

    uint32_t currentTick = 0;
    for (const auto& ev : timeline) {
        uint32_t delta = ev.tick - currentTick;
        currentTick = ev.tick;

        writeVLQ(trackBytes, delta);
        trackBytes.push_back(ev.status);
        trackBytes.push_back(ev.data1);
        trackBytes.push_back(ev.data2);
    }

    // End of Track
    writeVLQ(trackBytes, 0);
    trackBytes.push_back(0xFF);
    trackBytes.push_back(0x2F);
    trackBytes.push_back(0x00);
}

bool StandardMidiWriter::exportMidiFile(const Sequencer::OrchestralPattern& pattern,
                                      const Harmonic::OrchestralVoicing& voicing,
                                      double bpm,
                                      int numBars,
                                      const std::string& destinationFilePath,
                                      std::optional<Harmonic::InstrumentId> singleStem) {
    std::map<Harmonic::InstrumentId, Harmonic::VoiceAssignment> voiceMap;
    for (const auto& v : voicing.voices) {
        voiceMap[v.instrument] = v;
    }

    std::vector<std::vector<uint8_t>> allTracks;

    if (!singleStem.has_value()) {
        // Multi-track Format 1: Track 0 is Conductor, subsequent tracks are instruments
        std::vector<uint8_t> conductorTrack;
        buildConductorTrack(conductorTrack, bpm);
        allTracks.push_back(conductorTrack);

        for (const auto& [inst, trackPattern] : pattern.tracks) {
            if (voiceMap.find(inst) != voiceMap.end()) {
                std::vector<uint8_t> instTrack;
                buildInstrumentTrack(instTrack, inst, trackPattern, voiceMap[inst], voicing.sourceHarmonic, numBars);
                allTracks.push_back(instTrack);
            }
        }
    } else {
        // Single Stem: Format 0 (or Format 1 with conductor + 1 track)
        Harmonic::InstrumentId stemId = singleStem.value();
        if (pattern.tracks.find(stemId) != pattern.tracks.end() && voiceMap.find(stemId) != voiceMap.end()) {
            std::vector<uint8_t> conductorTrack;
            buildConductorTrack(conductorTrack, bpm);
            allTracks.push_back(conductorTrack);

            std::vector<uint8_t> instTrack;
            buildInstrumentTrack(instTrack, stemId, pattern.tracks.at(stemId), voiceMap[stemId], voicing.sourceHarmonic, numBars);
            allTracks.push_back(instTrack);
        }
    }

    if (allTracks.empty()) return false;

    // Write binary SMF to disk
    std::vector<uint8_t> fileBytes;

    // Header Chunk "MThd"
    fileBytes.push_back('M'); fileBytes.push_back('T');
    fileBytes.push_back('h'); fileBytes.push_back('d');
    writeBigEndian32(fileBytes, 6); // Length = 6

    uint16_t format = (allTracks.size() > 1) ? 1 : 0;
    writeBigEndian16(fileBytes, format);
    writeBigEndian16(fileBytes, static_cast<uint16_t>(allTracks.size()));
    writeBigEndian16(fileBytes, ticksPerQuarter);

    // Track Chunks "MTrk"
    for (const auto& t : allTracks) {
        fileBytes.push_back('M'); fileBytes.push_back('T');
        fileBytes.push_back('r'); fileBytes.push_back('k');
        writeBigEndian32(fileBytes, static_cast<uint32_t>(t.size()));
        fileBytes.insert(fileBytes.end(), t.begin(), t.end());
    }

    std::ofstream out(destinationFilePath, std::ios::binary);
    if (!out.is_open()) return false;

    out.write(reinterpret_cast<const char*>(fileBytes.data()), fileBytes.size());
    out.close();

    return true;
}

} // namespace MidiExport
