#include "TestHarness.h"
#include "Converter/MidiPresetConverter.h"
#include "MidiExport/StandardMidiWriter.h"
#include "Sequencer/PatternModel.h"
#include "Sequencer/SequencerEngine.h"
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

TEST_CASE(MidiPresetConverter, HarmonicLadderArpeggioAndSustain) {
    Converter::MidiPresetConverter converter;

    // Construct a synthetic ParsedMidiFile with an arpeggio and sustained notes in C Major
    Converter::ParsedMidiFile midi;
    midi.ticksPerQuarter = 480;
    midi.bpm = 120.0;
    midi.fileName = "ArpAndSustainTest";

    // Track 1: Violins 1 with 16th-note arpeggio (C4, E4, G4, C5)
    Converter::ParsedMidiTrack arpTrack;
    arpTrack.trackIndex = 0;
    arpTrack.trackName = "Violins 1 Arp";
    arpTrack.suggestedInstrument = Harmonic::InstrumentId::Violins1;
    arpTrack.suggestedArrangerMode = "Lowest";
    arpTrack.channel = 1;

    // 120 ticks per 16th note
    int t16 = 120;
    // Step 0: C4 (60)
    arpTrack.notes.push_back({60, 100, 0 * t16, t16, 1});
    // Step 1: E4 (64)
    arpTrack.notes.push_back({64, 105, 1 * t16, t16, 1});
    // Step 2: G4 (67)
    arpTrack.notes.push_back({67, 110, 2 * t16, t16, 1});
    // Step 3: C5 (72)
    arpTrack.notes.push_back({72, 115, 3 * t16, t16, 1});
    arpTrack.averagePitch = 66;
    arpTrack.minPitch = 60;
    arpTrack.maxPitch = 72;
    arpTrack.noteCount = 4;
    midi.tracks.push_back(arpTrack);

    // Track 2: Cellos with sustained held note (C3 for 8 steps)
    Converter::ParsedMidiTrack susTrack;
    susTrack.trackIndex = 1;
    susTrack.trackName = "Cellos Sustain";
    susTrack.suggestedInstrument = Harmonic::InstrumentId::Cellos;
    susTrack.suggestedArrangerMode = "Root";
    susTrack.channel = 4;
    susTrack.notes.push_back({48, 90, 0, 8 * t16, 4}); // C3 held for 8 steps
    susTrack.averagePitch = 48;
    susTrack.minPitch = 48;
    susTrack.maxPitch = 48;
    susTrack.noteCount = 1;
    midi.tracks.push_back(susTrack);

    // Tonal result: C Major
    Converter::TonalAnalysisResult tonal;
    tonal.detectedRootPitchClass = 0;
    tonal.detectedRootName = "C";
    tonal.detectedChordQuality = Harmonic::ChordQuality::MajorTriad;
    tonal.detectedMode = Harmonic::ScaleMode::Ionian;
    tonal.detectedChordTones = {0, 4, 7};
    tonal.detectedHarmonicPcs = {0, 4, 7};

    Converter::ConversionOptions options;
    options.presetName = "Arp Sustain Preset";
    options.lengthSteps = 16;

    auto pattern = converter.convertToPattern(midi, tonal, options);
    ASSERT_TRUE(pattern.tracks.find(Harmonic::InstrumentId::Violins1) != pattern.tracks.end());
    ASSERT_TRUE(pattern.tracks.find(Harmonic::InstrumentId::Cellos) != pattern.tracks.end());

    const auto& v1Pattern = pattern.tracks.at(Harmonic::InstrumentId::Violins1);
    // In C Major pitch ladder: Lowest in octave 5 (or 4 depending on workingBase)
    // Step 0: Root (offset 0)
    ASSERT_TRUE(v1Pattern.steps[0].active);
    ASSERT_EQ(v1Pattern.steps[0].stepOffset, 0);

    // Step 1: 3rd (offset +1 in chord tone ladder!)
    ASSERT_TRUE(v1Pattern.steps[1].active);
    ASSERT_EQ(v1Pattern.steps[1].stepOffset, 1);

    // Step 2: 5th (offset +2 in chord tone ladder!)
    ASSERT_TRUE(v1Pattern.steps[2].active);
    ASSERT_EQ(v1Pattern.steps[2].stepOffset, 2);

    // Step 3: Octave (offset +3 in chord tone ladder!)
    ASSERT_TRUE(v1Pattern.steps[3].active);
    ASSERT_EQ(v1Pattern.steps[3].stepOffset, 3);

    // Cellos sustained note
    const auto& vcPattern = pattern.tracks.at(Harmonic::InstrumentId::Cellos);
    ASSERT_TRUE(vcPattern.steps[0].active);
    ASSERT_EQ(vcPattern.steps[0].stepOffset, 0); // Root
    ASSERT_EQ(vcPattern.steps[0].lengthSteps, 8); // 8 steps held
    ASSERT_EQ(vcPattern.steps[0].action, Harmonic::StepActionType::Sustain);
}

