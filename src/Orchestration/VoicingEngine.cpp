#include "VoicingEngine.h"
#include <algorithm>
#include <cmath>

namespace Orchestration {

void VoicingEngine::resetHistory() {
    previousPitches.clear();
}

int VoicingEngine::findBestPitchForInstrument(Harmonic::InstrumentId id,
                                             const std::vector<int>& allowedPitchClasses,
                                             int preferredPitchClass,
                                             int targetRegisterMin,
                                             int targetRegisterMax) {
    auto tess = getTessitura(id);
    int effMin = std::max(tess.minPitch, targetRegisterMin);
    int effMax = std::min(tess.maxPitch, targetRegisterMax);
    if (effMin > effMax) {
        effMin = tess.minPitch;
        effMax = tess.maxPitch;
    }

    std::vector<int> candidatePitches;
    // Generate all valid pitches in the tessitura for the allowed pitch classes
    for (int p = effMin; p <= effMax; ++p) {
        int pc = p % 12;
        if (std::find(allowedPitchClasses.begin(), allowedPitchClasses.end(), pc) != allowedPitchClasses.end()) {
            candidatePitches.push_back(p);
        }
    }

    if (candidatePitches.empty()) {
        // Fallback: search whole tessitura
        for (int p = tess.minPitch; p <= tess.maxPitch; ++p) {
            int pc = p % 12;
            if (std::find(allowedPitchClasses.begin(), allowedPitchClasses.end(), pc) != allowedPitchClasses.end()) {
                candidatePitches.push_back(p);
            }
        }
    }

    if (candidatePitches.empty()) {
        return (tess.minPitch + tess.maxPitch) / 2;
    }

    // Heuristic selection:
    // If we have history for this instrument, prioritize minimal step distance (voice leading!)
    bool hasHistory = (previousPitches.find(id) != previousPitches.end());
    int prev = hasHistory ? previousPitches[id] : (effMin + effMax) / 2;

    int bestPitch = candidatePitches.front();
    int bestCost = 99999;

    for (int cp : candidatePitches) {
        int cost = 0;
        int pc = cp % 12;

        // Voice leading distance
        if (hasHistory) {
            cost += std::abs(cp - prev) * 10;
        } else {
            // Distance from sweet center
            int sweetCenter = (tess.sweetMin + tess.sweetMax) / 2;
            cost += std::abs(cp - sweetCenter) * 4;
        }

        // Preference bonus for preferred pitch class
        if (preferredPitchClass >= 0) {
            if (pc == preferredPitchClass) {
                cost -= 15;
            }
        }

        if (cost < bestCost) {
            bestCost = cost;
            bestPitch = cp;
        }
    }

    previousPitches[id] = bestPitch;
    return bestPitch;
}

Harmonic::OrchestralVoicing VoicingEngine::generateVoicing(const Harmonic::HarmonicFrame& frame,
                                                           VoicingStyle style) {
    Harmonic::OrchestralVoicing result;
    result.sourceHarmonic = frame;

    // Collect pitch classes
    std::vector<int> chordPcs;
    if (!frame.chordTones.empty()) {
        for (int interval : frame.chordTones) {
            chordPcs.push_back((frame.rootPitchClass + interval) % 12);
        }
    } else if (!frame.pitches.empty()) {
        for (int p : frame.pitches) chordPcs.push_back((p % 12 + 12) % 12);
    } else {
        chordPcs = {frame.rootPitchClass, (frame.rootPitchClass + 4) % 12, (frame.rootPitchClass + 7) % 12};
    }
    std::sort(chordPcs.begin(), chordPcs.end());
    chordPcs.erase(std::unique(chordPcs.begin(), chordPcs.end()), chordPcs.end());

    int rootPc = frame.rootPitchClass;
    int bassPc = (frame.bassMidiNote % 12);

    // Identify chord tones: root, 3rd, 5th, 7th, extensions if available
    int thirdPc = -1;
    int fifthPc = -1;
    int seventhPc = -1;

    for (int pc : chordPcs) {
        int interval = (pc - rootPc + 12) % 12;
        if (interval == 3 || interval == 4) thirdPc = pc;
        else if (interval == 6 || interval == 7 || interval == 8) fifthPc = pc;
        else if (interval == 9 || interval == 10 || interval == 11) seventhPc = pc;
    }

    if (fifthPc < 0) fifthPc = (rootPc + 7) % 12;

    auto assign = [&](Harmonic::InstrumentId id, int prefPc, int regMin, int regMax,
                      Harmonic::ArticulationType art = Harmonic::ArticulationType::Sustain) {
        int pitch = findBestPitchForInstrument(id, chordPcs, prefPc, regMin, regMax);
        Harmonic::VoiceAssignment va;
        va.instrument = id;
        va.midiPitch = pitch;
        va.velocity = 85;
        va.articulation = art;
        va.midiChannel = getInstrumentDefaultMidiChannel(id);
        result.voices.push_back(va);
    };

    // 1. STRINGS SECTION
    // Double Basses: play bass note in deep octave
    assign(Harmonic::InstrumentId::DoubleBasses, bassPc, 24, 43);
    // Cellos: play bass or root an octave higher, or 5th
    assign(Harmonic::InstrumentId::Cellos, (frame.isSlashChord ? rootPc : bassPc), 36, 55);
    // Violas: warm middle harmony (3rd or 5th)
    assign(Harmonic::InstrumentId::Violas, (thirdPc >= 0 ? thirdPc : fifthPc), 48, 67);
    // Violins 2: harmony below Violins 1
    assign(Harmonic::InstrumentId::Violins2, (seventhPc >= 0 ? seventhPc : fifthPc), 55, 76);
    // Violins 1: melody / top voice
    int topPref = (seventhPc >= 0 ? seventhPc : (thirdPc >= 0 ? thirdPc : rootPc));
    assign(Harmonic::InstrumentId::Violins1, topPref, 64, 88);

    // 2. BRASS SECTION
    // Tuba: sub-bass foundation
    assign(Harmonic::InstrumentId::Tuba, bassPc, 28, 48);
    // Trombones: low-mid harmony
    assign(Harmonic::InstrumentId::Trombones, fifthPc, 40, 60);
    // French Horns: rich warm center (3rd or 7th)
    assign(Harmonic::InstrumentId::FrenchHorns, (thirdPc >= 0 ? thirdPc : rootPc), 48, 69);
    // Trumpets: bright fanfare / melody
    assign(Harmonic::InstrumentId::Trumpets, topPref, 58, 80);

    // 3. WOODWINDS SECTION
    // Bassoons: agile bottom
    assign(Harmonic::InstrumentId::Bassoons, bassPc, 36, 55);
    // Clarinets: middle blend
    assign(Harmonic::InstrumentId::Clarinets, (thirdPc >= 0 ? thirdPc : fifthPc), 50, 72);
    // Oboes: expressive mid-high
    assign(Harmonic::InstrumentId::Oboes, (seventhPc >= 0 ? seventhPc : thirdPc), 58, 79);
    // Flutes: ethereal top
    assign(Harmonic::InstrumentId::Flutes, topPref, 65, 88);

    // 4. PERCUSSION SECTION
    // Timpani: tuned to bass / root or fifth
    assign(Harmonic::InstrumentId::Timpani, bassPc, 36, 53);
    assign(Harmonic::InstrumentId::OrchestralPerc, 60, 48, 72, Harmonic::ArticulationType::Staccato);
    assign(Harmonic::InstrumentId::Celesta, topPref, 60, 96, Harmonic::ArticulationType::Staccato);

    // 5. HARP & KEYBOARDS
    assign(Harmonic::InstrumentId::Harp, (thirdPc >= 0 ? thirdPc : rootPc), 48, 84);
    assign(Harmonic::InstrumentId::Piano, rootPc, 36, 72);
    assign(Harmonic::InstrumentId::ChurchOrgan, bassPc, 36, 72);

    // 6. GUITARS & BASS
    assign(Harmonic::InstrumentId::AcousticGuitar, (thirdPc >= 0 ? thirdPc : rootPc), 40, 72);
    assign(Harmonic::InstrumentId::ElectricGuitar, rootPc, 40, 76);
    assign(Harmonic::InstrumentId::BassGuitar, bassPc, 28, 52);

    // 7. CHOIR
    assign(Harmonic::InstrumentId::ChoirFull, (fifthPc >= 0 ? fifthPc : rootPc), 48, 76);

    // 8. SYNTHS
    assign(Harmonic::InstrumentId::SynthesizerLead, topPref, 60, 88);
    assign(Harmonic::InstrumentId::SynthesizerPad, (thirdPc >= 0 ? thirdPc : fifthPc), 48, 72);

    return result;
}

} // namespace Orchestration
