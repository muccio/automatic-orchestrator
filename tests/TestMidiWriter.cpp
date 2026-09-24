#include "TestHarness.h"
#include "MidiExport/StandardMidiWriter.h"
#include "Sequencer/PatternModel.h"
#include "Orchestration/VoicingEngine.h"
#include <fstream>

TEST_CASE(StandardMidiWriter, VariableLengthQuantityEncoding) {
    using namespace MidiExport;
    std::vector<uint8_t> bytes;

    writeVLQ(bytes, 0);
    ASSERT_EQ(bytes.size(), 1);
    ASSERT_EQ(bytes[0], 0x00);

    bytes.clear();
    writeVLQ(bytes, 127);
    ASSERT_EQ(bytes.size(), 1);
    ASSERT_EQ(bytes[0], 0x7F);

    bytes.clear();
    writeVLQ(bytes, 128);
    ASSERT_EQ(bytes.size(), 2);
    ASSERT_EQ(bytes[0], 0x81);
    ASSERT_EQ(bytes[1], 0x00);
}

TEST_CASE(StandardMidiWriter, MultiTrackMidiFileExport) {
    MidiExport::StandardMidiWriter writer;

    auto pattern = Sequencer::createActionOstinatoPattern();

    Harmonic::HarmonicFrame cMaj;
    cMaj.rootPitchClass = 0;
    cMaj.bassMidiNote = 48;
    cMaj.quality = Harmonic::ChordQuality::MajorTriad;
    cMaj.pitches = {48, 52, 55};
    cMaj.chordTones = {0, 4, 7};

    Orchestration::VoicingEngine ve;
    auto voicing = ve.generateVoicing(cMaj);

    std::string tempPath = "/tmp/test_orchestrator_master.mid";
    bool ok = writer.exportMidiFile(pattern, voicing, 120.0, 2, tempPath);
    ASSERT_TRUE(ok);

    // Read back and inspect binary header
    std::ifstream file(tempPath, std::ios::binary);
    ASSERT_TRUE(file.is_open());

    char header[4];
    file.read(header, 4);
    // Check "MThd" magic signature
    ASSERT_TRUE(header[0] == 'M' && header[1] == 'T' && header[2] == 'h' && header[3] == 'd');

    // Read header length (must be 6)
    uint8_t hlen[4];
    file.read(reinterpret_cast<char*>(hlen), 4);
    uint32_t headerLength = (hlen[0] << 24) | (hlen[1] << 16) | (hlen[2] << 8) | hlen[3];
    ASSERT_EQ(headerLength, 6);

    // Read format (must be 1 for multi-track)
    uint8_t fmt[2];
    file.read(reinterpret_cast<char*>(fmt), 2);
    uint16_t format = (fmt[0] << 8) | fmt[1];
    ASSERT_EQ(format, 1);
}

TEST_CASE(StandardMidiWriter, SingleStemMidiFileExport) {
    MidiExport::StandardMidiWriter writer;

    auto pattern = Sequencer::createActionOstinatoPattern();

    Harmonic::HarmonicFrame cMaj;
    cMaj.rootPitchClass = 0;
    cMaj.bassMidiNote = 48;
    cMaj.quality = Harmonic::ChordQuality::MajorTriad;
    cMaj.pitches = {48, 52, 55};
    cMaj.chordTones = {0, 4, 7};

    Orchestration::VoicingEngine ve;
    auto voicing = ve.generateVoicing(cMaj);

    std::string stemPath = "/tmp/test_orchestrator_violins1.mid";
    bool ok = writer.exportMidiFile(pattern, voicing, 120.0, 2, stemPath, Harmonic::InstrumentId::Violins1);
    ASSERT_TRUE(ok);

    std::ifstream file(stemPath, std::ios::binary);
    ASSERT_TRUE(file.is_open());

    char header[4];
    file.read(header, 4);
    ASSERT_TRUE(header[0] == 'M' && header[1] == 'T' && header[2] == 'h' && header[3] == 'd');
}