TEST_CASE(MidiPresetConverter, DyadsAndTriadsPolyphonyConversion) {
    Converter::MidiPresetConverter converter;

    Converter::ParsedMidiFile midi;
    midi.ticksPerQuarter = 480;
    midi.bpm = 120.0;
    midi.fileName = "PolyphonyTest";

    // Track with a Dyad at step 0 (E4 + G4) and a Triad at step 4 (C4 + E4 + G4)
    Converter::ParsedMidiTrack hornTrack;
    hornTrack.trackIndex = 0;
    hornTrack.trackName = "French Horns Chords";
    hornTrack.suggestedInstrument = Harmonic::InstrumentId::FrenchHorns;
    hornTrack.suggestedArrangerMode = "Lowest";
    hornTrack.channel = 6;

    int t16 = 120;
    // Step 0: Dyad E4 (64) + G4 (67)
    hornTrack.notes.push_back({64, 95, 0, 2 * t16, 6});
    hornTrack.notes.push_back({67, 100, 0, 2 * t16, 6});

    // Step 4: Triad C4 (60) + E4 (64) + G4 (67)
    hornTrack.notes.push_back({60, 90, 4 * t16, 4 * t16, 6});
    hornTrack.notes.push_back({64, 95, 4 * t16, 4 * t16, 6});
    hornTrack.notes.push_back({67, 100, 4 * t16, 4 * t16, 6});

    hornTrack.averagePitch = 64;
    hornTrack.minPitch = 60;
    hornTrack.maxPitch = 67;
    hornTrack.noteCount = 5;
    midi.tracks.push_back(hornTrack);

    Converter::TonalAnalysisResult tonal;
    tonal.detectedRootPitchClass = 0;
    tonal.detectedRootName = "C";
    tonal.detectedChordQuality = Harmonic::ChordQuality::MajorTriad;
    tonal.detectedMode = Harmonic::ScaleMode::Ionian;
    tonal.detectedChordTones = {0, 4, 7};
    tonal.detectedHarmonicPcs = {0, 4, 7};

    Converter::ConversionOptions options;
    options.presetName = "Horn Chords Preset";
    options.lengthSteps = 16;

    auto pattern = converter.convertToPattern(midi, tonal, options);
    ASSERT_TRUE(pattern.tracks.find(Harmonic::InstrumentId::FrenchHorns) != pattern.tracks.end());

    const auto& fh = pattern.tracks.at(Harmonic::InstrumentId::FrenchHorns);

    // Step 0: Dyad E4 (1) and G4 (2) relative to workingBase C4 (0)
    ASSERT_TRUE(fh.steps[0].active);
    ASSERT_EQ(fh.steps[0].stepOffset, 1); // E4 is degree +1
    ASSERT_EQ(fh.steps[0].extraOffsets.size(), 1u);
    ASSERT_EQ(fh.steps[0].extraOffsets[0], 2); // G4 is degree +2

    // Step 4: Triad C4 (0), E4 (1), G4 (2)
    ASSERT_TRUE(fh.steps[4].active);
    ASSERT_EQ(fh.steps[4].stepOffset, 0); // C4 is root (0)
    ASSERT_EQ(fh.steps[4].extraOffsets.size(), 2u);
    ASSERT_EQ(fh.steps[4].extraOffsets[0], 1); // E4 is degree +1
    ASSERT_EQ(fh.steps[4].extraOffsets[1], 2); // G4 is degree +2
}

