#pragma once

#include "Common/HarmonicTypes.h"
#include <vector>

namespace Harmonic {

class ScaleQuantizer {
public:
    ScaleQuantizer() = default;

    // Quantizes an individual MIDI pitch to the nearest scale tone
    int quantizePitch(int midiPitch, int rootPitchClass, ScaleMode mode) const;

    // Transforms an entire HarmonicFrame into a target ScaleMode
    HarmonicFrame transformToMode(const HarmonicFrame& input, ScaleMode targetMode) const;

    // Checks if a given pitch belongs to the scale
    bool isPitchInScale(int midiPitch, int rootPitchClass, ScaleMode mode) const;
};

} // namespace Harmonic
