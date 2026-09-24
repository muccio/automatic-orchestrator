#pragma once

#include "Common/HarmonicTypes.h"
#include "Sequencer/PatternModel.h"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace MidiExport {

void writeVLQ(std::vector<uint8_t>& buffer, uint32_t value);
void writeBigEndian16(std::vector<uint8_t>& buffer, uint16_t value);
void writeBigEndian32(std::vector<uint8_t>& buffer, uint32_t value);

class StandardMidiWriter {
public:
    StandardMidiWriter() = default;

    // Exports the orchestrated pattern to a Standard MIDI File (.mid)
    // If singleStem is nullopt, exports Format 1 with all instrument tracks.
    // If singleStem is provided, exports Format 0 with only that specific instrument track.
    bool exportMidiFile(const Sequencer::OrchestralPattern& pattern,
                        const Harmonic::OrchestralVoicing& voicing,
                        double bpm,
                        int numBars,
                        const std::string& destinationFilePath,
                        std::optional<Harmonic::InstrumentId> singleStem = std::nullopt);

private:
    uint16_t ticksPerQuarter = 480;

    void buildConductorTrack(std::vector<uint8_t>& trackBytes, double bpm);

    void buildInstrumentTrack(std::vector<uint8_t>& trackBytes,
                             Harmonic::InstrumentId inst,
                             const Sequencer::TrackPattern& trackPattern,
                             const Harmonic::VoiceAssignment& voice,
                             int numBars);
};

} // namespace MidiExport