TEST_CASE(MidiPresetConverter, SequencerEnginePlaysPolyphonicPreset) {
    Sequencer::SequencerEngine engine;

    // Build a pattern with a dyad (offset 0 and +1)
    Sequencer::OrchestralPattern pattern;
    Sequencer::TrackPattern trk;
    trk.instrument = Harmonic::InstrumentId::FrenchHorns;
    trk.arrangerMode = "Lowest";
    trk.stepCount = 16;
    trk.steps.resize(16);
    trk.steps[0].active = true;
    trk.steps[0].stepOffset = 0; // Root
    trk.steps[0].extraOffsets = { 1 }; // 3rd (Dyad!)
    trk.steps[0].lengthSteps = 1;
    trk.steps[0].velocity = 100;
    trk.steps[0].action = Harmonic::StepActionType::Ostinato;

    pattern.tracks[Harmonic::InstrumentId::FrenchHorns] = trk;
    engine.setPattern(pattern);

    // Provide C Major voicing
    Harmonic::HarmonicFrame cMaj;
    cMaj.rootPitchClass = 0;
    cMaj.quality = Harmonic::ChordQuality::MajorTriad;
    cMaj.pitches = {60, 64, 67}; // C4, E4, G4

    Harmonic::OrchestralVoicing voicing;
    voicing.sourceHarmonic = cMaj;
    voicing.voices.push_back({Harmonic::InstrumentId::FrenchHorns, 60, 100, Harmonic::ArticulationType::Sustain, 6});
    engine.updateVoicing(voicing);

    std::vector<Sequencer::ScheduledMidiEvent> events;
    // Process block triggering step 0
    engine.processBlock(512, 44100.0, 0.0, true, events);

    // Must emit TWO NoteOns: one for root (60) and one for 3rd (64)
    std::vector<int> pitchesSounded;
    for (const auto& ev : events) {
        if (ev.isNoteOn) pitchesSounded.push_back(ev.pitch);
    }
    ASSERT_EQ(pitchesSounded.size(), 2u);
    std::sort(pitchesSounded.begin(), pitchesSounded.end());
    ASSERT_EQ(pitchesSounded[0], 60); // C4
    ASSERT_EQ(pitchesSounded[1], 64); // E4
}

TEST_CASE(MidiPresetConverter, InspectTestConvertMid) {
    Converter::MidiPresetConverter converter;
    Converter::ParsedMidiFile parsed;
    std::string err;
    bool ok = converter.parseMidiFile("example/testConvert.mid", parsed, err);
    std::cout << "\n=== InspectTestConvertMid ===" << std::endl;
    std::cout << "Parse OK: " << ok << ", Err: " << err << std::endl;
    if (ok) {
        std::cout << "File: " << parsed.fileName << ", TicksPerQuarter: " << parsed.ticksPerQuarter
                  << ", BPM: " << parsed.bpm << ", TimeSig: " << parsed.timeSigNum << "/" << parsed.timeSigDen
                  << ", TotalTicks: " << parsed.totalTicks << std::endl;
        int ticksPer16th = parsed.ticksPerQuarter / 4;
        int total16ths = parsed.totalTicks / ticksPer16th;
        std::cout << "Total 16th steps: " << total16ths << " (approx " << (total16ths / 16.0) << " bars)" << std::endl;

        for (const auto& trk : parsed.tracks) {
            std::cout << "Track " << trk.trackIndex << ": '" << trk.trackName << "' ch=" << trk.channel
                      << " notes=" << trk.notes.size() << " suggested=" << Harmonic::instrumentToString(trk.suggestedInstrument)
                      << " mode=" << trk.suggestedArrangerMode << std::endl;
            for (const auto& n : trk.notes) {
                std::cout << "   startTick=" << n.startTick << " (step " << (n.startTick / ticksPer16th)
                          << " bar " << (n.startTick / (ticksPer16th * 16) + 1)
                          << ") durTicks=" << n.durationTicks << " (durSteps " << (n.durationTicks / (double)ticksPer16th)
                          << ") pitch=" << n.pitch << " vel=" << n.velocity << std::endl;
            }
        }

        auto tonal = converter.analyzeTonalCenter(parsed);
        std::cout << "Tonal: " << tonal.detectedChordName << ", root=" << tonal.detectedRootName
                  << ", conf=" << tonal.confidence << std::endl;
        std::cout << "Harmonic PCs: ";
        for (int pc : tonal.detectedHarmonicPcs) std::cout << pc << " ";
        std::cout << std::endl;
    }
}

