#pragma once

#include "Common/HarmonicTypes.h"
#include <vector>
#include <set>

namespace Harmonic {

class ChordDetector {
public:
    ChordDetector();

    // Analyzes an arbitrary set of MIDI pitches and produces a HarmonicFrame
    HarmonicFrame detectChord(const std::vector<int>& midiPitches);

private:
    struct ChordTemplate {
        ChordQuality quality;
        std::vector<int> intervals; // e.g. {0, 4, 7} for major triad
        std::string suffix;         // e.g. "m", "7", "Maj7"
        int priority;               // Higher priority breaks ties
    };

    std::vector<ChordTemplate> templates;
    void initTemplates();
};

} // namespace Harmonic
