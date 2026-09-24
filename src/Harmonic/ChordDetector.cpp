#include "ChordDetector.h"
#include <algorithm>
#include <set>

namespace Harmonic {

ChordDetector::ChordDetector() {
    initTemplates();
}

void ChordDetector::initTemplates() {
    templates = {
        // Priority 100+: Extensions (most specific)
        { ChordQuality::Major9,          {0, 4, 7, 11, 2}, "Maj9", 110 },
        { ChordQuality::Dominant9,       {0, 4, 7, 10, 2}, "9",    110 },
        { ChordQuality::Minor9,          {0, 3, 7, 10, 2}, "m9",   110 },
        { ChordQuality::Add9,            {0, 4, 7, 2},     "add9", 100 },
        { ChordQuality::Sharp11,         {0, 4, 7, 11, 6}, "Maj7(#11)", 105 },

        // Priority 80-90: 7ths
        { ChordQuality::Major7,          {0, 4, 7, 11},    "Maj7",  90 },
        { ChordQuality::Dominant7,       {0, 4, 7, 10},    "7",     90 },
        { ChordQuality::Minor7,          {0, 3, 7, 10},    "m7",    90 },
        { ChordQuality::HalfDiminished7, {0, 3, 6, 10},    "m7b5",  95 },
        { ChordQuality::Diminished7,     {0, 3, 6, 9},     "dim7",  95 },
        { ChordQuality::MinorMajor7,     {0, 3, 7, 11},    "mMaj7", 85 },
        { ChordQuality::SevenSus4,       {0, 5, 7, 10},    "7sus4", 85 },
        { ChordQuality::Augmented7,      {0, 4, 8, 10},    "aug7",  85 },
        { ChordQuality::Major6,          {0, 4, 7, 9},     "6",     80 },
        { ChordQuality::Minor6,          {0, 3, 7, 9},     "m6",    80 },

        // Priority 60-70: Triads
        { ChordQuality::MajorTriad,      {0, 4, 7},        "",      70 },
        { ChordQuality::MinorTriad,      {0, 3, 7},        "m",     70 },
        { ChordQuality::DiminishedTriad, {0, 3, 6},        "dim",   75 },
        { ChordQuality::AugmentedTriad,  {0, 4, 8},        "aug",   75 },
        { ChordQuality::Sus4,            {0, 5, 7},        "sus4",  65 },
        { ChordQuality::Sus2,            {0, 2, 7},        "sus2",  65 },

        // Priority 40: Power chord (dyad)
        { ChordQuality::PowerChord,      {0, 7},           "5",     40 }
    };
}

HarmonicFrame ChordDetector::detectChord(const std::vector<int>& midiPitches) {
    HarmonicFrame frame;
    if (midiPitches.empty()) {
        frame.quality = ChordQuality::Unknown;
        frame.chordName = "None";
        return frame;
    }

    std::vector<int> sortedPitches = midiPitches;
    std::sort(sortedPitches.begin(), sortedPitches.end());
    sortedPitches.erase(std::unique(sortedPitches.begin(), sortedPitches.end()), sortedPitches.end());

    frame.pitches = sortedPitches;
    frame.bassMidiNote = sortedPitches.front();
    int bassPc = frame.bassMidiNote % 12;

    if (sortedPitches.size() == 1) {
        frame.isSingleNote = true;
        frame.quality = ChordQuality::SingleNote;
        frame.rootPitchClass = bassPc;
        frame.chordName = noteNumberToName(frame.bassMidiNote, true);
        frame.chordTones = {0};
        return frame;
    }

    // Extract unique pitch classes
    std::set<int> pitchClasses;
    for (int p : sortedPitches) {
        pitchClasses.insert(p % 12);
    }

    // 1. First test: Is there an exact root-position chord in the FULL set where root == bassPc?
    // We sort templates by number of notes (most specific first: 9ths -> 7ths -> Triads)
    for (const auto& tpl : templates) {
        if (tpl.intervals.size() == pitchClasses.size()) {
            std::set<int> intervalsFromBass;
            for (int pc : pitchClasses) {
                intervalsFromBass.insert((pc - bassPc + 12) % 12);
            }

            bool exactMatch = true;
            for (int ti : tpl.intervals) {
                if (!intervalsFromBass.count(ti)) {
                    exactMatch = false;
                    break;
                }
            }

            if (exactMatch) {
                frame.rootPitchClass = bassPc;
                frame.quality = tpl.quality;
                frame.chordTones = tpl.intervals;
                frame.isSlashChord = false;
                std::string rootName = pitchClassToName(bassPc, true);
                frame.chordName = rootName + tpl.suffix;
                return frame;
            }
        }
    }

    // 2. Second test: Upper-structure slash chord (e.g. Bb/C, F/G)
    // Check if notes above the bass form an exact triad or 7th chord
    std::set<int> upperPitchClasses;
    for (int p : sortedPitches) {
        int pc = p % 12;
        if (p > frame.bassMidiNote && pc != bassPc) {
            upperPitchClasses.insert(pc);
        }
    }

    if (upperPitchClasses.size() >= 3) {
        for (int upperRoot : upperPitchClasses) {
            std::set<int> upperIntervals;
            for (int pc : upperPitchClasses) {
                upperIntervals.insert((pc - upperRoot + 12) % 12);
            }

            for (const auto& tpl : templates) {
                if (tpl.intervals.size() == upperPitchClasses.size()) {
                    bool exactMatch = true;
                    for (int ti : tpl.intervals) {
                        if (!upperIntervals.count(ti)) {
                            exactMatch = false;
                            break;
                        }
                    }

                    if (exactMatch) {
                        frame.rootPitchClass = upperRoot;
                        frame.quality = tpl.quality;
                        frame.chordTones = tpl.intervals;
                        frame.isSlashChord = true;
                        std::string upperName = pitchClassToName(upperRoot, true) + tpl.suffix;
                        std::string bassName = pitchClassToName(bassPc, true);
                        frame.chordName = upperName + "/" + bassName;
                        return frame;
                    }
                }
            }
        }
    }

    // 3. Third test: Does the full pitch class set match an inverted chord (e.g. C/E, Dm/F)?
    for (int candidateRoot : pitchClasses) {
        if (candidateRoot == bassPc) continue; // Root position already checked above

        std::set<int> intervals;
        for (int pc : pitchClasses) {
            intervals.insert((pc - candidateRoot + 12) % 12);
        }

        for (const auto& tpl : templates) {
            if (tpl.intervals.size() == pitchClasses.size()) {
                bool exactMatch = true;
                for (int ti : tpl.intervals) {
                    if (!intervals.count(ti)) {
                        exactMatch = false;
                        break;
                    }
                }

                if (exactMatch) {
                    frame.rootPitchClass = candidateRoot;
                    frame.quality = tpl.quality;
                    frame.chordTones = tpl.intervals;
                    frame.isSlashChord = true;
                    std::string rootName = pitchClassToName(candidateRoot, true);
                    std::string bassName = pitchClassToName(bassPc, true);
                    frame.chordName = rootName + tpl.suffix + "/" + bassName;
                    return frame;
                }
            }
        }
    }

    // 4. Fourth test: Partial match with scoring (handles omissions like 9th without 5th, or dense voicings)
    int bestScore = -1000;
    int bestRoot = bassPc;
    const ChordTemplate* bestTpl = nullptr;

    for (int candidateRoot : pitchClasses) {
        std::set<int> intervals;
        for (int pc : pitchClasses) {
            intervals.insert((pc - candidateRoot + 12) % 12);
        }

        for (const auto& tpl : templates) {
            int matchedIntervals = 0;
            for (int ti : tpl.intervals) {
                if (intervals.count(ti)) {
                    matchedIntervals++;
                }
            }

            // At least 3 notes matched
            if (matchedIntervals >= 3 && matchedIntervals >= (int)tpl.intervals.size() - 1) {
                int score = tpl.priority * 10;
                int extraNotes = (int)intervals.size() - matchedIntervals;
                score -= extraNotes * 25;
                if (candidateRoot == bassPc) {
                    score += 50;
                }

                if (score > bestScore) {
                    bestScore = score;
                    bestRoot = candidateRoot;
                    bestTpl = &tpl;
                }
            }
        }
    }

    if (bestTpl != nullptr) {
        frame.rootPitchClass = bestRoot;
        frame.quality = bestTpl->quality;
        frame.chordTones = bestTpl->intervals;

        std::string rootName = pitchClassToName(bestRoot, true);
        std::string name = rootName + bestTpl->suffix;

        if (bassPc != bestRoot) {
            frame.isSlashChord = true;
            std::string bassName = pitchClassToName(bassPc, true);
            name += "/" + bassName;
        } else {
            frame.isSlashChord = false;
        }

        frame.chordName = name;
        return frame;
    }

    // 5. Fallback: Power chord or Cluster
    if (pitchClasses.size() == 2 && pitchClasses.count((bassPc + 7) % 12)) {
        frame.rootPitchClass = bassPc;
        frame.quality = ChordQuality::PowerChord;
        frame.chordTones = {0, 7};
        frame.chordName = pitchClassToName(bassPc, true) + "5";
    } else {
        frame.rootPitchClass = bassPc;
        frame.quality = ChordQuality::Cluster;
        frame.chordName = pitchClassToName(bassPc, true) + " Clust";
    }

    return frame;
}

} // namespace Harmonic