TEST_CASE(MidiPresetConverter, TestConvertMidSimilarityRoundtrip) {
    Converter::MidiPresetConverter converter;
    Converter::ParsedMidiFile origMidi;
    std::string err;
    bool ok = converter.parseMidiFile("example/testConvert.mid", origMidi, err);
    ASSERT_TRUE(ok);

    auto tonal = converter.analyzeTonalCenter(origMidi);

    // Auto-detect number of 16th steps in the file
    int ticksPer16th = origMidi.ticksPerQuarter / 4;
    int detectedSteps = static_cast<int>(std::ceil(static_cast<double>(origMidi.totalTicks) / ticksPer16th));
    // Round up to multiple of 16 (full bars)
    int barCount = std::max(1, (detectedSteps + 15) / 16);
    int totalSteps = barCount * 16;
    std::cout << "Detected totalSteps: " << totalSteps << " (" << barCount << " bars)" << std::endl;

    Converter::ConversionOptions options;
    options.presetName = "testConvert_Roundtrip";
    options.lengthSteps = totalSteps;

    auto pattern = converter.convertToPattern(origMidi, tonal, options);
    std::cout << "Pattern tracks count: " << pattern.tracks.size() << std::endl;
    for (const auto& [inst, trk] : pattern.tracks) {
        int activeCount = 0;
        for (const auto& stp : trk.steps) if (stp.active) ++activeCount;
        std::cout << "  Track inst=" << Harmonic::instrumentToString(inst)
                  << " name='" << trk.trackName << "' activeSteps=" << activeCount << "/" << trk.steps.size() << std::endl;
    }

    // Build the identical harmonic frame detected
    Harmonic::HarmonicFrame frame;
    frame.rootPitchClass = tonal.detectedRootPitchClass;
    frame.bassMidiNote = tonal.referenceBassMidiNote;
    frame.quality = tonal.detectedChordQuality;
    frame.chordName = tonal.detectedChordName;
    frame.chordTones = tonal.detectedChordTones;
    frame.activeMode = tonal.detectedMode;

    for (int oct = 1; oct <= 8; ++oct) {
        for (int interval : tonal.detectedChordTones) {
            frame.pitches.push_back(oct * 12 + ((tonal.detectedRootPitchClass + interval) % 12));
        }
    }
    std::sort(frame.pitches.begin(), frame.pitches.end());
    frame.pitches.erase(std::unique(frame.pitches.begin(), frame.pitches.end()), frame.pitches.end());

    Orchestration::VoicingEngine ve;
    auto voicing = ve.generateVoicing(frame);

    // Export regenerated MIDI using StandardMidiWriter
    MidiExport::StandardMidiWriter writer;
    std::string regenMidiPath = "/tmp/testConvert_regenerated.mid";
    bool exportOk = writer.exportMidiFile(pattern, voicing, origMidi.bpm, barCount, regenMidiPath);
    ASSERT_TRUE(exportOk);

    // Parse regenerated MIDI back
    Converter::ParsedMidiFile regenMidi;
    bool parseRegenOk = converter.parseMidiFile(regenMidiPath, regenMidi, err);
    ASSERT_TRUE(parseRegenOk);

    // Compare original notes vs regenerated notes
    // Build maps of (step, pitch) per instrument
    struct NoteKey {
        int step;
        int pitch;
        bool operator<(const NoteKey& o) const {
            if (step != o.step) return step < o.step;
            return pitch < o.pitch;
        }
    };

    int totalOrigNotes = 0;
    int matchedNotes = 0;

    for (const auto& origTrk : origMidi.tracks) {
        if (!origTrk.isEnabled || origTrk.notes.empty()) continue;

        Harmonic::InstrumentId inst = origTrk.suggestedInstrument;
        // Find corresponding regenerated track
        const Converter::ParsedMidiTrack* regenTrk = nullptr;
        for (const auto& rt : regenMidi.tracks) {
            if (rt.suggestedInstrument == inst) {
                regenTrk = &rt;
                break;
            }
        }

        std::cout << "\nComparing Track '" << origTrk.trackName << "' (" << Harmonic::instrumentToString(inst) << "):" << std::endl;
        std::map<NoteKey, int> origCounts;
        for (const auto& n : origTrk.notes) {
            int step = static_cast<int>(std::round(static_cast<double>(n.startTick) / ticksPer16th));
            origCounts[{step, n.pitch}]++;
            totalOrigNotes++;
        }

        std::map<NoteKey, int> regenCounts;
        if (regenTrk) {
            int rTicksPer16th = regenMidi.ticksPerQuarter / 4;
            for (const auto& n : regenTrk->notes) {
                int step = static_cast<int>(std::round(static_cast<double>(n.startTick) / rTicksPer16th));
                regenCounts[{step, n.pitch}]++;
                std::cout << "  Regen has step=" << step << " pitch=" << n.pitch << " (" << Harmonic::noteNumberToName(n.pitch) << ") dur=" << n.durationTicks << std::endl;
            }
        }

        int trackMatches = 0;
        int trackOrig = 0;
        for (const auto& [key, count] : origCounts) {
            trackOrig += count;
            auto it = regenCounts.find(key);
            if (it != regenCounts.end()) {
                int common = std::min(count, it->second);
                trackMatches += common;
                matchedNotes += common;
            } else {
                std::cout << "  MISMATCH: Orig has step=" << key.step << " pitch=" << key.pitch << " (" << Harmonic::noteNumberToName(key.pitch) << ") count=" << count << std::endl;
            }
        }
        float trkPct = (trackOrig > 0) ? (100.0f * trackMatches / trackOrig) : 100.0f;
        std::cout << "  Track Match: " << trackMatches << "/" << trackOrig << " (" << trkPct << "%)" << std::endl;
    }

    float overallSimilarity = (totalOrigNotes > 0) ? (100.0f * matchedNotes / totalOrigNotes) : 0.0f;
    std::cout << "\n>>> OVERALL SIMILARITY: " << matchedNotes << "/" << totalOrigNotes << " (" << overallSimilarity << "%) <<<\n" << std::endl;
    // User requested similarity >= 80%
    ASSERT_TRUE(overallSimilarity >= 80.0f);
}

