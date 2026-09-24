#include "TestHarness.h"
#include "Converter/MidiPresetConverter.h"
#include "MidiExport/StandardMidiWriter.h"
#include "Sequencer/PatternModel.h"
#include "Orchestration/VoicingEngine.h"
#include <iostream>

TEST_CASE(MidiPresetConverter, RoundtripExportAndParse) {
    MidiExport::StandardMidiWriter writer;
    Converter::MidiPresetConverter converter;

    // Create an orchestration in C Major
    auto pattern = Sequencer::createActionOstinatoPattern();

    Harmonic::HarmonicFrame cMaj;
    cMaj.rootPitchClass = 0; // C
    cMaj.bassMidiNote = 36;  // C2
    cMaj.quality = Harmonic::ChordQuality::MajorTriad;
    cMaj.pitches = {36, 48, 52, 55, 60, 64, 67}; // C, E, G across octaves
    cMaj.chordTones = {0, 4, 7};
    cMaj.chordName = "C Major";

    Orchestration::VoicingEngine ve;
    auto voicing = ve.generateVoicing(cMaj);

    std::string midiPath = "/tmp/test_converter_cmaj.mid";
    bool exportOk = writer.exportMidiFile(pattern, voicing, 130.0, 2, midiPath);
    ASSERT_TRUE(exportOk);

    // Parse back using MidiPresetConverter
    Converter::ParsedMidiFile parsed;
    std::string err;
    bool parseOk = converter.parseMidiFile(midiPath, parsed, err);
    ASSERT_TRUE(parseOk);
    ASSERT_TRUE(parsed.tracks.size() > 0);
    ASSERT_EQ(parsed.ticksPerQuarter, 480);

    // Harmonic & Tonal Analysis
    auto tonal = converter.analyzeTonalCenter(parsed);
    // Should detect C (0) as root
    ASSERT_EQ(tonal.detectedRootPitchClass, 0);
    ASSERT_EQ(tonal.detectedRootName, "C");
    ASSERT_TRUE(tonal.confidence > 0.4f);

    // Convert back to OrchestralPattern preset
    Converter::ConversionOptions options;
    options.presetName = "Converted Action Ostinato";
    options.lengthSteps = 16;
    auto convertedPattern = converter.convertToPattern(parsed, tonal, options);

    ASSERT_EQ(convertedPattern.name, "Converted Action Ostinato");
    ASSERT_EQ(convertedPattern.tracks.begin()->second.stepCount, 16);
    ASSERT_TRUE(convertedPattern.tracks.size() > 0);

    // Test JSON serialization of converted pattern
    std::string jsonStr = convertedPattern.toJson();
    ASSERT_TRUE(jsonStr.find("\"name\": \"Converted Action Ostinato\"") != std::string::npos);
    ASSERT_TRUE(jsonStr.find("\"tracks\"") != std::string::npos);
}

TEST_CASE(MidiPresetConverter, MinorTonalityAnalysis) {
    MidiExport::StandardMidiWriter writer;
    Converter::MidiPresetConverter converter;

    // Create an orchestration in D Minor
    auto pattern = Sequencer::createEpicFanfarePattern();

    Harmonic::HarmonicFrame dMin;
    dMin.rootPitchClass = 2; // D
    dMin.bassMidiNote = 38;  // D2
    dMin.quality = Harmonic::ChordQuality::MinorTriad;
    dMin.pitches = {38, 45, 50, 53, 57, 62, 65}; // D, F, A across octaves
    dMin.chordTones = {0, 3, 7};
    dMin.chordName = "D Minor";

    Orchestration::VoicingEngine ve;
    auto voicing = ve.generateVoicing(dMin);

    std::string midiPath = "/tmp/test_converter_dmin.mid";
    bool exportOk = writer.exportMidiFile(pattern, voicing, 115.0, 2, midiPath);
    ASSERT_TRUE(exportOk);

    Converter::ParsedMidiFile parsed;
    std::string err;
    bool parseOk = converter.parseMidiFile(midiPath, parsed, err);
    ASSERT_TRUE(parseOk);

    auto tonal = converter.analyzeTonalCenter(parsed);
    // Should detect D (2) as root
    ASSERT_EQ(tonal.detectedRootPitchClass, 2);
    ASSERT_EQ(tonal.detectedRootName, "D");
    ASSERT_TRUE(tonal.detectedMode == Harmonic::ScaleMode::Aeolian || tonal.detectedChordQuality == Harmonic::ChordQuality::MinorTriad);
}

TEST_CASE(MidiPresetConverter, InstrumentKeywordDetection) {
    Converter::MidiPresetConverter converter;

    ASSERT_EQ(converter.detectInstrument("1st Violins KS", 1, 76), Harmonic::InstrumentId::Violins1);
    ASSERT_EQ(converter.detectInstrument("Violin II legato", 2, 68), Harmonic::InstrumentId::Violins2);
    ASSERT_EQ(converter.detectInstrument("Viola Section", 3, 55), Harmonic::InstrumentId::Violas);
    ASSERT_EQ(converter.detectInstrument("Cello Ostinato", 4, 45), Harmonic::InstrumentId::Cellos);
    ASSERT_EQ(converter.detectInstrument("Contrabass Spiccato", 5, 32), Harmonic::InstrumentId::DoubleBasses);

    ASSERT_EQ(converter.detectInstrument("French Horns a4", 6, 52), Harmonic::InstrumentId::FrenchHorns);
    ASSERT_EQ(converter.detectInstrument("Trumpet 1 Staccato", 7, 65), Harmonic::InstrumentId::Trumpets);
    ASSERT_EQ(converter.detectInstrument("Tenor Trombone", 8, 48), Harmonic::InstrumentId::Trombones);
    ASSERT_EQ(converter.detectInstrument("Tuba Solo", 9, 30), Harmonic::InstrumentId::Tuba);

    ASSERT_EQ(converter.detectInstrument("Flute I Legato", 10, 75), Harmonic::InstrumentId::Flutes);
    ASSERT_EQ(converter.detectInstrument("Oboe Section", 11, 68), Harmonic::InstrumentId::Oboes);
    ASSERT_EQ(converter.detectInstrument("Clarinet in Bb", 12, 60), Harmonic::InstrumentId::Clarinets);
    ASSERT_EQ(converter.detectInstrument("Bassoon 1", 13, 44), Harmonic::InstrumentId::Bassoons);

    ASSERT_EQ(converter.detectInstrument("Timpani Hits", 14, 45), Harmonic::InstrumentId::Timpani);
    ASSERT_EQ(converter.detectInstrument("Orchestral Percussion", 15, 60), Harmonic::InstrumentId::OrchestralPerc);
}
