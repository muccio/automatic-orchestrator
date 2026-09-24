#include "Converter/MidiPresetConverter.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "========================================================\n";
        std::cout << " Automatic Orchestrator - MIDI to Preset CLI Converter  \n";
        std::cout << "========================================================\n\n";
        std::cout << "Usage: midi_preset_cli <input.mid> [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --name <preset_name>  Custom name for the generated preset\n";
        std::cout << "  --root <pitch_name>   Override root note (e.g. C, D, Eb, F#)\n";
        std::cout << "  --mode <scale_mode>   Override scale mode (Major, Minor, Dorian, etc.)\n";
        std::cout << "  --steps <16|32>       Pattern length in 16th steps (default: 16)\n";
        std::cout << "  --tempo <bpm>         Override tempo in BPM\n";
        std::cout << "  --out <output.json>   Output file path (default: Application Support Presets)\n\n";
        return 1;
    }

    std::string midiPath = argv[1];
    Converter::ConversionOptions options;
    std::string outputPath;

    // Parse options
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--name" && i + 1 < argc) {
            options.presetName = argv[++i];
        } else if (arg == "--root" && i + 1 < argc) {
            std::string rootStr = argv[++i];
            const char* pNames[12] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
            for (int r = 0; r < 12; ++r) {
                if (rootStr == pNames[r]) {
                    options.overrideRootPitchClass = r;
                    break;
                }
            }
        } else if (arg == "--mode" && i + 1 < argc) {
            std::string modeStr = argv[++i];
            if (modeStr == "Minor" || modeStr == "Aeolian") options.overrideMode = Harmonic::ScaleMode::Aeolian;
            else if (modeStr == "Dorian") options.overrideMode = Harmonic::ScaleMode::Dorian;
            else if (modeStr == "Phrygian") options.overrideMode = Harmonic::ScaleMode::Phrygian;
            else if (modeStr == "Lydian") options.overrideMode = Harmonic::ScaleMode::Lydian;
            else if (modeStr == "Mixolydian") options.overrideMode = Harmonic::ScaleMode::Mixolydian;
            else options.overrideMode = Harmonic::ScaleMode::Ionian;
            options.useOverrideMode = true;
        } else if (arg == "--steps" && i + 1 < argc) {
            options.lengthSteps = std::atoi(argv[++i]);
        } else if (arg == "--tempo" && i + 1 < argc) {
            options.tempoBpm = std::atof(argv[++i]);
        } else if (arg == "--out" && i + 1 < argc) {
            outputPath = argv[++i];
        }
    }

    Converter::MidiPresetConverter converter;
    Converter::ParsedMidiFile midiFile;
    std::string err;

    std::cout << "Parsing MIDI file: " << midiPath << " ...\n";
    if (!converter.parseMidiFile(midiPath, midiFile, err)) {
        std::cerr << "Error: " << err << "\n";
        return 1;
    }

    std::cout << "Parsed " << midiFile.tracks.size() << " track(s), Tempo: "
              << static_cast<int>(midiFile.bpm) << " BPM, Time Sig: "
              << midiFile.timeSigNum << "/" << midiFile.timeSigDen << "\n\n";

    // Harmonic / Tonal Analysis
    auto tonal = converter.analyzeTonalCenter(midiFile);
    std::cout << "--- Tonal & Harmonic Analysis ---\n";
    std::cout << "Detected Tonality: " << tonal.detectedChordName << "\n";
    std::cout << "Detected Root:     " << tonal.detectedRootName << " (Pitch Class " << tonal.detectedRootPitchClass << ")\n";
    std::cout << "Confidence:        " << static_cast<int>(tonal.confidence * 100.0f) << "%\n\n";

    // Track Mappings
    std::cout << "--- Track & Instrument Mappings ---\n";
    for (const auto& trk : midiFile.tracks) {
        std::cout << "Track " << trk.trackIndex + 1 << ": \"" << trk.trackName << "\" -> "
                  << Harmonic::instrumentToString(trk.suggestedInstrument) << " ("
                  << Harmonic::articulationToString(trk.suggestedArticulation) << ", "
                  << trk.noteCount << " notes, Mode: " << trk.suggestedArrangerMode << ")\n";
    }
    std::cout << "\n";

    // Default output path if not specified
    if (outputPath.empty()) {
        const char* home = std::getenv("HOME");
        std::string presetDir = (home != nullptr)
            ? std::string(home) + "/Library/Application Support/Automatic Orchestrator/Presets"
            : "./Presets";
        std::string pName = options.presetName.empty() ? midiFile.fileName : options.presetName;
        // Strip .mid extension from name if present
        if (pName.size() > 4 && pName.substr(pName.size() - 4) == ".mid") {
            pName = pName.substr(0, pName.size() - 4);
        }
        outputPath = presetDir + "/" + pName + ".json";
    }

    auto pattern = converter.convertToPattern(midiFile, tonal, options);
    if (!pattern.saveToFile(outputPath)) {
        std::cerr << "Error writing preset file to " << outputPath << "\n";
        return 1;
    }

    std::cout << "SUCCESS: Preset saved to:\n  " << outputPath << "\n";
    std::cout << "It is now available in Automatic Orchestrator!\n";

    return 0;
}
