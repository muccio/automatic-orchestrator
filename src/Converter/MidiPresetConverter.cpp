#include "MidiPresetConverter.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>

namespace Converter {

// Standard Krumhansl-Schmuckler Key Profiles (12 pitch classes: tonic, m2, M2, m3, M3, P4, tritone, P5, m6, M6, m7, M7)
static const std::vector<float> KS_MAJOR_PROFILE = {
    6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f
};

static const std::vector<float> KS_MINOR_PROFILE = {
    6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f
};

static const char* PITCH_NAMES[12] = {
    "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
};

MidiPresetConverter::MidiPresetConverter() {
}

uint32_t MidiPresetConverter::readBigEndian32(const uint8_t* ptr) {
    return (static_cast<uint32_t>(ptr[0]) << 24) |
           (static_cast<uint32_t>(ptr[1]) << 16) |
           (static_cast<uint32_t>(ptr[2]) << 8)  |
           (static_cast<uint32_t>(ptr[3]));
}

uint16_t MidiPresetConverter::readBigEndian16(const uint8_t* ptr) {
    return static_cast<uint16_t>((static_cast<uint16_t>(ptr[0]) << 8) | static_cast<uint16_t>(ptr[1]));
}

int MidiPresetConverter::readVariableLength(const uint8_t*& ptr, const uint8_t* end) {
    int value = 0;
    while (ptr < end) {
        uint8_t b = *ptr++;
        value = (value << 7) | (b & 0x7F);
        if ((b & 0x80) == 0) break;
    }
    return value;
}

bool MidiPresetConverter::parseMidiFile(const std::string& path, ParsedMidiFile& outData, std::string& errorMsg) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        errorMsg = "Unable to open MIDI file: " + path;
        return false;
    }

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    outData.filePath = path;
    size_t lastSlash = path.find_last_of("/\\");
    outData.fileName = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;

    return parseMidiBytes(buffer.data(), buffer.size(), outData, errorMsg);
}

