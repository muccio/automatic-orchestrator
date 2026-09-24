#pragma once

#include "Common/HarmonicTypes.h"
#include <random>

namespace Orchestration {

class HarmonicRandomizer {
public:
    HarmonicRandomizer();

    void setSeed(unsigned int seed) { rng.seed(seed); }

    void setInversionJitterChance(double chance) { inversionJitterChance = chance; }
    void setTensionInjectionChance(double chance) { tensionInjectionChance = chance; }
    void setVelocityHumanizeAmount(int amount) { velocityHumanizeAmount = amount; }
    void setTimingHumanizeMs(double ms) { timingHumanizeMs = ms; }

    int applyVelocityHumanize(int baseVelocity);
    double applyTimingHumanize(); // returns seconds (+/- delta)

    Harmonic::OrchestralVoicing processVoicing(const Harmonic::OrchestralVoicing& inputVoicing,
                                               const Harmonic::HarmonicFrame& frame);

private:
    std::mt19937 rng;
    double inversionJitterChance = 0.0;
    double tensionInjectionChance = 0.0;
    int velocityHumanizeAmount = 0;
    double timingHumanizeMs = 0.0;
};

} // namespace Orchestration
