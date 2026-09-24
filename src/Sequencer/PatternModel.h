#pragma once

#include "Common/HarmonicTypes.h"
#include <vector>
#include <map>
#include <string>

namespace Sequencer {

struct StepDefinition {
    Harmonic::StepActionType action = Harmonic::StepActionType::Ostinato;
    int velocity = 90;
    double gate = 0.5; // 50% step duration
    int octaveOffset = 0; // -2 to +2 octaves
    Harmonic::ArticulationType articulation = Harmonic::ArticulationType::Spiccato;
};

struct TrackPattern {
    Harmonic::InstrumentId instrument;
    int stepCount = 16;
    double stepDivision = 0.25; // 1/16th note (1 quarter note = 1.0)
    std::vector<StepDefinition> steps;
};

struct OrchestralPattern {
    std::string name;
    std::string styleCategory;
    std::map<Harmonic::InstrumentId, TrackPattern> tracks;
};

// Factory functions for built-in orchestral styles
OrchestralPattern createActionOstinatoPattern();
OrchestralPattern createEpicFanfarePattern();
OrchestralPattern createLyricalAdagioPattern();
OrchestralPattern createSuspenseMysteryPattern();

} // namespace Sequencer