bool MidiPresetConverter::parseMidiBytes(const uint8_t* data, size_t size, ParsedMidiFile& outData, std::string& errorMsg) {
    if (size < 14) {
        errorMsg = "File is too small to be a valid MIDI file.";
        return false;
    }

    const uint8_t* ptr = data;
    const uint8_t* end = data + size;

    // Check Header Chunk 'MThd'
    if (ptr[0] != 'M' || ptr[1] != 'T' || ptr[2] != 'h' || ptr[3] != 'd') {
        errorMsg = "Invalid MIDI header chunk: missing 'MThd'";
        return false;
    }
    ptr += 4;

    uint32_t headerLen = readBigEndian32(ptr);
    ptr += 4;
    if (headerLen < 6 || ptr + headerLen > end) {
        errorMsg = "Corrupted MIDI header chunk length.";
        return false;
    }

    outData.format = readBigEndian16(ptr);
    ptr += 2;
    uint16_t numTracks = readBigEndian16(ptr);
    ptr += 2;
    uint16_t division = readBigEndian16(ptr);
    ptr += 2;

    if (division & 0x8000) {
        outData.ticksPerQuarter = 480; // SMTPE fallback
    } else {
        outData.ticksPerQuarter = division > 0 ? division : 480;
    }

    // Skip any extra header bytes
    if (headerLen > 6) {
        ptr += (headerLen - 6);
    }

    outData.tracks.clear();
    int trackIndex = 0;
    int64_t maxTrackTicks = 0;

    while (ptr < end && trackIndex < numTracks) {
        if (ptr + 8 > end) break;
        if (ptr[0] != 'M' || ptr[1] != 'T' || ptr[2] != 'r' || ptr[3] != 'k') {
            // Non-track chunk or padding, skip to next byte
            ptr++;
            continue;
        }
        ptr += 4;

        uint32_t trackLen = readBigEndian32(ptr);
        ptr += 4;

        const uint8_t* trackEnd = ptr + trackLen;
        if (trackEnd > end) {
            trackEnd = end;
        }

        ParsedMidiTrack parsedTrack;
        parsedTrack.trackIndex = trackIndex;
        parsedTrack.trackName = "Track " + std::to_string(trackIndex + 1);

        int64_t currentTick = 0;
        uint8_t runningStatus = 0;

        struct ActiveNote {
            int pitch;
            int velocity;
            int64_t startTick;
            int channel;
        };
        std::map<int, ActiveNote> activeNotes; // key = (channel << 8) | pitch

        while (ptr < trackEnd) {
            int delta = readVariableLength(ptr, trackEnd);
            currentTick += delta;

            if (ptr >= trackEnd) break;

            uint8_t statusByte = *ptr;
            if (statusByte < 0x80) {
                // Use running status
                statusByte = runningStatus;
            } else {
                ptr++;
                if (statusByte < 0xF0) {
                    runningStatus = statusByte;
                }
            }

            if (statusByte >= 0x80 && statusByte <= 0x8F) {
                // Note Off
                int ch = (statusByte & 0x0F) + 1;
                if (ptr + 2 > trackEnd) break;
                int pitch = *ptr++;
                ptr++; // skip off velocity

                int noteKey = (ch << 8) | pitch;
                auto it = activeNotes.find(noteKey);
                if (it != activeNotes.end()) {
                    MidiNoteEvent evt;
                    evt.pitch = it->second.pitch;
                    evt.velocity = it->second.velocity;
                    evt.startTick = it->second.startTick;
                    evt.durationTicks = std::max<int64_t>(1, currentTick - it->second.startTick);
                    evt.channel = ch;
                    parsedTrack.notes.push_back(evt);
                    activeNotes.erase(it);
                }
            } else if (statusByte >= 0x90 && statusByte <= 0x9F) {
                // Note On
                int ch = (statusByte & 0x0F) + 1;
                if (ptr + 2 > trackEnd) break;
                int pitch = *ptr++;
                int vel = *ptr++;

                int noteKey = (ch << 8) | pitch;
                if (vel == 0) {
                    // Note Off via zero velocity
                    auto it = activeNotes.find(noteKey);
                    if (it != activeNotes.end()) {
                        MidiNoteEvent evt;
                        evt.pitch = it->second.pitch;
                        evt.velocity = it->second.velocity;
                        evt.startTick = it->second.startTick;
                        evt.durationTicks = std::max<int64_t>(1, currentTick - it->second.startTick);
                        evt.channel = ch;
                        parsedTrack.notes.push_back(evt);
                        activeNotes.erase(it);
                    }
                } else {
                    // Active Note On
                    if (activeNotes.find(noteKey) != activeNotes.end()) {
                        // Previous note didn't receive Note Off, close it now
                        MidiNoteEvent evt;
                        evt.pitch = activeNotes[noteKey].pitch;
                        evt.velocity = activeNotes[noteKey].velocity;
                        evt.startTick = activeNotes[noteKey].startTick;
                        evt.durationTicks = std::max<int64_t>(1, currentTick - activeNotes[noteKey].startTick);
                        evt.channel = ch;
                        parsedTrack.notes.push_back(evt);
                    }
                    activeNotes[noteKey] = { pitch, vel, currentTick, ch };
                    parsedTrack.channel = ch;
                }
            } else if (statusByte >= 0xB0 && statusByte <= 0xBF) {
                // Control Change
                int ch = (statusByte & 0x0F) + 1;
                if (ptr + 2 > trackEnd) break;
                int ccNum = *ptr++;
                int ccVal = *ptr++;
                parsedTrack.ccEvents.push_back({ ccNum, ccVal, currentTick, ch });
            } else if (statusByte >= 0xC0 && statusByte <= 0xDF) {
                // Program Change or Channel Aftertouch (1 byte)
                if (ptr < trackEnd) ptr++;
            } else if (statusByte >= 0xE0 && statusByte <= 0xEF) {
                // Pitch Bend (2 bytes)
                if (ptr + 2 <= trackEnd) ptr += 2;
            } else if (statusByte == 0xFF) {
                // Meta Event
                if (ptr >= trackEnd) break;
                uint8_t metaType = *ptr++;
                int metaLen = readVariableLength(ptr, trackEnd);
                if (ptr + metaLen > trackEnd) metaLen = static_cast<int>(trackEnd - ptr);

                if (metaType == 0x03 && metaLen > 0) {
                    // Track Name
                    parsedTrack.trackName = std::string(reinterpret_cast<const char*>(ptr), metaLen);
                } else if (metaType == 0x51 && metaLen == 3) {
                    // Set Tempo (microseconds per quarter note)
                    uint32_t us = (static_cast<uint32_t>(ptr[0]) << 16) |
                                  (static_cast<uint32_t>(ptr[1]) << 8)  |
                                  (static_cast<uint32_t>(ptr[2]));
                    if (us > 0) {
                        outData.bpm = 60000000.0 / static_cast<double>(us);
                    }
                } else if (metaType == 0x58 && metaLen >= 2) {
                    // Time Signature
                    outData.timeSigNum = ptr[0];
                    outData.timeSigDen = 1 << ptr[1];
                }

                ptr += metaLen;
            } else if (statusByte == 0xF0 || statusByte == 0xF7) {
                // SysEx
                int sysexLen = readVariableLength(ptr, trackEnd);
                if (ptr + sysexLen <= trackEnd) {
                    ptr += sysexLen;
                } else {
                    ptr = trackEnd;
                }
            } else {
                // Unknown single-byte event
            }
        }

        // Close any lingering active notes at the end of track
        for (const auto& [k, n] : activeNotes) {
            MidiNoteEvent evt;
            evt.pitch = n.pitch;
            evt.velocity = n.velocity;
            evt.startTick = n.startTick;
            evt.durationTicks = std::max<int64_t>(1, currentTick - n.startTick);
            evt.channel = n.channel;
            parsedTrack.notes.push_back(evt);
        }

        maxTrackTicks = std::max(maxTrackTicks, currentTick);
        ptr = trackEnd; // advance to next chunk

        // Compute track statistics
        parsedTrack.noteCount = static_cast<int>(parsedTrack.notes.size());
        if (!parsedTrack.notes.empty()) {
            int pitchSum = 0;
            for (const auto& n : parsedTrack.notes) {
                parsedTrack.minPitch = std::min(parsedTrack.minPitch, n.pitch);
                parsedTrack.maxPitch = std::max(parsedTrack.maxPitch, n.pitch);
                pitchSum += n.pitch;
            }
            parsedTrack.averagePitch = pitchSum / parsedTrack.noteCount;
            parsedTrack.suggestedInstrument = detectInstrument(parsedTrack.trackName, parsedTrack.channel, parsedTrack.averagePitch);
            parsedTrack.suggestedSection = getSectionForInstrument(parsedTrack.suggestedInstrument);
            parsedTrack.suggestedArticulation = detectArticulation(parsedTrack);

            // Suggested arranger mode
            if (parsedTrack.suggestedSection == Harmonic::OrchestralSection::Strings) {
                if (parsedTrack.suggestedInstrument == Harmonic::InstrumentId::DoubleBasses) parsedTrack.suggestedArrangerMode = "Lowest";
                else if (parsedTrack.suggestedInstrument == Harmonic::InstrumentId::Violins1) parsedTrack.suggestedArrangerMode = "Top";
                else parsedTrack.suggestedArrangerMode = "Chord";
            } else if (parsedTrack.suggestedSection == Harmonic::OrchestralSection::Brass) {
                if (parsedTrack.suggestedInstrument == Harmonic::InstrumentId::Tuba) parsedTrack.suggestedArrangerMode = "Lowest";
                else if (parsedTrack.suggestedInstrument == Harmonic::InstrumentId::Trumpets) parsedTrack.suggestedArrangerMode = "Top";
                else parsedTrack.suggestedArrangerMode = "Chord";
            } else {
                parsedTrack.suggestedArrangerMode = "Top";
            }

            outData.tracks.push_back(parsedTrack);
        }

        trackIndex++;
    }

    outData.totalTicks = maxTrackTicks;
    if (outData.tracks.empty()) {
        errorMsg = "No MIDI note events found in file.";
        return false;
    }

    return true;
}