TEST_CASE(MidiPresetConverter, MultiBarNoteDurationAndPresetRoundtrip) {
    // 1. Create a 4-bar pattern (64 steps) with multi-length notes (up to 32 steps)
    Sequencer::OrchestralPattern pat;
    pat.name = "Epic 4-Bar Fantasy Theme";
    pat.bpm = 128.0;
    pat.barLength = 4;

    Sequencer::TrackPattern trk;
    trk.instrument = Harmonic::InstrumentId::Violins1;
    trk.trackName = "1st Violins";
    trk.section = Harmonic::OrchestralSection::Strings;
    trk.stepCount = 64;
    trk.steps.resize(64);

    // Step 0: 4 steps long note
    trk.steps[0].active = true;
    trk.steps[0].stepOffset = 0;
    trk.steps[0].lengthSteps = 4;
    trk.steps[0].action = Harmonic::StepActionType::Sustain;

    // Step 16 (Bar 2): 16 steps long note (Full Bar Sustain)
    trk.steps[16].active = true;
    trk.steps[16].stepOffset = 2;
    trk.steps[16].lengthSteps = 16;
    trk.steps[16].action = Harmonic::StepActionType::Sustain;

    // Step 32 (Bar 3 & 4): 32 steps long note (2 Full Bars Sustain)
    trk.steps[32].active = true;
    trk.steps[32].stepOffset = 4;
    trk.steps[32].lengthSteps = 32;
    trk.steps[32].action = Harmonic::StepActionType::Sustain;

    pat.tracks[trk.instrument] = trk;

    // 2. Test JSON Serialization
    std::string jsonStr = pat.toJson();
    ASSERT_TRUE(!jsonStr.empty());
    ASSERT_TRUE(jsonStr.find("\"barLength\": 4") != std::string::npos);

    // 3. Test Loading Preset from JSON
    Sequencer::OrchestralPattern loaded = Sequencer::OrchestralPattern::fromJson(jsonStr);
    ASSERT_EQ(loaded.name, "Epic 4-Bar Fantasy Theme");
    ASSERT_EQ(loaded.barLength, 4);
    ASSERT_EQ(loaded.tracks.size(), 1);

    const auto& loadedTrk = loaded.tracks.at(Harmonic::InstrumentId::Violins1);
    ASSERT_EQ(loadedTrk.steps.size(), 64);

    // Verify duration values were not truncated to 16
    ASSERT_TRUE(loadedTrk.steps[0].active);
    ASSERT_EQ(loadedTrk.steps[0].lengthSteps, 4);

    ASSERT_TRUE(loadedTrk.steps[16].active);
    ASSERT_EQ(loadedTrk.steps[16].lengthSteps, 16);

    ASSERT_TRUE(loadedTrk.steps[32].active);
    ASSERT_EQ(loadedTrk.steps[32].lengthSteps, 32); // Crucial: > 16 steps duration preserved!
}



