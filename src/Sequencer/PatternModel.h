#pragma once

#include "Common/HarmonicTypes.h"
#include <vector>
#include <map>
#include <string>

namespace Sequencer {

struct StepDefinition {
    bool active = true;
    int stepOffset = 0; // Relative pitch step: e.g. -8 to +9 (0 = Lowest / Root chord tone)
    int lengthSteps = 1; // Duration in 16th steps (1 to 16)
    int velocity = 100; // 1 to 127
    double gate = 0.85;  // duration fraction of total length
    int octaveOffset = 0; // -2 to +2 octaves
    Harmonic::StepActionType action = Harmonic::StepActionType::Ostinato;
    Harmonic::ArticulationType articulation = Harmonic::ArticulationType::Spiccato;
};

struct TrackPattern {
    Harmonic::InstrumentId instrument = Harmonic::InstrumentId::Violins1;
    std::string trackName;
    Harmonic::OrchestralSection section = Harmonic::OrchestralSection::Strings;
    int midiChannel = 1;             // 1 to 16
    Harmonic::ArticulationType articulation = Harmonic::ArticulationType::Spiccato;
    std::string arrangerMode = "Top"; // "Top", "Lowest", "Chord", "Root", "Arp Up", "Arp Down"
    int octaveOffset = 0;            // -2, -1, 0, +1, +2
    float volume = 0.8f;             // 0.0 to 1.0 (gain)
    float pan = 0.0f;                // -1.0 (Left) to +1.0 (Right), 0.0 = Center
    bool isMuted = false;
    bool isSolo = false;
    int stepCount = 16;
    double stepDivision = 0.25;      // 1/16th note
    std::vector<StepDefinition> steps;
    std::vector<int> cc1Curve;       // 16 or 32 values (0 to 127) for dynamics
};

struct OrchestralPattern {
    std::string name = "Action Ostinato";
    std::string styleCategory = "Action";
    double bpm = 120.0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;
    int barLength = 2; // 1, 2, or 4 bars
    std::map<Harmonic::InstrumentId, TrackPattern> tracks;

    // Serialization & Persistence
    std::string toJson() const;
    static OrchestralPattern fromJson(const std::string& jsonStr);
    bool saveToFile(const std::string& filePath) const;
    static OrchestralPattern loadFromFile(const std::string& filePath);
};

// Factory functions for built-in orchestral styles
OrchestralPattern createActionOstinatoPattern();
OrchestralPattern createEpicFanfarePattern();
OrchestralPattern createLyricalAdagioPattern();
OrchestralPattern createSuspenseMysteryPattern();
OrchestralPattern createWarDrumsPattern();
OrchestralPattern createFantasyAdventurePattern();

} // namespace Sequencer