float MidiPresetConverter::computeCorrelation(const std::vector<float>& x, const std::vector<float>& y) {
    if (x.size() != y.size() || x.empty()) return 0.0f;

    float sumX = 0.0f, sumY = 0.0f;
    for (size_t i = 0; i < x.size(); ++i) {
        sumX += x[i];
        sumY += y[i];
    }
    float meanX = sumX / static_cast<float>(x.size());
    float meanY = sumY / static_cast<float>(y.size());

    float numer = 0.0f;
    float denomX = 0.0f;
    float denomY = 0.0f;

    for (size_t i = 0; i < x.size(); ++i) {
        float diffX = x[i] - meanX;
        float diffY = y[i] - meanY;
        numer += diffX * diffY;
        denomX += diffX * diffX;
        denomY += diffY * diffY;
    }

    float denom = std::sqrt(denomX * denomY);
    if (denom <= 0.00001f) return 0.0f;

    return numer / denom;
}

TonalAnalysisResult MidiPresetConverter::analyzeTonalCenter(const ParsedMidiFile& midiData) {
    TonalAnalysisResult result;
    result.pitchClassDistribution.assign(12, 0.0f);

    std::vector<int> bar1Pitches;
    int lowestBassMidi = 127;
    int ticksPer16th = std::max(1, midiData.ticksPerQuarter / 4);
    int64_t bar1Ticks = ticksPer16th * 16;

    // Weight pitch classes by duration in beats * velocity
    for (const auto& trk : midiData.tracks) {
        for (const auto& n : trk.notes) {
            int pc = ((n.pitch % 12) + 12) % 12;
            float durBeats = static_cast<float>(n.durationTicks) / static_cast<float>(midiData.ticksPerQuarter);
            float weight = durBeats * (n.velocity / 127.0f);
            result.pitchClassDistribution[pc] += weight;

            // Collect notes in the first bar for direct chord detection
            if (n.startTick < bar1Ticks) {
                bar1Pitches.push_back(n.pitch);
                if (n.startTick <= ticksPer16th * 2) {
                    lowestBassMidi = std::min(lowestBassMidi, n.pitch);
                }
            }
        }
    }

    if (lowestBassMidi == 127) {
        lowestBassMidi = 36; // C2 default
    }
    result.referenceBassMidiNote = lowestBassMidi;

    // Normalize distribution vector
    float totalWeight = std::accumulate(result.pitchClassDistribution.begin(), result.pitchClassDistribution.end(), 0.0f);
    if (totalWeight > 0.001f) {
        for (auto& w : result.pitchClassDistribution) {
            w /= totalWeight;
        }
    }

    // Krumhansl-Schmuckler Key Finding
    float bestCorr = -2.0f;
    int bestRoot = 0;
    bool bestIsMajor = true;

    for (int r = 0; r < 12; ++r) {
        // Build circularly shifted profiles for candidate root r
        std::vector<float> candidateMajor(12);
        std::vector<float> candidateMinor(12);
        for (int i = 0; i < 12; ++i) {
            candidateMajor[i] = KS_MAJOR_PROFILE[(i - r + 12) % 12];
            candidateMinor[i] = KS_MINOR_PROFILE[(i - r + 12) % 12];
        }

        float corrMaj = computeCorrelation(result.pitchClassDistribution, candidateMajor);
        float corrMin = computeCorrelation(result.pitchClassDistribution, candidateMinor);

        // Boost keys that align with the downbeat bass root
        int bassPc = ((lowestBassMidi % 12) + 12) % 12;
        if (bassPc == r) {
            corrMaj += 0.08f;
            corrMin += 0.08f;
        }

        if (corrMaj > bestCorr) {
            bestCorr = corrMaj;
            bestRoot = r;
            bestIsMajor = true;
        }
        if (corrMin > bestCorr) {
            bestCorr = corrMin;
            bestRoot = r;
            bestIsMajor = false;
        }
    }

    result.detectedRootPitchClass = bestRoot;
    result.detectedRootName = PITCH_NAMES[bestRoot];
    result.confidence = std::clamp((bestCorr + 1.0f) / 2.0f, 0.0f, 1.0f);

    if (bestIsMajor) {
        result.detectedMode = Harmonic::ScaleMode::Ionian;
        result.detectedChordQuality = Harmonic::ChordQuality::MajorTriad;
        result.detectedChordName = std::string(PITCH_NAMES[bestRoot]) + " Major";
    } else {
        result.detectedMode = Harmonic::ScaleMode::Aeolian;
        result.detectedChordQuality = Harmonic::ChordQuality::MinorTriad;
        result.detectedChordName = std::string(PITCH_NAMES[bestRoot]) + " Minor";
    }

    // Direct Bar 1 Chord Detection confirmation
    if (!bar1Pitches.empty()) {
        std::sort(bar1Pitches.begin(), bar1Pitches.end());
        bar1Pitches.erase(std::unique(bar1Pitches.begin(), bar1Pitches.end()), bar1Pitches.end());
        auto chordFrame = chordDetector.detectChord(bar1Pitches);
        if (chordFrame.quality != Harmonic::ChordQuality::Unknown && chordFrame.quality != Harmonic::ChordQuality::SingleNote) {
            result.detectedChordQuality = chordFrame.quality;
            result.detectedChordName = chordFrame.chordName;
            result.detectedRootPitchClass = chordFrame.rootPitchClass;
            result.detectedRootName = PITCH_NAMES[chordFrame.rootPitchClass];
        }
    }

    return result;
}

