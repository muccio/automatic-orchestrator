#pragma once

#include "Common/HarmonicTypes.h"
#include "Sequencer/PatternModel.h"
#include "Harmonic/ChordDetector.h"
#include "Harmonic/ScaleQuantizer.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace Converter {

struct MidiNoteEvent {
    int pitch = 60;
    int velocity = 100;
    int64_t startTick = 0;
    int64_t durationTicks = 0;
    int channel = 1;
};

struct MidiCcEvent {
    int ccNumber = 1;
    int value = 64;
    int64_t tick = 0;
    int channel = 1;
};

struct ParsedMidiTrack {
    int trackIndex = 0;
    std::string trackName;
    int channel = 1;
    std::vector<MidiNoteEvent> notes;
    std::vector<MidiCcEvent> ccEvents;

    // Smart auto-detection properties
    Harmonic::InstrumentId suggestedInstrument = Harmonic::InstrumentId::Violins1;
    Harmonic::OrchestralSection suggestedSection = Harmonic::OrchestralSection::Strings;
    Harmonic::ArticulationType suggestedArticulation = Harmonic::ArticulationType::Sustain;
    std::string suggestedArrangerMode = "Top";
    int noteCount = 0;
    int minPitch = 127;
    int maxPitch = 0;
    int averagePitch = 60;
    bool isEnabled = true;
};

struct ParsedMidiFile {
    std::string filePath;
    std::string fileName;
    int format = 1;
    int ticksPerQuarter = 480;
    double bpm = 120.0;
    int timeSigNum = 4;
    int timeSigDen = 4;
    int64_t totalTicks = 0;
    std::vector<ParsedMidiTrack> tracks;
};

struct TonalAnalysisResult {
    int detectedRootPitchClass = 0; // 0 = C, 1 = C#, ..., 11 = B
    std::string detectedRootName = "C";
    Harmonic::ScaleMode detectedMode = Harmonic::ScaleMode::Ionian;
    Harmonic::ChordQuality detectedChordQuality = Harmonic::ChordQuality::MajorTriad;
    std::string detectedChordName = "C Major";
    float confidence = 0.0f; // 0.0 to 1.0
    std::vector<float> pitchClassDistribution; // 12 elements (C .. B)
    int referenceBassMidiNote = 36;
    std::vector<int> detectedChordTones; // intervals from root, e.g. {0, 4, 7}
    std::vector<int> detectedHarmonicPcs; // pitch classes, e.g. {0, 4, 7}
};

struct TrackMappingConfig {
    Harmonic::InstrumentId instrument = Harmonic::InstrumentId::Violins1;
    Harmonic::OrchestralSection section = Harmonic::OrchestralSection::Strings;
    Harmonic::ArticulationType articulation = Harmonic::ArticulationType::Sustain;
    std::string arrangerMode = "Top";
    int octaveOffset = 0;
    float volume = 0.85f;
    float pan = 0.0f;
    bool enabled = true;
};

struct ConversionOptions {
    std::string presetName;
    int overrideRootPitchClass = -1; // -1 to use detected
    Harmonic::ScaleMode overrideMode = Harmonic::ScaleMode::Ionian;
    bool useOverrideMode = false;
    int lengthSteps = 16;
    double tempoBpm = 0.0; // 0 to use file tempo
    std::map<int, TrackMappingConfig> trackConfigs; // key is trackIndex
};

class MidiPresetConverter {
public:
    MidiPresetConverter();
    ~MidiPresetConverter() = default;

    // 1. SMF Parsing
    bool parseMidiFile(const std::string& path, ParsedMidiFile& outData, std::string& errorMsg);
    bool parseMidiBytes(const uint8_t* data, size_t size, ParsedMidiFile& outData, std::string& errorMsg);

    // 2. Harmonic & Tonal Analysis
    TonalAnalysisResult analyzeTonalCenter(const ParsedMidiFile& midiData);

    // 3. Smart Instrument & Articulation Matching
    Harmonic::InstrumentId detectInstrument(const std::string& trackName, int channel, int avgPitch);
    Harmonic::OrchestralSection getSectionForInstrument(Harmonic::InstrumentId inst);
    Harmonic::ArticulationType detectArticulation(const ParsedMidiTrack& track);

    // 4. Pattern Conversion
    Sequencer::OrchestralPattern convertToPattern(const ParsedMidiFile& midiData,
                                                 const TonalAnalysisResult& tonalResult,
                                                 const ConversionOptions& options);

    // 5. Complete High-Level Export
    bool convertAndSavePreset(const std::string& midiPath,
                             const std::string& outputJsonPath,
                             const ConversionOptions& options,
                             std::string& errorMsg);

private:
    Harmonic::ChordDetector chordDetector;
    Harmonic::ScaleQuantizer scaleQuantizer;

    // Helper functions
    static int readVariableLength(const uint8_t*& ptr, const uint8_t* end);
    static uint32_t readBigEndian32(const uint8_t* ptr);
    static uint16_t readBigEndian16(const uint8_t* ptr);
    static float computeCorrelation(const std::vector<float>& x, const std::vector<float>& y);
    static int pitchToScaleDegreeOffset(int midiPitch, int rootMidiNote, Harmonic::ScaleMode mode);
};

} // namespace Converter
