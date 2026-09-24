#include "ScaleQuantizer.h"
#include <algorithm>
#include <cmath>

namespace Harmonic {

bool ScaleQuantizer::isPitchInScale(int midiPitch, int rootPitchClass, ScaleMode mode) const {
    int interval = ((midiPitch % 12) - (rootPitchClass % 12) + 12) % 12;
    auto scaleIntervals = getScaleModeIntervals(mode);
    return std::find(scaleIntervals.begin(), scaleIntervals.end(), interval) != scaleIntervals.end();
}

int ScaleQuantizer::quantizePitch(int midiPitch, int rootPitchClass, ScaleMode mode) const {
    if (isPitchInScale(midiPitch, rootPitchClass, mode)) {
        return midiPitch;
    }

    int pc = (midiPitch % 12);
    int octaveBase = midiPitch - pc;
    int interval = ((pc - rootPitchClass) + 12) % 12;

    auto scaleIntervals = getScaleModeIntervals(mode);

    int bestDiff = 999;
    int bestInterval = interval;

    for (int si : scaleIntervals) {
        int diff = std::abs(si - interval);
        // Circular distance on 12-tone circle
        if (diff > 6) diff = 12 - diff;

        if (diff < bestDiff) {
            bestDiff = diff;
            bestInterval = si;
        }
    }

    int quantizedPc = (rootPitchClass + bestInterval) % 12;
    int resultPitch = octaveBase + quantizedPc;

    // Keep closest octave to original pitch
    if (resultPitch - midiPitch > 6) resultPitch -= 12;
    if (midiPitch - resultPitch > 6) resultPitch += 12;

    return std::clamp(resultPitch, 0, 127);
}

HarmonicFrame ScaleQuantizer::transformToMode(const HarmonicFrame& input, ScaleMode targetMode) const {
    HarmonicFrame out = input;
    out.activeMode = targetMode;

    std::vector<int> newPitches;
    std::vector<int> newChordTones;

    for (int p : input.pitches) {
        int qp = quantizePitch(p, input.rootPitchClass, targetMode);
        newPitches.push_back(qp);
        int interval = ((qp % 12) - input.rootPitchClass + 12) % 12;
        newChordTones.push_back(interval);
    }

    std::sort(newPitches.begin(), newPitches.end());
    newPitches.erase(std::unique(newPitches.begin(), newPitches.end()), newPitches.end());

    std::sort(newChordTones.begin(), newChordTones.end());
    newChordTones.erase(std::unique(newChordTones.begin(), newChordTones.end()), newChordTones.end());

    out.pitches = newPitches;
    out.chordTones = newChordTones;
    if (!newPitches.empty()) {
        out.bassMidiNote = newPitches.front();
    }

    // Infer updated quality if 3 notes
    if (newChordTones.size() == 3) {
        if (newChordTones == std::vector<int>{0, 4, 7}) out.quality = ChordQuality::MajorTriad;
        else if (newChordTones == std::vector<int>{0, 3, 7}) out.quality = ChordQuality::MinorTriad;
        else if (newChordTones == std::vector<int>{0, 3, 6}) out.quality = ChordQuality::DiminishedTriad;
        else if (newChordTones == std::vector<int>{0, 4, 8}) out.quality = ChordQuality::AugmentedTriad;
    }

    std::string rootName = pitchClassToName(out.rootPitchClass, true);
    out.chordName = rootName + " " + chordQualityToString(out.quality);

    return out;
}

} // namespace Harmonic