Harmonic::InstrumentId MidiPresetConverter::detectInstrument(const std::string& trackName, int channel, int avgPitch) {
    std::string lower = trackName;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });

    // String Matching
    if (lower.find("violin 1") != std::string::npos || lower.find("vln 1") != std::string::npos ||
        lower.find("vln i") != std::string::npos || lower.find("violins 1") != std::string::npos ||
        lower.find("violins i") != std::string::npos || lower.find("1st violins") != std::string::npos) {
        return Harmonic::InstrumentId::Violins1;
    }
    if (lower.find("violin 2") != std::string::npos || lower.find("vln 2") != std::string::npos ||
        lower.find("vln ii") != std::string::npos || lower.find("violins 2") != std::string::npos ||
        lower.find("violins ii") != std::string::npos || lower.find("2nd violins") != std::string::npos) {
        return Harmonic::InstrumentId::Violins2;
    }
    if (lower.find("violin") != std::string::npos || lower.find("vln") != std::string::npos) {
        return (avgPitch > 74) ? Harmonic::InstrumentId::Violins1 : Harmonic::InstrumentId::Violins2;
    }
    if (lower.find("viola") != std::string::npos || lower.find("vla") != std::string::npos) {
        return Harmonic::InstrumentId::Violas;
    }
    if (lower.find("cello") != std::string::npos || lower.find("vlc") != std::string::npos || lower.find("vc") != std::string::npos) {
        return Harmonic::InstrumentId::Cellos;
    }
    if (lower.find("bass") != std::string::npos || lower.find("contrabass") != std::string::npos ||
        lower.find("double bass") != std::string::npos || lower.find("cb") != std::string::npos) {
        if (lower.find("bassoon") == std::string::npos) {
            return Harmonic::InstrumentId::DoubleBasses;
        }
    }

    // Brass Matching
    if (lower.find("horn") != std::string::npos || lower.find("corno") != std::string::npos || lower.find("hrn") != std::string::npos) {
        return Harmonic::InstrumentId::FrenchHorns;
    }
    if (lower.find("trumpet") != std::string::npos || lower.find("tromba") != std::string::npos ||
        lower.find("tpt") != std::string::npos || lower.find("trpt") != std::string::npos) {
        return Harmonic::InstrumentId::Trumpets;
    }
    if (lower.find("trombone") != std::string::npos || lower.find("posaune") != std::string::npos || lower.find("trb") != std::string::npos) {
        return Harmonic::InstrumentId::Trombones;
    }
    if (lower.find("tuba") != std::string::npos || lower.find("sousaphone") != std::string::npos) {
        return Harmonic::InstrumentId::Tuba;
    }

    // Woodwinds Matching
    if (lower.find("flute") != std::string::npos || lower.find("flauto") != std::string::npos || lower.find("fl") != std::string::npos) {
        return Harmonic::InstrumentId::Flutes;
    }
    if (lower.find("oboe") != std::string::npos || lower.find("hautbois") != std::string::npos || lower.find("ob") != std::string::npos) {
        return Harmonic::InstrumentId::Oboes;
    }
    if (lower.find("clarinet") != std::string::npos || lower.find("cl") != std::string::npos) {
        return Harmonic::InstrumentId::Clarinets;
    }
    if (lower.find("bassoon") != std::string::npos || lower.find("fagott") != std::string::npos || lower.find("fg") != std::string::npos) {
        return Harmonic::InstrumentId::Bassoons;
    }

    // Percussion Matching
    if (lower.find("timpani") != std::string::npos || lower.find("timp") != std::string::npos) {
        return Harmonic::InstrumentId::Timpani;
    }
    if (lower.find("perc") != std::string::npos || lower.find("drum") != std::string::npos ||
        lower.find("cymb") != std::string::npos || lower.find("snare") != std::string::npos) {
        return Harmonic::InstrumentId::OrchestralPerc;
    }

    // Channel-based fallback (Standard 16-channel template)
    switch (channel) {
        case 1: return Harmonic::InstrumentId::Violins1;
        case 2: return Harmonic::InstrumentId::Violins2;
        case 3: return Harmonic::InstrumentId::Violas;
        case 4: return Harmonic::InstrumentId::Cellos;
        case 5: return Harmonic::InstrumentId::DoubleBasses;
        case 6: return Harmonic::InstrumentId::FrenchHorns;
        case 7: return Harmonic::InstrumentId::Trumpets;
        case 8: return Harmonic::InstrumentId::Trombones;
        case 9: return Harmonic::InstrumentId::Tuba;
        case 10: return Harmonic::InstrumentId::Flutes;
        case 11: return Harmonic::InstrumentId::Oboes;
        case 12: return Harmonic::InstrumentId::Clarinets;
        case 13: return Harmonic::InstrumentId::Bassoons;
        case 14: return Harmonic::InstrumentId::Timpani;
        case 15: return Harmonic::InstrumentId::OrchestralPerc;
        default: break;
    }

    // Pitch register fallback
    if (avgPitch >= 76) return Harmonic::InstrumentId::Violins1;
    if (avgPitch >= 64) return Harmonic::InstrumentId::Violins2;
    if (avgPitch >= 52) return Harmonic::InstrumentId::Violas;
    if (avgPitch >= 40) return Harmonic::InstrumentId::Cellos;
    return Harmonic::InstrumentId::DoubleBasses;
}

