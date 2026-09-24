#include "HarmonicRandomizer.h"
#include "OrchestralTessituras.h"
#include <algorithm>

namespace Orchestration {

HarmonicRandomizer::HarmonicRandomizer() : rng(1337) {}

int HarmonicRandomizer::applyVelocityHumanize(int baseVelocity) {
    if (velocityHumanizeAmount <= 0) return baseVelocity;
    std::uniform_int_distribution<int> dist(-velocityHumanizeAmount, velocityHumanizeAmount);
    return std::clamp(baseVelocity + dist(rng), 1, 127);
}

double HarmonicRandomizer::applyTimingHumanize() {
    if (timingHumanizeMs <= 0.0) return 0.0;
    std::uniform_real_distribution<double> dist(-timingHumanizeMs, timingHumanizeMs);
    return dist(rng) / 1000.0; // convert to seconds
}

Harmonic::OrchestralVoicing HarmonicRandomizer::processVoicing(const Harmonic::OrchestralVoicing& inputVoicing,
                                                             const Harmonic::HarmonicFrame& frame) {
    Harmonic::OrchestralVoicing result = inputVoicing;
    std::uniform_real_distribution<double> coin(0.0, 1.0);

    // Get allowed modal scale degrees from active mode
    auto scaleIntervals = Harmonic::getScaleModeIntervals(frame.activeMode);

    // Identify tension candidates: e.g. interval 2 (9th), interval 9 (6th/13th), interval 11 (Maj7), or 6 (#11 in Lydian)
    std::vector<int> tensionIntervals;
    for (int si : scaleIntervals) {
        if (si == 2 || si == 9 || si == 11 || (si == 6 && frame.activeMode == Harmonic::ScaleMode::Lydian)) {
            tensionIntervals.push_back(si);
        }
    }

    for (auto& v : result.voices) {
        // Apply velocity humanization to all voices
        v.velocity = applyVelocityHumanize(v.velocity);

        // Never modify Basses or Tuba with tension injections (keep the harmonic floor rock solid)
        if (v.instrument == Harmonic::InstrumentId::DoubleBasses ||
            v.instrument == Harmonic::InstrumentId::Tuba ||
            v.instrument == Harmonic::InstrumentId::Timpani) {
            continue;
        }

        // Tension Injection on upper voices (Flutes, Violins, Horns, Clarinets)
        if (!tensionIntervals.empty() && coin(rng) < tensionInjectionChance) {
            std::uniform_int_distribution<size_t> tDist(0, tensionIntervals.size() - 1);
            int tensionInterval = tensionIntervals[tDist(rng)];
            int targetPc = (frame.rootPitchClass + tensionInterval) % 12;

            // Find closest octave in instrument tessitura
            auto tess = getInstrumentTessitura(v.instrument);
            int curOctaveBase = v.midiPitch - (v.midiPitch % 12);
            int newPitch = curOctaveBase + targetPc;
            if (newPitch < tess.minPitch) newPitch += 12;
            if (newPitch > tess.maxPitch) newPitch -= 12;

            if (newPitch >= tess.minPitch && newPitch <= tess.maxPitch) {
                v.midiPitch = newPitch;
            }
        }

        // Inversion Jitter: occasional octave shift if within sweet spot
        if (coin(rng) < inversionJitterChance) {
            auto tess = getInstrumentTessitura(v.instrument);
            if (coin(rng) > 0.5) {
                if (v.midiPitch + 12 <= tess.maxPitch) v.midiPitch += 12;
            } else {
                if (v.midiPitch - 12 >= tess.minPitch) v.midiPitch -= 12;
            }
        }
    }

    return result;
}

} // namespace Orchestration
