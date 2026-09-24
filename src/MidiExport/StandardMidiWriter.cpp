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

    // Delta 0: Track Name "Tempo / Master"
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
                                             int numBars) {
    trackBytes.clear();

    // Delta 0: Track Name
    std::string trackName = Harmonic::instrumentToString(inst);
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
        int priority; // NoteOff before NoteOn at identical tick
    };
    std::vector<MidiTickEvent> timeline;

    uint32_t ticksPerStep = ticksPerQuarter / 4; // 1/16th note = 120 ticks
    uint32_t totalSteps = numBars * 16;

    for (uint32_t s = 0; s < totalSteps; ++s) {
        if (trackPattern.steps.empty()) break;
        const auto& step = trackPattern.steps[s % trackPattern.steps.size()];
        if (step.action == Harmonic::StepActionType::Rest) continue;

        uint32_t stepStartTick = s * ticksPerStep;
        uint32_t gateTicks = static_cast<uint32_t>(ticksPerStep * std::clamp(step.gate, 0.1, 1.0));
        uint32_t stepEndTick = stepStartTick + gateTicks;

        uint8_t pitch = static_cast<uint8_t>(std::clamp(voice.midiPitch + (step.octaveOffset * 12), 12, 127));
        uint8_t vel = static_cast<uint8_t>(std::clamp(step.velocity, 1, 127));

        // NoteOn
        timeline.push_back({stepStartTick, static_cast<uint8_t>(0x90 | ch), pitch, vel, 2});
        // NoteOff
        timeline.push_back({stepEndTick, static_cast<uint8_t>(0x80 | ch), pitch, 0, 1});
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
                buildInstrumentTrack(instTrack, inst, trackPattern, voiceMap[inst], numBars);
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
            buildInstrumentTrack(instTrack, stemId, pattern.tracks.at(stemId), voiceMap[stemId], numBars);
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