Harmonic::OrchestralSection MidiPresetConverter::getSectionForInstrument(Harmonic::InstrumentId inst) {
    return Harmonic::getInstrumentSection(inst);
}

Harmonic::ArticulationType MidiPresetConverter::detectArticulation(const ParsedMidiTrack& track) {
    // 1. Check for CC58 (Cinematic Studio Series)
    for (const auto& cc : track.ccEvents) {
        if (cc.ccNumber == 58) {
            if (cc.value <= 15) return Harmonic::ArticulationType::Sustain;
            if (cc.value <= 35) return Harmonic::ArticulationType::Staccato;
            if (cc.value <= 55) return Harmonic::ArticulationType::Spiccato;
            if (cc.value <= 70) return Harmonic::ArticulationType::Marcato;
            if (cc.value <= 90) return Harmonic::ArticulationType::Tremolo;
            if (cc.value <= 110) return Harmonic::ArticulationType::Pizzicato;
        } else if (cc.ccNumber == 32) { // Spitfire UACC
            if (cc.value == 1) return Harmonic::ArticulationType::Sustain;
            if (cc.value == 40) return Harmonic::ArticulationType::Staccato;
            if (cc.value == 42) return Harmonic::ArticulationType::Spiccato;
            if (cc.value == 56) return Harmonic::ArticulationType::Pizzicato;
            if (cc.value == 11) return Harmonic::ArticulationType::Tremolo;
        }
    }

    // 2. Note length heuristic
    if (track.notes.empty()) return Harmonic::ArticulationType::Sustain;

    int64_t totalDuration = 0;
    for (const auto& n : track.notes) {
        totalDuration += n.durationTicks;
    }
    float avgDurationBeats = static_cast<float>(totalDuration) / static_cast<float>(track.notes.size() * 480);

    if (avgDurationBeats <= 0.35f) {
        return Harmonic::ArticulationType::Spiccato;
    } else if (avgDurationBeats <= 0.65f) {
        return Harmonic::ArticulationType::Staccato;
    }

    return Harmonic::ArticulationType::Sustain;
}

int MidiPresetConverter::pitchToScaleDegreeOffset(int midiPitch, int rootMidiNote, Harmonic::ScaleMode mode) {
    int semitoneDiff = midiPitch - rootMidiNote;
    int octaves = semitoneDiff / 12;
    int pcDiff = semitoneDiff % 12;
    if (pcDiff < 0) {
        pcDiff += 12;
        octaves -= 1;
    }

    // Map 12 semitones to standard diatonic degrees (0, 1, 2, 3, 4, 5, 6)
    auto intervals = Harmonic::getScaleModeIntervals(mode);
    int closestDegree = 0;
    int minDistance = 99;

    for (size_t d = 0; d < intervals.size(); ++d) {
        int dist = std::abs(intervals[d] - pcDiff);
        if (dist < minDistance) {
            minDistance = dist;
            closestDegree = static_cast<int>(d);
        }
    }

    int degreeOffset = octaves * 7 + closestDegree;
    return std::clamp(degreeOffset, -8, 9);
}

Sequencer::OrchestralPattern MidiPresetConverter::convertToPattern(const ParsedMidiFile& midiData,
                                                                   const TonalAnalysisResult& tonalResult,
                                                                   const ConversionOptions& options) {
    Sequencer::OrchestralPattern pattern;
    pattern.name = options.presetName.empty() ? midiData.fileName : options.presetName;
    pattern.bpm = (options.tempoBpm > 20.0) ? options.tempoBpm : midiData.bpm;
    int numSteps = (options.lengthSteps > 0) ? options.lengthSteps : 16;
    pattern.barLength = (numSteps >= 32) ? 2 : 1;

    int rootPc = (options.overrideRootPitchClass >= 0) ? options.overrideRootPitchClass : tonalResult.detectedRootPitchClass;
    Harmonic::ScaleMode scaleMode = options.useOverrideMode ? options.overrideMode : tonalResult.detectedMode;

    // Establish canonical reference root pitch in octave 3 (MIDI note 48 + rootPc)
    int referenceRootMidi = 48 + rootPc;

    int ticksPer16th = std::max(1, midiData.ticksPerQuarter / 4);
    int64_t maxPatternTicks = static_cast<int64_t>(numSteps) * ticksPer16th;

    for (const auto& track : midiData.tracks) {
        // Check if track is enabled in configuration
        bool isEnabled = track.isEnabled;
        TrackMappingConfig config;
        config.instrument = track.suggestedInstrument;
        config.section = track.suggestedSection;
        config.articulation = track.suggestedArticulation;
        config.arrangerMode = track.suggestedArrangerMode;
        config.enabled = true;

        auto it = options.trackConfigs.find(track.trackIndex);
        if (it != options.trackConfigs.end()) {
            config = it->second;
            isEnabled = config.enabled;
        }

        if (!isEnabled || track.notes.empty()) {
            continue;
        }

        // Initialize track pattern
        Sequencer::TrackPattern trackPattern;
        trackPattern.instrument = config.instrument;
        trackPattern.trackName = track.trackName;
        trackPattern.section = config.section;
        trackPattern.midiChannel = track.channel;
        trackPattern.articulation = config.articulation;
        trackPattern.arrangerMode = config.arrangerMode;
        trackPattern.octaveOffset = config.octaveOffset;
        trackPattern.volume = config.volume;
        trackPattern.pan = config.pan;
        trackPattern.stepCount = numSteps;
        trackPattern.steps.resize(numSteps);

        for (int s = 0; s < numSteps; ++s) {
            trackPattern.steps[s].active = false;
            trackPattern.steps[s].stepOffset = 0;
            trackPattern.steps[s].velocity = 100;
            trackPattern.steps[s].lengthSteps = 1;
            trackPattern.steps[s].action = Harmonic::StepActionType::Rest;
        }

        // Map notes into steps
        for (const auto& n : track.notes) {
            if (n.startTick >= maxPatternTicks) continue;

            int step = static_cast<int>(std::round(static_cast<double>(n.startTick) / ticksPer16th));
            if (step < 0 || step >= numSteps) continue;

            int durSteps = static_cast<int>(std::round(static_cast<double>(n.durationTicks) / ticksPer16th));
            durSteps = std::clamp(durSteps, 1, numSteps - step);

            int offset = pitchToScaleDegreeOffset(n.pitch, referenceRootMidi, scaleMode);

            trackPattern.steps[step].active = true;
            trackPattern.steps[step].stepOffset = offset;
            trackPattern.steps[step].velocity = n.velocity;
            trackPattern.steps[step].lengthSteps = durSteps;
            trackPattern.steps[step].action = (durSteps > 1) ? Harmonic::StepActionType::Sustain : Harmonic::StepActionType::Ostinato;
        }

        // Sample CC1 (Modulation) curve across steps
        trackPattern.cc1Curve.assign(numSteps, 80);
        for (const auto& cc : track.ccEvents) {
            if (cc.ccNumber == 1 && cc.tick < maxPatternTicks) {
                int step = static_cast<int>(std::round(static_cast<double>(cc.tick) / ticksPer16th));
                if (step >= 0 && step < numSteps) {
                    trackPattern.cc1Curve[step] = cc.value;
                }
            }
        }

        pattern.tracks[config.instrument] = trackPattern;
    }

    return pattern;
}

bool MidiPresetConverter::convertAndSavePreset(const std::string& midiPath,
                                              const std::string& outputJsonPath,
                                              const ConversionOptions& options,
                                              std::string& errorMsg) {
    ParsedMidiFile midiFile;
    if (!parseMidiFile(midiPath, midiFile, errorMsg)) {
        return false;
    }

    TonalAnalysisResult tonal = analyzeTonalCenter(midiFile);
    Sequencer::OrchestralPattern pattern = convertToPattern(midiFile, tonal, options);

    if (!pattern.saveToFile(outputJsonPath)) {
        errorMsg = "Failed to write preset JSON to " + outputJsonPath;
        return false;
    }

    return true;
}

} // namespace Converter
