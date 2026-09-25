#include "ConverterApp.h"
#include <iomanip>
#include <cmath>

namespace Converter {

// -------------------------------------------------------------
// Helper function to map InstrumentId to/from ComboBox ID (1-based)
// -------------------------------------------------------------
static const Harmonic::InstrumentId ALL_INSTRUMENTS[] = {
    Harmonic::InstrumentId::Violins1,
    Harmonic::InstrumentId::Violins2,
    Harmonic::InstrumentId::Violas,
    Harmonic::InstrumentId::Cellos,
    Harmonic::InstrumentId::DoubleBasses,
    Harmonic::InstrumentId::Harp,
    Harmonic::InstrumentId::Trumpets,
    Harmonic::InstrumentId::FrenchHorns,
    Harmonic::InstrumentId::Trombones,
    Harmonic::InstrumentId::Tuba,
    Harmonic::InstrumentId::Flutes,
    Harmonic::InstrumentId::Oboes,
    Harmonic::InstrumentId::Clarinets,
    Harmonic::InstrumentId::Bassoons,
    Harmonic::InstrumentId::Timpani,
    Harmonic::InstrumentId::OrchestralPerc,
    Harmonic::InstrumentId::Celesta,
    Harmonic::InstrumentId::Piano,
    Harmonic::InstrumentId::ChurchOrgan,
    Harmonic::InstrumentId::AcousticGuitar,
    Harmonic::InstrumentId::ElectricGuitar,
    Harmonic::InstrumentId::BassGuitar,
    Harmonic::InstrumentId::ChoirFull,
    Harmonic::InstrumentId::SynthesizerLead,
    Harmonic::InstrumentId::SynthesizerPad
};
static const int NUM_ALL_INSTRUMENTS = sizeof(ALL_INSTRUMENTS) / sizeof(ALL_INSTRUMENTS[0]);

static juce::String formatStepDuration(int steps) {
    if (steps == 1) return "1/16";
    if (steps == 2) return "1/8";
    if (steps == 3) return "1/8 d";
    if (steps == 4) return "1/4";
    if (steps == 6) return "1/4 d";
    if (steps == 8) return "1/2";
    if (steps == 12) return "1/2 d";
    if (steps == 16) return "1 Bar";
    if (steps == 32) return "2 Bars";
    if (steps == 48) return "3 Bars";
    if (steps == 64) return "4 Bars";
    if (steps == 96) return "6 Bars";
    if (steps == 128) return "8 Bars";
    return juce::String(steps) + " st";
}

static int instrumentToId(Harmonic::InstrumentId id) {
    for (int i = 0; i < NUM_ALL_INSTRUMENTS; ++i) {
        if (ALL_INSTRUMENTS[i] == id) return i + 1;
    }
    return 1;
}

static Harmonic::InstrumentId idToInstrument(int id) {
    if (id >= 1 && id <= NUM_ALL_INSTRUMENTS) return ALL_INSTRUMENTS[id - 1];
    return Harmonic::InstrumentId::Violins1;
}

static juce::Colour getSectionColour(Harmonic::OrchestralSection sec) {
    switch (sec) {
        case Harmonic::OrchestralSection::Strings:    return juce::Colour(0xffe53e3e); // Crimson
        case Harmonic::OrchestralSection::Brass:      return juce::Colour(0xffdd6b20); // Amber
        case Harmonic::OrchestralSection::Woodwinds:  return juce::Colour(0xff319795); // Teal / Cyan
        case Harmonic::OrchestralSection::Percussion: return juce::Colour(0xff38a169); // Forest green
        case Harmonic::OrchestralSection::Keyboards:  return juce::Colour(0xff805ad5); // Royal Purple
        case Harmonic::OrchestralSection::Guitars:    return juce::Colour(0xffd69e2e); // Warm Gold
        case Harmonic::OrchestralSection::Choir:      return juce::Colour(0xff3182ce); // Sky Blue
        case Harmonic::OrchestralSection::Synths:     return juce::Colour(0xffd53f8c); // Magenta
        default:                                      return juce::Colour(0xff718096);
    }
}

static juce::String getSectionShortName(Harmonic::OrchestralSection sec) {
    switch (sec) {
        case Harmonic::OrchestralSection::Strings:    return "STR";
        case Harmonic::OrchestralSection::Brass:      return "BRS";
        case Harmonic::OrchestralSection::Woodwinds:  return "WND";
        case Harmonic::OrchestralSection::Percussion: return "PRC";
        case Harmonic::OrchestralSection::Keyboards:  return "KEY";
        case Harmonic::OrchestralSection::Guitars:    return "GTR";
        case Harmonic::OrchestralSection::Choir:      return "CHR";
        case Harmonic::OrchestralSection::Synths:     return "SYN";
        default:                                      return "ORC";
    }
}

// -------------------------------------------------------------
// TrackMappingRowComponent Implementation
// -------------------------------------------------------------
TrackMappingRowComponent::TrackMappingRowComponent(int trackIdx, const ParsedMidiTrack& trk, std::function<void()> onChange)
    : trackIndex(trackIdx), onConfigChanged(onChange)
{
    enableToggle.setToggleState(trk.isEnabled, juce::dontSendNotification);
    enableToggle.onClick = [this]() {
        bool en = enableToggle.getToggleState();
        instrumentSelector.setEnabled(en);
        articulationSelector.setEnabled(en);
        arrangerModeSelector.setEnabled(en);
        if (onConfigChanged) onConfigChanged();
        repaint();
    };
    addAndMakeVisible(enableToggle);

    trackNameLabel.setText(juce::String(trk.trackName) + " (Ch " + juce::String(trk.channel) + ")", juce::dontSendNotification);
    trackNameLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    trackNameLabel.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    addAndMakeVisible(trackNameLabel);

    juce::String minNote = Harmonic::noteNumberToName(trk.minPitch).c_str();
    juce::String maxNote = Harmonic::noteNumberToName(trk.maxPitch).c_str();
    noteStatsLabel.setText(juce::String(trk.noteCount) + " notes (" + minNote + " - " + maxNote + ")", juce::dontSendNotification);
    noteStatsLabel.setFont(juce::Font(10.5f));
    noteStatsLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa0aec0));
    addAndMakeVisible(noteStatsLabel);

    for (int i = 0; i < NUM_ALL_INSTRUMENTS; ++i) {
        instrumentSelector.addItem(Harmonic::instrumentToString(ALL_INSTRUMENTS[i]).c_str(), i + 1);
    }
    instrumentSelector.setSelectedId(instrumentToId(trk.suggestedInstrument), juce::dontSendNotification);
    instrumentSelector.onChange = [this]() {
        updateSectionBadge();
        if (onConfigChanged) onConfigChanged();
    };
    addAndMakeVisible(instrumentSelector);

    sectionBadge.setFont(juce::Font(10.0f, juce::Font::bold));
    sectionBadge.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(sectionBadge);

    articulationSelector.addItem("Sustain", 1);
    articulationSelector.addItem("Staccato", 2);
    articulationSelector.addItem("Spiccato", 3);
    articulationSelector.addItem("Marcato", 4);
    articulationSelector.addItem("Tremolo", 5);
    articulationSelector.addItem("Pizzicato", 6);
    articulationSelector.addItem("Runs", 7);

    int artId = 1;
    switch (trk.suggestedArticulation) {
        case Harmonic::ArticulationType::Sustain:   artId = 1; break;
        case Harmonic::ArticulationType::Staccato:  artId = 2; break;
        case Harmonic::ArticulationType::Spiccato:  artId = 3; break;
        case Harmonic::ArticulationType::Marcato:   artId = 4; break;
        case Harmonic::ArticulationType::Tremolo:   artId = 5; break;
        case Harmonic::ArticulationType::Pizzicato: artId = 6; break;
        case Harmonic::ArticulationType::Runs:      artId = 7; break;
        default: artId = 1; break;
    }
    articulationSelector.setSelectedId(artId, juce::dontSendNotification);
    articulationSelector.onChange = [this]() {
        if (onConfigChanged) onConfigChanged();
    };
    addAndMakeVisible(articulationSelector);

    arrangerModeSelector.addItem("Top", 1);
    arrangerModeSelector.addItem("Lowest", 2);
    arrangerModeSelector.addItem("Chord", 3);
    arrangerModeSelector.addItem("Root", 4);
    arrangerModeSelector.addItem("Arp Up", 5);
    arrangerModeSelector.addItem("Arp Down", 6);

    int modeId = 1;
    if (trk.suggestedArrangerMode == "Lowest") modeId = 2;
    else if (trk.suggestedArrangerMode == "Chord") modeId = 3;
    else if (trk.suggestedArrangerMode == "Root") modeId = 4;
    else if (trk.suggestedArrangerMode == "Arp Up") modeId = 5;
    else if (trk.suggestedArrangerMode == "Arp Down") modeId = 6;
    arrangerModeSelector.setSelectedId(modeId, juce::dontSendNotification);
    arrangerModeSelector.onChange = [this]() {
        if (onConfigChanged) onConfigChanged();
    };
    addAndMakeVisible(arrangerModeSelector);

    updateSectionBadge();
}

void TrackMappingRowComponent::updateSectionBadge() {
    auto inst = idToInstrument(instrumentSelector.getSelectedId());
    auto sec = Harmonic::getInstrumentSection(inst);
    juce::Colour bgCol = getSectionColour(sec);
    juce::String name = getSectionShortName(sec);

    sectionBadge.setText(name, juce::dontSendNotification);
    sectionBadge.setColour(juce::Label::backgroundColourId, bgCol.withAlpha(0.25f));
    sectionBadge.setColour(juce::Label::textColourId, bgCol.brighter(0.4f));
    sectionBadge.setColour(juce::Label::outlineColourId, bgCol.withAlpha(0.6f));
}

TrackMappingConfig TrackMappingRowComponent::getConfig() const {
    TrackMappingConfig cfg;
    cfg.instrument = idToInstrument(instrumentSelector.getSelectedId());
    cfg.section = Harmonic::getInstrumentSection(cfg.instrument);
    cfg.enabled = enableToggle.getToggleState();

    switch (articulationSelector.getSelectedId()) {
        case 1: cfg.articulation = Harmonic::ArticulationType::Sustain; break;
        case 2: cfg.articulation = Harmonic::ArticulationType::Staccato; break;
        case 3: cfg.articulation = Harmonic::ArticulationType::Spiccato; break;
        case 4: cfg.articulation = Harmonic::ArticulationType::Marcato; break;
        case 5: cfg.articulation = Harmonic::ArticulationType::Tremolo; break;
        case 6: cfg.articulation = Harmonic::ArticulationType::Pizzicato; break;
        case 7: cfg.articulation = Harmonic::ArticulationType::Runs; break;
        default: cfg.articulation = Harmonic::ArticulationType::Sustain; break;
    }

    switch (arrangerModeSelector.getSelectedId()) {
        case 1: cfg.arrangerMode = "Top"; break;
        case 2: cfg.arrangerMode = "Lowest"; break;
        case 3: cfg.arrangerMode = "Chord"; break;
        case 4: cfg.arrangerMode = "Root"; break;
        case 5: cfg.arrangerMode = "Arp Up"; break;
        case 6: cfg.arrangerMode = "Arp Down"; break;
        default: cfg.arrangerMode = "Chord"; break;
    }
    return cfg;
}

void TrackMappingRowComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    bool en = enableToggle.getToggleState();
    g.setColour(en ? juce::Colour(0xff1a202c) : juce::Colour(0xff12161f));
    g.fillRoundedRectangle(bounds.reduced(1.0f), 4.0f);
    g.setColour(en ? juce::Colour(0xff2d3748) : juce::Colour(0xff1a202c));
    g.drawRoundedRectangle(bounds.reduced(1.0f), 4.0f, 1.0f);
}

void TrackMappingRowComponent::resized() {
    auto area = getLocalBounds().reduced(8, 4);
    enableToggle.setBounds(area.removeFromLeft(28).withSizeKeepingCentre(20, 20));

    auto nameArea = area.removeFromLeft(190);
    trackNameLabel.setBounds(nameArea.removeFromTop(18));
    noteStatsLabel.setBounds(nameArea);

    area.removeFromLeft(10);
    instrumentSelector.setBounds(area.removeFromLeft(160).withSizeKeepingCentre(155, 24));

    area.removeFromLeft(8);
    sectionBadge.setBounds(area.removeFromLeft(50).withSizeKeepingCentre(48, 20));

    area.removeFromLeft(12);
    articulationSelector.setBounds(area.removeFromLeft(110).withSizeKeepingCentre(105, 24));

    area.removeFromLeft(12);
    arrangerModeSelector.setBounds(area.removeFromLeft(100).withSizeKeepingCentre(95, 24));
}

// -------------------------------------------------------------
// AuditionAudioPlayer Implementation
// -------------------------------------------------------------
AuditionAudioPlayer::AuditionAudioPlayer() {
    // Initialise audio device manager safely
    juce::String err = deviceManager.initialiseWithDefaultDevices(0, 2);
    if (err.isEmpty()) {
        deviceManager.addAudioCallback(this);
    }
}

AuditionAudioPlayer::~AuditionAudioPlayer() {
    stopPlayback();
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
}

void AuditionAudioPlayer::startPlayback() {
    stepSampleCounter = 0.0;
    currentStep.store(0);
    advanceStepAndTriggerNotes();
    playing.store(true);
}

void AuditionAudioPlayer::stopPlayback() {
    playing.store(false);
    std::lock_guard<std::mutex> lock(patternMutex);
    for (int i = 0; i < NUM_VOICES; ++i) {
        voices[i].active = false;
        voices[i].env = 0.0f;
    }
}

void AuditionAudioPlayer::setPattern(const Sequencer::OrchestralPattern& pat) {
    std::lock_guard<std::mutex> lock(patternMutex);
    cachedPattern = pat;
    cachedStepCount = pat.barLength * 16;
    for (const auto& [inst, trk] : pat.tracks) {
        if ((int)trk.stepCount > cachedStepCount) cachedStepCount = trk.stepCount;
    }
    if (pat.bpm > 20.0) currentBpm.store(pat.bpm);
    rebuildAuditionHarmony();
}

void AuditionAudioPlayer::updateAuditionChord(int rootPc, Harmonic::ChordQuality quality) {
    std::lock_guard<std::mutex> lock(patternMutex);
    cachedChord.rootPitchClass = ((rootPc % 12) + 12) % 12;
    cachedChord.quality = quality;
    cachedChord.chordTones = Harmonic::getChordQualityIntervals(quality);
    cachedChord.bassMidiNote = cachedChord.rootPitchClass + 36;
    rebuildAuditionHarmony();
}

void AuditionAudioPlayer::rebuildAuditionHarmony() {
    cachedPitchLadder.clear();
    std::vector<int> chordPcs;
    for (int iv : cachedChord.chordTones) {
        chordPcs.push_back((cachedChord.rootPitchClass + iv) % 12);
    }
    std::sort(chordPcs.begin(), chordPcs.end());
    chordPcs.erase(std::unique(chordPcs.begin(), chordPcs.end()), chordPcs.end());
    if (chordPcs.empty()) chordPcs = {cachedChord.rootPitchClass, (cachedChord.rootPitchClass + 4) % 12, (cachedChord.rootPitchClass + 7) % 12};

    for (int oct = 1; oct <= 9; ++oct) {
        for (int pc : chordPcs) {
            int p = oct * 12 + pc;
            if (p >= 12 && p <= 127) cachedPitchLadder.push_back(p);
        }
    }
    std::sort(cachedPitchLadder.begin(), cachedPitchLadder.end());
    cachedPitchLadder.erase(std::unique(cachedPitchLadder.begin(), cachedPitchLadder.end()), cachedPitchLadder.end());

    Orchestration::VoicingEngine ve;
    auto voicing = ve.generateVoicing(cachedChord);
    cachedVoiceBases.clear();
    for (const auto& v : voicing.voices) {
        cachedVoiceBases[v.instrument] = v.midiPitch;
    }
}

void AuditionAudioPlayer::audioDeviceAboutToStart(juce::AudioIODevice* device) {
    if (device != nullptr) {
        sampleRate = device->getCurrentSampleRate();
    }
}

void AuditionAudioPlayer::audioDeviceStopped() {
    stopPlayback();
}

void AuditionAudioPlayer::advanceStepAndTriggerNotes() {
    if (cachedStepCount <= 0) return;
    int step = currentStep.load();

    double secondsPer16th = 60.0 / (currentBpm.load() * 4.0);
    double stepSamples = secondsPer16th * sampleRate;

    // Check solo states
    bool anySolo = false;
    for (const auto& [inst, trk] : cachedPattern.tracks) {
        if (trk.isSolo) { anySolo = true; break; }
    }

    for (const auto& [inst, trk] : cachedPattern.tracks) {
        if (trk.steps.empty()) continue;
        if (anySolo) {
            if (!trk.isSolo) continue;
        } else if (trk.isMuted) {
            continue;
        }

        const auto& stepDef = trk.steps[step % trk.steps.size()];
        if (!stepDef.active || stepDef.action == Harmonic::StepActionType::Rest) continue;

        int voiceBase = 60;
        if (cachedVoiceBases.find(inst) != cachedVoiceBases.end()) {
            voiceBase = cachedVoiceBases[inst];
        }
        int effectiveBase = voiceBase + (trk.octaveOffset * 12);

        int baseIdx = 0;
        if (!cachedPitchLadder.empty()) {
            int minDiff = 999;
            for (size_t i = 0; i < cachedPitchLadder.size(); ++i) {
                int d = std::abs(cachedPitchLadder[i] - effectiveBase);
                if (d < minDiff) { minDiff = d; baseIdx = static_cast<int>(i); }
            }
        }

        std::vector<int> allOffsets = { stepDef.stepOffset };
        for (int eo : stepDef.extraOffsets) allOffsets.push_back(eo);

        for (int offVal : allOffsets) {
            int targetPitch = effectiveBase;
            if (!cachedPitchLadder.empty()) {
                int targetIdx = std::clamp(baseIdx + offVal, 0, (int)cachedPitchLadder.size() - 1);
                targetPitch = cachedPitchLadder[targetIdx];
            }
            targetPitch += (stepDef.octaveOffset * 12);
            targetPitch = std::clamp(targetPitch, 12, 127);

            // Assign synth voice
            auto& voice = voices[nextVoiceIndex];
            nextVoiceIndex = (nextVoiceIndex + 1) % NUM_VOICES;

            voice.active = true;
            voice.isReleasing = false;
            voice.env = 0.0f;
            voice.section = trk.section;
            voice.velocity = std::clamp((stepDef.velocity / 127.0f) * trk.volume, 0.1f, 1.0f);

            double freq = 440.0 * std::pow(2.0, (targetPitch - 69) / 12.0);
            voice.phaseDelta = static_cast<float>((freq * 2.0 * 3.1415926535) / sampleRate);

            int durSteps = std::clamp(stepDef.lengthSteps, 1, 128);
            voice.remainingSamples = stepSamples * durSteps * std::clamp(stepDef.gate, 0.1, 0.95);

            // Natural panning
            float pan = trk.pan;
            if (trk.section == Harmonic::OrchestralSection::Strings) {
                pan = (inst == Harmonic::InstrumentId::Violins1) ? -0.4f : ((inst == Harmonic::InstrumentId::Violins2) ? -0.2f : ((inst == Harmonic::InstrumentId::Cellos) ? 0.35f : 0.0f));
            } else if (trk.section == Harmonic::OrchestralSection::Brass) {
                pan = 0.25f;
            } else if (trk.section == Harmonic::OrchestralSection::Woodwinds) {
                pan = -0.15f;
            }
            voice.panL = std::clamp(0.707f * (1.0f - pan), 0.0f, 1.0f);
            voice.panR = std::clamp(0.707f * (1.0f + pan), 0.0f, 1.0f);

            // Articulation-specific envelope rates
            if (stepDef.articulation == Harmonic::ArticulationType::Staccato ||
                stepDef.articulation == Harmonic::ArticulationType::Spiccato) {
                voice.attackRate = 0.04f;
                voice.releaseRate = 0.015f;
            } else {
                voice.attackRate = 0.01f;
                voice.releaseRate = 0.003f;
            }
        }
    }
}

void AuditionAudioPlayer::renderVoices(float* outL, float* outR, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        float sampleL = 0.0f;
        float sampleR = 0.0f;

        for (int v = 0; v < NUM_VOICES; ++v) {
            auto& voice = voices[v];
            if (!voice.active) continue;

            if (!voice.isReleasing) {
                voice.env += voice.attackRate;
                if (voice.env > 1.0f) voice.env = 1.0f;
                voice.remainingSamples -= 1.0;
                if (voice.remainingSamples <= 0.0) voice.isReleasing = true;
            } else {
                voice.env -= voice.releaseRate;
                if (voice.env <= 0.0f) {
                    voice.env = 0.0f;
                    voice.active = false;
                    continue;
                }
            }

            // Warm bandlimited synthesis per section
            float osc = 0.0f;
            switch (voice.section) {
                case Harmonic::OrchestralSection::Strings:
                    osc = std::sin(voice.phase) + 0.45f * std::sin(2.0f * voice.phase) + 0.2f * std::sin(3.0f * voice.phase);
                    break;
                case Harmonic::OrchestralSection::Brass:
                    osc = std::sin(voice.phase) + 0.65f * std::sin(2.0f * voice.phase) + 0.4f * std::sin(3.0f * voice.phase) + 0.2f * std::sin(4.0f * voice.phase);
                    break;
                case Harmonic::OrchestralSection::Woodwinds:
                    osc = std::sin(voice.phase) + 0.35f * std::sin(3.0f * voice.phase);
                    break;
                case Harmonic::OrchestralSection::Keyboards:
                    osc = std::sin(voice.phase) + 0.3f * std::sin(2.0f * voice.phase) + 0.15f * std::sin(4.0f * voice.phase);
                    break;
                default:
                    osc = std::sin(voice.phase) + 0.3f * std::sin(2.0f * voice.phase);
                    break;
            }

            voice.phase += voice.phaseDelta;
            if (voice.phase >= 6.2831853f) voice.phase -= 6.2831853f;

            float val = osc * voice.env * voice.velocity * 0.18f;
            sampleL += val * voice.panL;
            sampleR += val * voice.panR;
        }

        outL[i] += sampleL;
        outR[i] += sampleR;
    }
}

void AuditionAudioPlayer::audioDeviceIOCallbackWithContext(const float* const*,
                                                         int,
                                                         float* const* outputChannelData,
                                                         int numOutputChannels,
                                                         int numSamples,
                                                         const juce::AudioIODeviceCallbackContext&)
{
    float* outL = (numOutputChannels > 0) ? outputChannelData[0] : nullptr;
    float* outR = (numOutputChannels > 1) ? outputChannelData[1] : nullptr;

    if (!outL || !outR) return;

    juce::FloatVectorOperations::clear(outL, numSamples);
    juce::FloatVectorOperations::clear(outR, numSamples);

    if (!playing.load()) {
        renderVoices(outL, outR, numSamples);
        return;
    }

    double bpm = currentBpm.load();
    if (bpm < 20.0) bpm = 120.0;
    double samplesPerStep = (sampleRate * 60.0) / (bpm * 4.0); // 16th note step

    int samplesProcessed = 0;
    while (samplesProcessed < numSamples) {
        double samplesRemaining = samplesPerStep - stepSampleCounter;
        int chunk = std::min(numSamples - samplesProcessed, static_cast<int>(std::ceil(samplesRemaining)));
        if (chunk <= 0) chunk = 1;

        renderVoices(outL + samplesProcessed, outR + samplesProcessed, chunk);

        stepSampleCounter += chunk;
        samplesProcessed += chunk;

        if (stepSampleCounter >= samplesPerStep) {
            stepSampleCounter -= samplesPerStep;
            int totalSteps = (cachedStepCount > 0) ? cachedStepCount : 16;
            int nextStep = (currentStep.load() + 1) % totalSteps;
            currentStep.store(nextStep);

            if (patternMutex.try_lock()) {
                advanceStepAndTriggerNotes();
                patternMutex.unlock();
            }
        }
    }
}

// -------------------------------------------------------------
// SequencerPreviewGridComponent Implementation
// -------------------------------------------------------------
SequencerPreviewGridComponent::SequencerPreviewGridComponent() {
    setOpaque(true);
}

void SequencerPreviewGridComponent::setPattern(const Sequencer::OrchestralPattern& pat) {
    pattern = pat;
    rebuildTrackList();
    repaint();
}

void SequencerPreviewGridComponent::setAuditionChord(int rootPc, Harmonic::ChordQuality quality) {
    auditionRoot = ((rootPc % 12) + 12) % 12;
    auditionQuality = quality;
    repaint();
}

void SequencerPreviewGridComponent::setPlayheadStep(int step) {
    if (playheadStep != step) {
        playheadStep = step;
        repaint();
    }
}

void SequencerPreviewGridComponent::setSelectedStep(Harmonic::InstrumentId inst, int step) {
    selectedInst = inst;
    selectedStep = step;
    repaint();
    if (onStepSelected) onStepSelected(selectedInst, selectedStep);
}

void SequencerPreviewGridComponent::updateSelectedStep(bool active, int stepOffset, int velocity, int lengthSteps, Harmonic::ArticulationType art) {
    if (pattern.tracks.find(selectedInst) == pattern.tracks.end()) return;
    auto& trk = pattern.tracks[selectedInst];
    if (selectedStep >= 0 && selectedStep < (int)trk.steps.size()) {
        trk.steps[selectedStep].active = active;
        trk.steps[selectedStep].stepOffset = stepOffset;
        trk.steps[selectedStep].velocity = velocity;
        trk.steps[selectedStep].lengthSteps = lengthSteps;
        trk.steps[selectedStep].articulation = art;
        trk.steps[selectedStep].action = (lengthSteps > 1) ? Harmonic::StepActionType::Sustain : Harmonic::StepActionType::Ostinato;
        repaint();
        if (onPatternModified) onPatternModified();
    }
}

void SequencerPreviewGridComponent::rebuildTrackList() {
    trackList.clear();
    for (const auto& [inst, trk] : pattern.tracks) {
        trackList.push_back(inst);
    }
    if (!trackList.empty() && std::find(trackList.begin(), trackList.end(), selectedInst) == trackList.end()) {
        selectedInst = trackList.front();
        selectedStep = 0;
    }
}

std::string SequencerPreviewGridComponent::calculateNoteNameForStep(Harmonic::InstrumentId inst, const Sequencer::StepDefinition& stepDef) const {
    if (!stepDef.active) return "-";
    if (pattern.tracks.find(inst) == pattern.tracks.end()) return "-";
    const auto& trk = pattern.tracks.at(inst);

    Harmonic::HarmonicFrame frame;
    frame.rootPitchClass = auditionRoot;
    frame.quality = auditionQuality;
    frame.chordTones = Harmonic::getChordQualityIntervals(auditionQuality);
    frame.bassMidiNote = auditionRoot + 36;

    Orchestration::VoicingEngine ve;
    auto voicing = ve.generateVoicing(frame);
    int voiceBase = 60;
    for (const auto& v : voicing.voices) {
        if (v.instrument == inst) { voiceBase = v.midiPitch; break; }
    }
    int effectiveBase = voiceBase + (trk.octaveOffset * 12);

    std::vector<int> ladder;
    std::vector<int> chordPcs;
    for (int iv : frame.chordTones) chordPcs.push_back((auditionRoot + iv) % 12);
    std::sort(chordPcs.begin(), chordPcs.end());
    chordPcs.erase(std::unique(chordPcs.begin(), chordPcs.end()), chordPcs.end());
    if (chordPcs.empty()) chordPcs = {auditionRoot, (auditionRoot + 4) % 12, (auditionRoot + 7) % 12};

    for (int oct = 1; oct <= 9; ++oct) {
        for (int pc : chordPcs) {
            int p = oct * 12 + pc;
            if (p >= 12 && p <= 127) ladder.push_back(p);
        }
    }
    std::sort(ladder.begin(), ladder.end());
    ladder.erase(std::unique(ladder.begin(), ladder.end()), ladder.end());

    int baseIdx = 0;
    int minDiff = 999;
    for (size_t i = 0; i < ladder.size(); ++i) {
        int d = std::abs(ladder[i] - effectiveBase);
        if (d < minDiff) { minDiff = d; baseIdx = static_cast<int>(i); }
    }

    int targetIdx = std::clamp(baseIdx + stepDef.stepOffset, 0, (int)ladder.size() - 1);
    int pitch = ladder[targetIdx] + (stepDef.octaveOffset * 12);
    return Harmonic::noteNumberToName(pitch, true);
}

void SequencerPreviewGridComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff0e1117)); // Dark charcoal canvas

    int totalSteps = pattern.barLength * 16;
    for (const auto& [inst, trk] : pattern.tracks) {
        if ((int)trk.stepCount > totalSteps) totalSteps = trk.stepCount;
    }
    if (totalSteps <= 0) totalSteps = 16;

    const int headerWidth = 170;
    const int stepWidth = 24;
    const int rowHeight = 36;
    const int topHeaderHeight = 26;

    // 1. Top Header Row: Step Numbers & Bars
    g.setColour(juce::Colour(0xff161b22));
    g.fillRect(0, 0, getWidth(), topHeaderHeight);

    g.setColour(juce::Colour(0xff8b949e));
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    for (int s = 0; s < totalSteps; ++s) {
        int x = headerWidth + s * stepWidth;
        if (s % 16 == 0) {
            // Bar label
            int barNum = (s / 16) + 1;
            g.setColour(juce::Colour(0xff58a6ff));
            g.drawText("BAR " + juce::String(barNum), x + 2, 2, 50, 10, juce::Justification::left);
        }
        g.setColour((s % 4 == 0) ? juce::Colour(0xffedf2f7) : juce::Colour(0xff718096));
        g.drawText(juce::String(s + 1), x, 12, stepWidth, 12, juce::Justification::centred);

        // Bar divider lines
        if (s % 16 == 0) {
            g.setColour(juce::Colour(0xff30363d));
            g.drawVerticalLine(x, 0, getHeight());
        } else if (s % 4 == 0) {
            g.setColour(juce::Colour(0xff21262d));
            g.drawVerticalLine(x, 0, getHeight());
        }
    }

    // 2. Track Rows
    int y = topHeaderHeight;
    for (Harmonic::InstrumentId inst : trackList) {
        if (pattern.tracks.find(inst) == pattern.tracks.end()) continue;
        const auto& trk = pattern.tracks.at(inst);

        bool isTrackSelected = (inst == selectedInst);

        // Row background
        g.setColour(isTrackSelected ? juce::Colour(0xff1f242c) : juce::Colour(0xff12161f));
        g.fillRect(0, y, getWidth(), rowHeight);
        g.setColour(juce::Colour(0xff21262d));
        g.drawHorizontalLine(y + rowHeight - 1, 0, getWidth());

        // Header Background
        g.setColour(juce::Colour(0xff161b22));
        g.fillRect(0, y, headerWidth, rowHeight);
        g.setColour(juce::Colour(0xff30363d));
        g.drawVerticalLine(headerWidth, y, y + rowHeight);

        // Section badge
        auto sec = trk.section;
        juce::Colour secCol = getSectionColour(sec);
        g.setColour(secCol);
        g.fillRect(4, y + 4, 32, rowHeight - 8);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText(getSectionShortName(sec), 4, y + 4, 32, rowHeight - 8, juce::Justification::centred);

        // Track Name
        g.setColour(juce::Colour(0xfff0f6fc));
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(trk.trackName, 42, y + 4, 85, rowHeight - 8, juce::Justification::centredLeft, true);

        // Mute / Solo badges
        g.setColour(trk.isMuted ? juce::Colour(0xffe53e3e) : juce::Colour(0xff21262d));
        g.fillRect(130, y + 8, 16, 20);
        g.setColour(trk.isMuted ? juce::Colours::white : juce::Colour(0xff8b949e));
        g.setFont(juce::Font(9.5f, juce::Font::bold));
        g.drawText("M", 130, y + 8, 16, 20, juce::Justification::centred);

        g.setColour(trk.isSolo ? juce::Colour(0xffd69e2e) : juce::Colour(0xff21262d));
        g.fillRect(148, y + 8, 16, 20);
        g.setColour(trk.isSolo ? juce::Colours::black : juce::Colour(0xff8b949e));
        g.drawText("S", 148, y + 8, 16, 20, juce::Justification::centred);

        // 1. Draw cell backgrounds and grid lines
        for (int s = 0; s < totalSteps; ++s) {
            int x = headerWidth + s * stepWidth;
            bool isCellSelected = (isTrackSelected && s == selectedStep);

            // Subtle cell border
            g.setColour(juce::Colour(0xff1a1f26));
            g.drawRect(x, y + 2, stepWidth, rowHeight - 4, 1);

            if (isCellSelected) {
                g.setColour(juce::Colour(0xff4a5568));
                g.drawRect(x + 1, y + 3, stepWidth - 2, rowHeight - 6, 1);
            }
        }

        // 2. Draw active note blocks on top (supporting multi-step durations)
        for (int s = 0; s < totalSteps; ++s) {
            if (s >= (int)trk.steps.size()) break;
            const auto& sd = trk.steps[s];
            if (!sd.active || sd.action == Harmonic::StepActionType::Rest) continue;

            int x = headerWidth + s * stepWidth;
            bool isCellSelected = (isTrackSelected && s == selectedStep);
            int len = std::clamp(sd.lengthSteps, 1, std::max(1, totalSteps - s));
            int blockW = (len * stepWidth) - 2;

            // Active step fill
            g.setColour(secCol.withAlpha(0.88f));
            g.fillRoundedRectangle((float)x + 1.0f, (float)y + 3.0f, (float)blockW, (float)rowHeight - 6.0f, 4.0f);

            // Selection highlight
            if (isCellSelected) {
                g.setColour(juce::Colours::yellow);
                g.drawRoundedRectangle((float)x + 1.0f, (float)y + 3.0f, (float)blockW, (float)rowHeight - 6.0f, 4.0f, 2.0f);
            } else {
                g.setColour(secCol.brighter(0.45f));
                g.drawRoundedRectangle((float)x + 1.0f, (float)y + 3.0f, (float)blockW, (float)rowHeight - 6.0f, 4.0f, 1.0f);
            }

            // Right-edge resize handle (visual cue for interactive dragging)
            int handleX = x + blockW - 6;
            g.setColour(juce::Colours::white.withAlpha(0.65f));
            g.fillRect(handleX, y + 9, 2, rowHeight - 18);
            g.fillRect(handleX + 3, y + 9, 2, rowHeight - 18);

            // Note name and degree offset / duration
            std::string noteName = calculateNoteNameForStep(inst, sd);
            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(9.0f, juce::Font::bold));
            g.drawText(noteName, x + 3, y + 5, blockW - 10, 12, juce::Justification::left);

            juce::String degStr = (sd.stepOffset >= 0 ? "+" : "") + juce::String(sd.stepOffset);
            if (len > 1) {
                degStr += " (" + formatStepDuration(len) + ")";
            }
            g.setFont(juce::Font(8.0f));
            g.setColour(juce::Colour(0xffedf2f7));
            g.drawText(degStr, x + 3, y + 17, blockW - 10, 10, juce::Justification::left);

            // Velocity bar at bottom
            float velNorm = sd.velocity / 127.0f;
            g.setColour(juce::Colour(0xffffffff).withAlpha(0.7f));
            g.fillRect(x + 3, y + rowHeight - 6, static_cast<int>((blockW - 6) * velNorm), 2);
        }

        y += rowHeight;
    }

    // 3. Playhead cursor
    if (playheadStep >= 0 && playheadStep < totalSteps) {
        int px = headerWidth + playheadStep * stepWidth;
        g.setColour(juce::Colour(0xff00d2ff)); // Neon Cyan
        g.drawVerticalLine(px + (stepWidth / 2), 0, getHeight());

        juce::Path p;
        p.addTriangle(px + 4.0f, 0.0f, px + stepWidth - 4.0f, 0.0f, px + (stepWidth / 2.0f), 8.0f);
        g.fillPath(p);
    }
}

void SequencerPreviewGridComponent::mouseMove(const juce::MouseEvent& e) {
    const int headerWidth = 170;
    const int stepWidth = 24;
    const int rowHeight = 36;
    const int topHeaderHeight = 26;

    int totalSteps = pattern.barLength * 16;
    for (const auto& [inst, trk] : pattern.tracks) {
        if ((int)trk.stepCount > totalSteps) totalSteps = trk.stepCount;
    }
    if (totalSteps <= 0) totalSteps = 16;

    if (e.y < topHeaderHeight || e.x < headerWidth) {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    int trackIdx = (e.y - topHeaderHeight) / rowHeight;
    if (trackIdx < 0 || trackIdx >= (int)trackList.size()) {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    Harmonic::InstrumentId inst = trackList[trackIdx];
    if (pattern.tracks.find(inst) == pattern.tracks.end()) {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    const auto& trk = pattern.tracks.at(inst);
    bool nearHandle = false;
    for (int s = 0; s < (int)trk.steps.size(); ++s) {
        if (trk.steps[s].active && trk.steps[s].action != Harmonic::StepActionType::Rest) {
            int len = std::clamp(trk.steps[s].lengthSteps, 1, std::max(1, totalSteps - s));
            int blockRight = headerWidth + s * stepWidth + (len * stepWidth) - 2;
            if (std::abs(e.x - blockRight) <= 6) {
                nearHandle = true;
                break;
            }
        }
    }

    if (nearHandle) {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    } else {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void SequencerPreviewGridComponent::mouseDown(const juce::MouseEvent& e) {
    const int headerWidth = 170;
    const int stepWidth = 24;
    const int rowHeight = 36;
    const int topHeaderHeight = 26;

    int totalSteps = pattern.barLength * 16;
    for (const auto& [inst, trk] : pattern.tracks) {
        if ((int)trk.stepCount > totalSteps) totalSteps = trk.stepCount;
    }
    if (totalSteps <= 0) totalSteps = 16;

    int mx = e.x;
    int my = e.y;

    if (my < topHeaderHeight) return;

    int trackIdx = (my - topHeaderHeight) / rowHeight;
    if (trackIdx < 0 || trackIdx >= (int)trackList.size()) return;

    Harmonic::InstrumentId inst = trackList[trackIdx];
    auto& trk = pattern.tracks[inst];

    // Clicked in header area (Mute / Solo toggle)
    if (mx < headerWidth) {
        if (mx >= 130 && mx < 146) {
            trk.isMuted = !trk.isMuted;
            repaint();
            if (onPatternModified) onPatternModified();
            return;
        } else if (mx >= 148 && mx < 164) {
            trk.isSolo = !trk.isSolo;
            repaint();
            if (onPatternModified) onPatternModified();
            return;
        }
        setSelectedStep(inst, selectedStep);
        return;
    }

    // Check if clicked near right edge of an active note (resize handle)
    if (e.mods.isLeftButtonDown()) {
        for (int s = 0; s < (int)trk.steps.size(); ++s) {
            if (trk.steps[s].active && trk.steps[s].action != Harmonic::StepActionType::Rest) {
                int len = std::clamp(trk.steps[s].lengthSteps, 1, std::max(1, totalSteps - s));
                int blockRight = headerWidth + s * stepWidth + (len * stepWidth) - 2;
                if (std::abs(mx - blockRight) <= 6) {
                    isResizingDuration = true;
                    resizingInst = inst;
                    resizingStep = s;
                    originalLength = len;
                    dragStartX = mx;
                    setSelectedStep(inst, s);
                    return;
                }
            }
        }
    }

    // Clicked in step grid
    int step = (mx - headerWidth) / stepWidth;
    if (step < 0 || step >= totalSteps) return;

    if (step >= (int)trk.steps.size()) {
        trk.steps.resize(totalSteps);
    }

    // Check if clicked inside the duration of a sustained note
    int containingStep = -1;
    for (int s = 0; s < (int)trk.steps.size(); ++s) {
        if (trk.steps[s].active && trk.steps[s].action != Harmonic::StepActionType::Rest) {
            int len = std::clamp(trk.steps[s].lengthSteps, 1, std::max(1, totalSteps - s));
            if (step >= s && step < s + len) {
                containingStep = s;
                break;
            }
        }
    }

    if (containingStep >= 0 && containingStep != step && !e.mods.isRightButtonDown()) {
        setSelectedStep(inst, containingStep);
        return;
    }

    if (e.mods.isRightButtonDown()) {
        // Right click: toggle active
        trk.steps[step].active = !trk.steps[step].active;
    } else {
        // Left click: if inactive -> activate; if active and already selected -> toggle off
        if (!trk.steps[step].active) {
            trk.steps[step].active = true;
            trk.steps[step].velocity = 95;
            trk.steps[step].lengthSteps = 1;
        } else if (selectedInst == inst && selectedStep == step) {
            trk.steps[step].active = false;
        }
    }

    setSelectedStep(inst, step);
    if (onPatternModified) onPatternModified();
}

void SequencerPreviewGridComponent::mouseDrag(const juce::MouseEvent& e) {
    if (isResizingDuration && pattern.tracks.find(resizingInst) != pattern.tracks.end()) {
        auto& trk = pattern.tracks[resizingInst];
        if (resizingStep >= 0 && resizingStep < (int)trk.steps.size()) {
            int totalSteps = pattern.barLength * 16;
            for (const auto& [i, t] : pattern.tracks) {
                if ((int)t.stepCount > totalSteps) totalSteps = t.stepCount;
            }
            const int stepWidth = 24;
            int deltaSteps = (e.x - dragStartX) / stepWidth;
            int maxAllowed = std::max(1, totalSteps - resizingStep);
            int newLen = std::clamp(originalLength + deltaSteps, 1, maxAllowed);
            if (trk.steps[resizingStep].lengthSteps != newLen) {
                trk.steps[resizingStep].lengthSteps = newLen;
                trk.steps[resizingStep].action = (newLen > 1) ? Harmonic::StepActionType::Sustain : Harmonic::StepActionType::Ostinato;
                repaint();
                if (onStepSelected) onStepSelected(resizingInst, resizingStep);
                if (onPatternModified) onPatternModified();
            }
        }
    }
}

void SequencerPreviewGridComponent::mouseUp(const juce::MouseEvent& /*e*/) {
    if (isResizingDuration) {
        isResizingDuration = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
        if (onPatternModified) onPatternModified();
    }
}

void SequencerPreviewGridComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    const int headerWidth = 170;
    const int stepWidth = 24;
    const int rowHeight = 36;
    const int topHeaderHeight = 26;

    if (e.y < topHeaderHeight || e.x < headerWidth) return;
    int trackIdx = (e.y - topHeaderHeight) / rowHeight;
    if (trackIdx < 0 || trackIdx >= (int)trackList.size()) return;

    Harmonic::InstrumentId inst = trackList[trackIdx];
    auto& trk = pattern.tracks[inst];
    int step = (e.x - headerWidth) / stepWidth;
    if (step < 0 || step >= (int)trk.steps.size()) return;

    if (trk.steps[step].active) {
        int delta = (wheel.deltaY > 0) ? 1 : -1;
        if (e.mods.isShiftDown()) {
            // Shift + Mouse Wheel: adjust note duration (lengthSteps)
            int totalSteps = pattern.barLength * 16;
            int maxLen = std::max(1, totalSteps - step);
            trk.steps[step].lengthSteps = std::clamp(trk.steps[step].lengthSteps + delta, 1, maxLen);
            trk.steps[step].action = (trk.steps[step].lengthSteps > 1) ? Harmonic::StepActionType::Sustain : Harmonic::StepActionType::Ostinato;
        } else {
            // Normal Mouse Wheel: adjust pitch step offset (-8 to +9)
            trk.steps[step].stepOffset = std::clamp(trk.steps[step].stepOffset + delta, -8, 9);
        }
        setSelectedStep(inst, step);
        if (onPatternModified) onPatternModified();
    }
}

// -------------------------------------------------------------
// ConverterWizardComponent Implementation
// -------------------------------------------------------------
ConverterWizardComponent::ConverterWizardComponent() {
    setSize(980, 660);

    // Navigation Buttons
    backBtn.onClick = [this]() {
        if (currentStep == WizardStep::TonalAnalysis) setStep(WizardStep::FileLoad);
        else if (currentStep == WizardStep::TrackMapping) setStep(WizardStep::TonalAnalysis);
        else if (currentStep == WizardStep::SequencerAudition) setStep(WizardStep::TrackMapping);
        else if (currentStep == WizardStep::SaveExport) setStep(WizardStep::SequencerAudition);
    };
    backBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3748));
    addAndMakeVisible(backBtn);

    nextBtn.onClick = [this]() {
        if (currentStep == WizardStep::FileLoad) {
            if (hasFileLoaded) setStep(WizardStep::TonalAnalysis);
        } else if (currentStep == WizardStep::TonalAnalysis) {
            setStep(WizardStep::TrackMapping);
        } else if (currentStep == WizardStep::TrackMapping) {
            setStep(WizardStep::SequencerAudition); // -> Go to new Audition step!
        } else if (currentStep == WizardStep::SequencerAudition) {
            setStep(WizardStep::SaveExport); // -> Go to Save step!
        }
    };
    nextBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3182ce));
    addAndMakeVisible(nextBtn);

    // Step 1: File Load
    step1Title.setFont(juce::Font(18.0f, juce::Font::bold));
    step1Title.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    addAndMakeVisible(step1Title);

    browseBtn.onClick = [this]() {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Select Orchestral MIDI File", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.mid;*.midi");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc) {
                auto file = fc.getResult();
                if (file.existsAsFile()) {
                    loadMidiFile(file);
                }
            });
    };
    browseBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3182ce)); // Blue
    addAndMakeVisible(browseBtn);

    browsePresetBtn.onClick = [this]() {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Select Orchestrator Preset (.json)", getPresetsFolder(), "*.json");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc) {
                auto file = fc.getResult();
                if (file.existsAsFile()) {
                    loadPresetFile(file);
                }
            });
    };
    browsePresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff6b46c1)); // Purple Accent
    addAndMakeVisible(browsePresetBtn);

    fileInfoLabel.setFont(juce::Font(13.0f));
    fileInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa0aec0));
    fileInfoLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(fileInfoLabel);

    // Step 2: Tonal & Harmonic Analysis
    step2Title.setFont(juce::Font(18.0f, juce::Font::bold));
    step2Title.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    addChildComponent(step2Title);

    detectedKeyBadge.setFont(juce::Font(28.0f, juce::Font::bold));
    detectedKeyBadge.setJustificationType(juce::Justification::centred);
    detectedKeyBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff2b6cb0).withAlpha(0.2f));
    detectedKeyBadge.setColour(juce::Label::textColourId, juce::Colour(0xff63b3ed));
    detectedKeyBadge.setColour(juce::Label::outlineColourId, juce::Colour(0xff3182ce));
    addChildComponent(detectedKeyBadge);

    confidenceLabel.setFont(juce::Font(13.0f));
    confidenceLabel.setJustificationType(juce::Justification::centred);
    confidenceLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa0aec0));
    addChildComponent(confidenceLabel);

    rootSelectorLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    rootSelectorLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe2e8f0));
    addChildComponent(rootSelectorLabel);

    static const char* PITCH_NAMES[12] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
    for (int i = 0; i < 12; ++i) rootSelector.addItem(PITCH_NAMES[i], i + 1);
    addChildComponent(rootSelector);

    modeSelectorLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    modeSelectorLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe2e8f0));
    addChildComponent(modeSelectorLabel);

    modeSelector.addItem("Ionian (Major)", 1);
    modeSelector.addItem("Dorian", 2);
    modeSelector.addItem("Phrygian", 3);
    modeSelector.addItem("Lydian", 4);
    modeSelector.addItem("Mixolydian", 5);
    modeSelector.addItem("Aeolian (Minor)", 6);
    modeSelector.addItem("Locrian", 7);
    modeSelector.addItem("Harmonic Minor", 8);
    modeSelector.addItem("Melodic Minor", 9);
    addChildComponent(modeSelector);

    // Step 3: Track & Instrument Mapping
    step3Title.setFont(juce::Font(18.0f, juce::Font::bold));
    step3Title.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    addChildComponent(step3Title);

    trackListViewport.setViewedComponent(&trackListContainer, false);
    trackListViewport.setScrollBarsShown(true, false);
    addChildComponent(trackListViewport);

    // Step 4: Sequencer Preview & Audition (NEW STEP!)
    step4Title.setFont(juce::Font(17.0f, juce::Font::bold));
    step4Title.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    addChildComponent(step4Title);

    auditionPlayBtn.onClick = [this]() {
        if (audioPlayer.isPlaying()) {
            audioPlayer.stopPlayback();
            auditionPlayBtn.setButtonText("▶ PLAY AUDITION");
            auditionPlayBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2f855a)); // Green
            previewGrid.setPlayheadStep(-1);
        } else {
            audioPlayer.startPlayback();
            auditionPlayBtn.setButtonText("■ STOP");
            auditionPlayBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe53e3e)); // Crimson
        }
    };
    auditionPlayBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2f855a));
    addChildComponent(auditionPlayBtn);

    auditionBpmLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    auditionBpmLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa0aec0));
    addChildComponent(auditionBpmLabel);

    auditionBpmSlider.setRange(40.0, 240.0, 1.0);
    auditionBpmSlider.setValue(120.0, juce::dontSendNotification);
    auditionBpmSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 45, 20);
    auditionBpmSlider.onValueChange = [this]() {
        audioPlayer.setBpm(auditionBpmSlider.getValue());
    };
    addChildComponent(auditionBpmSlider);

    // Multi-Bar Controls in Step 4
    barLengthLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    barLengthLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa0aec0));
    addChildComponent(barLengthLabel);

    barLengthSelector.addItem("1 Bar (16 st)", 1);
    barLengthSelector.addItem("2 Bars (32 st)", 2);
    barLengthSelector.addItem("3 Bars (48 st)", 3);
    barLengthSelector.addItem("4 Bars (64 st)", 4);
    barLengthSelector.addItem("6 Bars (96 st)", 6);
    barLengthSelector.addItem("8 Bars (128 st)", 8);
    barLengthSelector.addItem("16 Bars (256 st)", 16);
    barLengthSelector.setSelectedId(2, juce::dontSendNotification);
    barLengthSelector.onChange = [this]() {
        setPatternBarLength(barLengthSelector.getSelectedId());
    };
    addChildComponent(barLengthSelector);

    btnRemoveBar.onClick = [this]() {
        if (currentPreviewPattern.barLength > 1) {
            setPatternBarLength(currentPreviewPattern.barLength - 1);
            barLengthSelector.setSelectedId(currentPreviewPattern.barLength, juce::dontSendNotification);
        }
    };
    btnRemoveBar.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3748));
    addChildComponent(btnRemoveBar);

    btnAddBar.onClick = [this]() {
        setPatternBarLength(currentPreviewPattern.barLength + 1);
        barLengthSelector.setSelectedId(currentPreviewPattern.barLength, juce::dontSendNotification);
    };
    btnAddBar.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b6cb0));
    addChildComponent(btnAddBar);

    btnDuplicateBar1ToAll.onClick = [this]() {
        auto& pat = previewGrid.getPatternRef();
        for (auto& [inst, trk] : pat.tracks) {
            if (trk.steps.size() >= 16) {
                for (int s = 16; s < (int)trk.steps.size(); ++s) {
                    trk.steps[s] = trk.steps[s % 16];
                }
            }
        }
        previewGrid.repaint();
        currentPreviewPattern = previewGrid.getPattern();
        audioPlayer.setPattern(currentPreviewPattern);
        updateInspectorForSelectedStep();
    };
    btnDuplicateBar1ToAll.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a5568));
    addChildComponent(btnDuplicateBar1ToAll);

    btnLoadPresetInStep4.onClick = [this]() {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Load Orchestrator Preset (.json)", getPresetsFolder(), "*.json");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc) {
                auto file = fc.getResult();
                if (file.existsAsFile()) {
                    loadPresetFile(file);
                }
            });
    };
    btnLoadPresetInStep4.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff6b46c1)); // Purple Accent
    addChildComponent(btnLoadPresetInStep4);

    btnQuickSaveInStep4.onClick = [this]() {
        savePreset(false);
    };
    btnQuickSaveInStep4.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2f855a)); // Green Accent
    addChildComponent(btnQuickSaveInStep4);

    auditionChordLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    auditionChordLabel.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    addChildComponent(auditionChordLabel);

    for (int i = 0; i < 12; ++i) auditionRootSelector.addItem(PITCH_NAMES[i], i + 1);
    auditionRootSelector.setSelectedId(3, juce::dontSendNotification); // D default
    auditionRootSelector.onChange = [this]() {
        int rootPc = auditionRootSelector.getSelectedId() - 1;
        auto quality = static_cast<Harmonic::ChordQuality>(auditionQualitySelector.getSelectedId());
        audioPlayer.updateAuditionChord(rootPc, quality);
        previewGrid.setAuditionChord(rootPc, quality);
        updateInspectorForSelectedStep();
    };
    addChildComponent(auditionRootSelector);

    auditionQualitySelector.addItem("Major Triad", static_cast<int>(Harmonic::ChordQuality::MajorTriad));
    auditionQualitySelector.addItem("Minor Triad", static_cast<int>(Harmonic::ChordQuality::MinorTriad));
    auditionQualitySelector.addItem("Dominant 7", static_cast<int>(Harmonic::ChordQuality::Dominant7));
    auditionQualitySelector.addItem("Major 7", static_cast<int>(Harmonic::ChordQuality::Major7));
    auditionQualitySelector.addItem("Minor 7", static_cast<int>(Harmonic::ChordQuality::Minor7));
    auditionQualitySelector.addItem("Diminished", static_cast<int>(Harmonic::ChordQuality::DiminishedTriad));
    auditionQualitySelector.addItem("Sus4", static_cast<int>(Harmonic::ChordQuality::Sus4));
    auditionQualitySelector.addItem("Major 9", static_cast<int>(Harmonic::ChordQuality::Major9));
    auditionQualitySelector.addItem("Minor 9", static_cast<int>(Harmonic::ChordQuality::Minor9));
    auditionQualitySelector.setSelectedId(static_cast<int>(Harmonic::ChordQuality::Major7), juce::dontSendNotification);
    auditionQualitySelector.onChange = [this]() {
        int rootPc = auditionRootSelector.getSelectedId() - 1;
        auto quality = static_cast<Harmonic::ChordQuality>(auditionQualitySelector.getSelectedId());
        audioPlayer.updateAuditionChord(rootPc, quality);
        previewGrid.setAuditionChord(rootPc, quality);
        updateInspectorForSelectedStep();
    };
    addChildComponent(auditionQualitySelector);

    // Quick chord presets
    auto setupQuickChord = [this](juce::TextButton& btn, int rootPc, Harmonic::ChordQuality q) {
        btn.onClick = [this, rootPc, q]() {
            auditionRootSelector.setSelectedId(rootPc + 1, juce::dontSendNotification);
            auditionQualitySelector.setSelectedId(static_cast<int>(q), juce::dontSendNotification);
            audioPlayer.updateAuditionChord(rootPc, q);
            previewGrid.setAuditionChord(rootPc, q);
            updateInspectorForSelectedStep();
        };
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3748));
        addChildComponent(btn);
    };
    setupQuickChord(btnChordC, 0, Harmonic::ChordQuality::MajorTriad);
    setupQuickChord(btnChordDm, 2, Harmonic::ChordQuality::MinorTriad);
    setupQuickChord(btnChordG7, 7, Harmonic::ChordQuality::Dominant7);
    setupQuickChord(btnChordEm, 4, Harmonic::ChordQuality::MinorTriad);
    setupQuickChord(btnChordF, 5, Harmonic::ChordQuality::MajorTriad);
    setupQuickChord(btnChordAm, 9, Harmonic::ChordQuality::MinorTriad);

    btnChordOrig.onClick = [this]() {
        auditionRootSelector.setSelectedId(tonalResult.detectedRootPitchClass + 1, juce::dontSendNotification);
        auditionQualitySelector.setSelectedId(static_cast<int>(tonalResult.detectedChordQuality), juce::dontSendNotification);
        audioPlayer.updateAuditionChord(tonalResult.detectedRootPitchClass, tonalResult.detectedChordQuality);
        previewGrid.setAuditionChord(tonalResult.detectedRootPitchClass, tonalResult.detectedChordQuality);
        updateInspectorForSelectedStep();
    };
    btnChordOrig.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a5568));
    addChildComponent(btnChordOrig);

    // Multi-track Grid Viewport
    gridViewport.setViewedComponent(&previewGrid, false);
    gridViewport.setScrollBarsShown(true, true);
    addChildComponent(gridViewport);

    previewGrid.onStepSelected = [this](Harmonic::InstrumentId, int) {
        updateInspectorForSelectedStep();
    };
    previewGrid.onPatternModified = [this]() {
        currentPreviewPattern = previewGrid.getPattern();
        audioPlayer.setPattern(currentPreviewPattern);
        updateInspectorForSelectedStep();
    };

    // Step Detail Inspector
    inspectorTitle.setFont(juce::Font(11.0f, juce::Font::bold));
    inspectorTitle.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    addChildComponent(inspectorTitle);

    inspectorTrackStepLabel.setFont(juce::Font(11.0f));
    inspectorTrackStepLabel.setColour(juce::Label::textColourId, juce::Colour(0xff63b3ed));
    addChildComponent(inspectorTrackStepLabel);

    inspectorActiveToggle.onClick = [this]() {
        auto inst = previewGrid.getSelectedInstrument();
        int step = previewGrid.getSelectedStep();
        if (previewGrid.getPattern().tracks.find(inst) != previewGrid.getPattern().tracks.end()) {
            const auto& sd = previewGrid.getPattern().tracks.at(inst).steps[step];
            previewGrid.updateSelectedStep(inspectorActiveToggle.getToggleState(), sd.stepOffset, sd.velocity, sd.lengthSteps, sd.articulation);
        }
    };
    addChildComponent(inspectorActiveToggle);

    inspectorOffsetLabel.setFont(juce::Font(10.5f));
    inspectorOffsetLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa0aec0));
    addChildComponent(inspectorOffsetLabel);

    inspectorOffsetSlider.setRange(-8, 9, 1);
    inspectorOffsetSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 35, 18);
    inspectorOffsetSlider.onValueChange = [this]() {
        auto inst = previewGrid.getSelectedInstrument();
        int step = previewGrid.getSelectedStep();
        if (previewGrid.getPattern().tracks.find(inst) != previewGrid.getPattern().tracks.end()) {
            const auto& sd = previewGrid.getPattern().tracks.at(inst).steps[step];
            previewGrid.updateSelectedStep(sd.active, static_cast<int>(inspectorOffsetSlider.getValue()), sd.velocity, sd.lengthSteps, sd.articulation);
        }
    };
    addChildComponent(inspectorOffsetSlider);

    inspectorPitchReadout.setFont(juce::Font(11.0f, juce::Font::bold));
    inspectorPitchReadout.setColour(juce::Label::textColourId, juce::Colour(0xfff6e05e)); // Gold
    addChildComponent(inspectorPitchReadout);

    inspectorVelocityLabel.setFont(juce::Font(10.5f));
    inspectorVelocityLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa0aec0));
    addChildComponent(inspectorVelocityLabel);

    inspectorVelocitySlider.setRange(1, 127, 1);
    inspectorVelocitySlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 35, 18);
    inspectorVelocitySlider.onValueChange = [this]() {
        auto inst = previewGrid.getSelectedInstrument();
        int step = previewGrid.getSelectedStep();
        if (previewGrid.getPattern().tracks.find(inst) != previewGrid.getPattern().tracks.end()) {
            const auto& sd = previewGrid.getPattern().tracks.at(inst).steps[step];
            previewGrid.updateSelectedStep(sd.active, sd.stepOffset, static_cast<int>(inspectorVelocitySlider.getValue()), sd.lengthSteps, sd.articulation);
        }
    };
    addChildComponent(inspectorVelocitySlider);

    inspectorLengthLabel.setFont(juce::Font(10.5f));
    inspectorLengthLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa0aec0));
    addChildComponent(inspectorLengthLabel);

    inspectorLengthSlider.setRange(1, 64, 1);
    inspectorLengthSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 35, 18);
    inspectorLengthSlider.onValueChange = [this]() {
        auto inst = previewGrid.getSelectedInstrument();
        int step = previewGrid.getSelectedStep();
        if (previewGrid.getPattern().tracks.find(inst) != previewGrid.getPattern().tracks.end()) {
            const auto& sd = previewGrid.getPattern().tracks.at(inst).steps[step];
            int len = static_cast<int>(inspectorLengthSlider.getValue());
            previewGrid.updateSelectedStep(sd.active, sd.stepOffset, sd.velocity, len, sd.articulation);
            inspectorLengthLabel.setText("Length: " + juce::String(len) + " (" + formatStepDuration(len) + ")", juce::dontSendNotification);
        }
    };
    addChildComponent(inspectorLengthSlider);

    // Quick Duration Buttons
    auto setQuickDuration = [this](int len) {
        auto inst = previewGrid.getSelectedInstrument();
        int step = previewGrid.getSelectedStep();
        if (previewGrid.getPattern().tracks.find(inst) != previewGrid.getPattern().tracks.end()) {
            const auto& sd = previewGrid.getPattern().tracks.at(inst).steps[step];
            previewGrid.updateSelectedStep(sd.active, sd.stepOffset, sd.velocity, len, sd.articulation);
            inspectorLengthSlider.setValue(len, juce::dontSendNotification);
            inspectorLengthLabel.setText("Length: " + juce::String(len) + " (" + formatStepDuration(len) + ")", juce::dontSendNotification);
        }
    };
    auto setupDurBtn = [this](juce::TextButton& btn, std::function<void()> cb) {
        btn.onClick = cb;
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3748));
        addChildComponent(btn);
    };
    setupDurBtn(btnDur16th, [setQuickDuration]() { setQuickDuration(1); });
    setupDurBtn(btnDur8th, [setQuickDuration]() { setQuickDuration(2); });
    setupDurBtn(btnDurQuarter, [setQuickDuration]() { setQuickDuration(4); });
    setupDurBtn(btnDurHalf, [setQuickDuration]() { setQuickDuration(8); });
    setupDurBtn(btnDur1Bar, [setQuickDuration]() { setQuickDuration(16); });
    setupDurBtn(btnDur2Bars, [setQuickDuration]() { setQuickDuration(32); });

    inspectorArtLabel.setFont(juce::Font(10.5f));
    inspectorArtLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa0aec0));
    addChildComponent(inspectorArtLabel);

    inspectorArtSelector.addItem("Sustain", 1);
    inspectorArtSelector.addItem("Staccato", 2);
    inspectorArtSelector.addItem("Spiccato", 3);
    inspectorArtSelector.addItem("Marcato", 4);
    inspectorArtSelector.onChange = [this]() {
        auto inst = previewGrid.getSelectedInstrument();
        int step = previewGrid.getSelectedStep();
        if (previewGrid.getPattern().tracks.find(inst) != previewGrid.getPattern().tracks.end()) {
            const auto& sd = previewGrid.getPattern().tracks.at(inst).steps[step];
            Harmonic::ArticulationType art = Harmonic::ArticulationType::Sustain;
            if (inspectorArtSelector.getSelectedId() == 2) art = Harmonic::ArticulationType::Staccato;
            else if (inspectorArtSelector.getSelectedId() == 3) art = Harmonic::ArticulationType::Spiccato;
            else if (inspectorArtSelector.getSelectedId() == 4) art = Harmonic::ArticulationType::Marcato;
            previewGrid.updateSelectedStep(sd.active, sd.stepOffset, sd.velocity, sd.lengthSteps, art);
        }
    };
    addChildComponent(inspectorArtSelector);

    btnOctaveUp.onClick = [this]() {
        auto inst = previewGrid.getSelectedInstrument();
        int step = previewGrid.getSelectedStep();
        if (previewGrid.getPattern().tracks.find(inst) != previewGrid.getPattern().tracks.end()) {
            auto& trk = previewGrid.getPatternRef().tracks[inst];
            trk.steps[step].octaveOffset = std::clamp(trk.steps[step].octaveOffset + 1, -2, 2);
            previewGrid.repaint();
            currentPreviewPattern = previewGrid.getPattern();
            audioPlayer.setPattern(currentPreviewPattern);
            updateInspectorForSelectedStep();
        }
    };
    btnOctaveUp.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3748));
    addChildComponent(btnOctaveUp);

    btnOctaveDown.onClick = [this]() {
        auto inst = previewGrid.getSelectedInstrument();
        int step = previewGrid.getSelectedStep();
        if (previewGrid.getPattern().tracks.find(inst) != previewGrid.getPattern().tracks.end()) {
            auto& trk = previewGrid.getPatternRef().tracks[inst];
            trk.steps[step].octaveOffset = std::clamp(trk.steps[step].octaveOffset - 1, -2, 2);
            previewGrid.repaint();
            currentPreviewPattern = previewGrid.getPattern();
            audioPlayer.setPattern(currentPreviewPattern);
            updateInspectorForSelectedStep();
        }
    };
    btnOctaveDown.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3748));
    addChildComponent(btnOctaveDown);

    btnDuplicateBar.onClick = [this]() {
        auto& pat = previewGrid.getPatternRef();
        for (auto& [inst, trk] : pat.tracks) {
            if (trk.steps.size() >= 32) {
                for (int s = 0; s < 16; ++s) {
                    trk.steps[s + 16] = trk.steps[s];
                }
            }
        }
        previewGrid.repaint();
        currentPreviewPattern = previewGrid.getPattern();
        audioPlayer.setPattern(currentPreviewPattern);
        updateInspectorForSelectedStep();
    };
    btnDuplicateBar.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a5568));
    addChildComponent(btnDuplicateBar);

    // Step 5: Save & Export
    step5Title.setFont(juce::Font(18.0f, juce::Font::bold));
    step5Title.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    addChildComponent(step5Title);

    presetNameLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    presetNameLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe2e8f0));
    addChildComponent(presetNameLabel);

    presetNameEditor.setFont(juce::Font(13.0f));
    addChildComponent(presetNameEditor);

    bpmLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    bpmLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe2e8f0));
    addChildComponent(bpmLabel);

    bpmEditor.setFont(juce::Font(13.0f));
    addChildComponent(bpmEditor);

    stepsLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    stepsLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe2e8f0));
    addChildComponent(stepsLabel);

    stepsSelector.addItem("1 Bar (16 steps)", 16);
    stepsSelector.addItem("2 Bars (32 steps)", 32);
    stepsSelector.addItem("3 Bars (48 steps)", 48);
    stepsSelector.addItem("4 Bars (64 steps)", 64);
    stepsSelector.addItem("6 Bars (96 steps)", 96);
    stepsSelector.addItem("8 Bars (128 steps)", 128);
    stepsSelector.addItem("16 Bars (256 steps)", 256);
    stepsSelector.setSelectedId(32, juce::dontSendNotification);
    addChildComponent(stepsSelector);

    saveDirectBtn.onClick = [this]() { savePreset(false); };
    saveDirectBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff38a169)); // Green
    addChildComponent(saveDirectBtn);

    saveAsBtn.onClick = [this]() { savePreset(true); };
    saveAsBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a5568));
    addChildComponent(saveAsBtn);

    copyJsonBtn.onClick = [this]() {
        currentPreviewPattern.name = presetNameEditor.getText().toStdString();
        currentPreviewPattern.bpm = bpmEditor.getText().getDoubleValue();
        std::string jsonStr = currentPreviewPattern.toJson();
        juce::SystemClipboard::copyTextToClipboard(jsonStr);
        statusLabel.setText("JSON copied to clipboard!", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff48bb78));
    };
    copyJsonBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3748));
    addChildComponent(copyJsonBtn);

    statusLabel.setFont(juce::Font(12.5f, juce::Font::bold));
    statusLabel.setJustificationType(juce::Justification::centred);
    addChildComponent(statusLabel);

    updateVisibilityForStep();
}

ConverterWizardComponent::~ConverterWizardComponent() {
    stopTimer();
    audioPlayer.stopPlayback();
}

void ConverterWizardComponent::timerCallback() {
    if (audioPlayer.isPlaying()) {
        int currentStepPlaying = audioPlayer.getCurrentStep();
        previewGrid.setPlayheadStep(currentStepPlaying);

        // Auto-scroll viewport if playhead moves out of view
        const int headerWidth = 170;
        const int stepWidth = 24;
        int playheadX = headerWidth + currentStepPlaying * stepWidth;
        int viewX = gridViewport.getViewPositionX();
        int viewW = gridViewport.getViewWidth();
        if (playheadX < viewX + headerWidth || playheadX > viewX + viewW - stepWidth) {
            int targetX = std::max(0, playheadX - viewW / 2);
            gridViewport.setViewPosition(targetX, gridViewport.getViewPositionY());
        }
    }
}

void ConverterWizardComponent::setStep(WizardStep step) {
    if (currentStep == WizardStep::SequencerAudition && step != WizardStep::SequencerAudition) {
        audioPlayer.stopPlayback();
        auditionPlayBtn.setButtonText("▶ PLAY AUDITION");
        auditionPlayBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2f855a));
        stopTimer();
    }

    currentStep = step;

    if (currentStep == WizardStep::SequencerAudition) {
        buildPreviewPattern();
        startTimerHz(30); // 30 FPS playhead refresh
    }

    if (currentStep == WizardStep::SaveExport) {
        if (presetNameEditor.getText().isEmpty()) {
            presetNameEditor.setText(currentPreviewPattern.name.empty() ? parsedMidi.fileName : currentPreviewPattern.name);
        }
        bpmEditor.setText(juce::String(currentPreviewPattern.bpm, 1));
        stepsSelector.setSelectedId(currentPreviewPattern.barLength * 16, juce::dontSendNotification);
    }

    updateVisibilityForStep();
    resized();
    repaint();
}

void ConverterWizardComponent::setPatternBarLength(int newBars) {
    if (newBars <= 0) newBars = 1;
    currentPreviewPattern.barLength = newBars;
    int newTotalSteps = newBars * 16;

    for (auto& [inst, trk] : currentPreviewPattern.tracks) {
        int oldSize = (int)trk.steps.size();
        trk.stepCount = newTotalSteps;
        trk.steps.resize(newTotalSteps);
        if (newTotalSteps > oldSize && oldSize > 0) {
            for (int s = oldSize; s < newTotalSteps; ++s) {
                trk.steps[s] = trk.steps[s % oldSize];
            }
        }
    }

    previewGrid.setPattern(currentPreviewPattern);
    audioPlayer.setPattern(currentPreviewPattern);
    stepsSelector.setSelectedId(newTotalSteps, juce::dontSendNotification);
    resized();
    previewGrid.repaint();
}

void ConverterWizardComponent::buildPreviewPattern() {
    if (isLoadedFromPreset) {
        previewGrid.setPattern(currentPreviewPattern);
        int root = auditionRootSelector.getSelectedId() - 1;
        if (root < 0) root = 0;
        auto quality = static_cast<Harmonic::ChordQuality>(auditionQualitySelector.getSelectedId());
        audioPlayer.setPattern(currentPreviewPattern);
        audioPlayer.updateAuditionChord(root, quality);
        previewGrid.setAuditionChord(root, quality);
        barLengthSelector.setSelectedId(currentPreviewPattern.barLength, juce::dontSendNotification);
        updateInspectorForSelectedStep();
        return;
    }

    ConversionOptions opt;
    opt.presetName = parsedMidi.fileName;
    opt.overrideRootPitchClass = rootSelector.getSelectedId() - 1;
    opt.overrideMode = static_cast<Harmonic::ScaleMode>(modeSelector.getSelectedId() - 1);
    opt.useOverrideMode = true;

    for (const auto& row : trackRows) {
        if (row != nullptr) {
            opt.trackConfigs[row->getTrackIndex()] = row->getConfig();
        }
    }

    currentPreviewPattern = converter.convertToPattern(parsedMidi, tonalResult, opt);
    previewGrid.setPattern(currentPreviewPattern);

    int root = (opt.overrideRootPitchClass >= 0) ? opt.overrideRootPitchClass : tonalResult.detectedRootPitchClass;
    auditionRootSelector.setSelectedId(root + 1, juce::dontSendNotification);
    auditionQualitySelector.setSelectedId(static_cast<int>(tonalResult.detectedChordQuality), juce::dontSendNotification);

    barLengthSelector.setSelectedId(currentPreviewPattern.barLength, juce::dontSendNotification);

    audioPlayer.setPattern(currentPreviewPattern);
    audioPlayer.updateAuditionChord(root, tonalResult.detectedChordQuality);
    previewGrid.setAuditionChord(root, tonalResult.detectedChordQuality);

    updateInspectorForSelectedStep();
}

void ConverterWizardComponent::updateInspectorForSelectedStep() {
    auto inst = previewGrid.getSelectedInstrument();
    int step = previewGrid.getSelectedStep();

    if (previewGrid.getPattern().tracks.find(inst) == previewGrid.getPattern().tracks.end()) {
        inspectorTrackStepLabel.setText("No track", juce::dontSendNotification);
        return;
    }

    const auto& trk = previewGrid.getPattern().tracks.at(inst);
    if (step < 0 || step >= (int)trk.steps.size()) return;

    const auto& sd = trk.steps[step];
    juce::String trackDesc = juce::String(trk.trackName) + " | Step " + juce::String(step + 1) + " (Bar " + juce::String((step / 16) + 1) + ")";
    inspectorTrackStepLabel.setText(trackDesc, juce::dontSendNotification);

    inspectorActiveToggle.setToggleState(sd.active, juce::dontSendNotification);
    inspectorOffsetSlider.setValue(sd.stepOffset, juce::dontSendNotification);
    inspectorVelocitySlider.setValue(sd.velocity, juce::dontSendNotification);

    int totalSteps = currentPreviewPattern.barLength * 16;
    inspectorLengthSlider.setRange(1, std::max(1, totalSteps), 1);
    inspectorLengthSlider.setValue(sd.lengthSteps, juce::dontSendNotification);
    inspectorLengthLabel.setText("Length: " + juce::String(sd.lengthSteps) + " (" + formatStepDuration(sd.lengthSteps) + ")", juce::dontSendNotification);

    int artId = 1;
    if (sd.articulation == Harmonic::ArticulationType::Staccato) artId = 2;
    else if (sd.articulation == Harmonic::ArticulationType::Spiccato) artId = 3;
    else if (sd.articulation == Harmonic::ArticulationType::Marcato) artId = 4;
    inspectorArtSelector.setSelectedId(artId, juce::dontSendNotification);

    std::string noteName = previewGrid.calculateNoteNameForStep(inst, sd);
    inspectorPitchReadout.setText("Note: " + juce::String(noteName), juce::dontSendNotification);
}

void ConverterWizardComponent::updateVisibilityForStep() {
    bool isStep1 = (currentStep == WizardStep::FileLoad);
    bool isStep2 = (currentStep == WizardStep::TonalAnalysis);
    bool isStep3 = (currentStep == WizardStep::TrackMapping);
    bool isStep4 = (currentStep == WizardStep::SequencerAudition);
    bool isStep5 = (currentStep == WizardStep::SaveExport);

    // Step 1
    step1Title.setVisible(isStep1);
    browseBtn.setVisible(isStep1);
    browsePresetBtn.setVisible(isStep1);
    fileInfoLabel.setVisible(isStep1);

    // Step 2
    step2Title.setVisible(isStep2);
    detectedKeyBadge.setVisible(isStep2);
    confidenceLabel.setVisible(isStep2);
    rootSelectorLabel.setVisible(isStep2);
    rootSelector.setVisible(isStep2);
    modeSelectorLabel.setVisible(isStep2);
    modeSelector.setVisible(isStep2);

    // Step 3
    step3Title.setVisible(isStep3);
    trackListViewport.setVisible(isStep3);

    // Step 4 (Multi-Bar Sequencer & Audition Editor)
    step4Title.setVisible(isStep4);
    auditionPlayBtn.setVisible(isStep4);
    auditionBpmLabel.setVisible(isStep4);
    auditionBpmSlider.setVisible(isStep4);
    barLengthLabel.setVisible(isStep4);
    barLengthSelector.setVisible(isStep4);
    btnRemoveBar.setVisible(isStep4);
    btnAddBar.setVisible(isStep4);
    btnDuplicateBar1ToAll.setVisible(isStep4);
    btnLoadPresetInStep4.setVisible(isStep4);
    btnQuickSaveInStep4.setVisible(isStep4);

    auditionChordLabel.setVisible(isStep4);
    auditionRootSelector.setVisible(isStep4);
    auditionQualitySelector.setVisible(isStep4);
    btnChordC.setVisible(isStep4);
    btnChordDm.setVisible(isStep4);
    btnChordG7.setVisible(isStep4);
    btnChordEm.setVisible(isStep4);
    btnChordF.setVisible(isStep4);
    btnChordAm.setVisible(isStep4);
    btnChordOrig.setVisible(isStep4);
    gridViewport.setVisible(isStep4);

    inspectorTitle.setVisible(isStep4);
    inspectorTrackStepLabel.setVisible(isStep4);
    inspectorActiveToggle.setVisible(isStep4);
    inspectorOffsetLabel.setVisible(isStep4);
    inspectorOffsetSlider.setVisible(isStep4);
    inspectorPitchReadout.setVisible(isStep4);
    inspectorVelocityLabel.setVisible(isStep4);
    inspectorVelocitySlider.setVisible(isStep4);
    inspectorLengthLabel.setVisible(isStep4);
    inspectorLengthSlider.setVisible(isStep4);
    btnDur16th.setVisible(isStep4);
    btnDur8th.setVisible(isStep4);
    btnDurQuarter.setVisible(isStep4);
    btnDurHalf.setVisible(isStep4);
    btnDur1Bar.setVisible(isStep4);
    btnDur2Bars.setVisible(isStep4);
    inspectorArtLabel.setVisible(isStep4);
    inspectorArtSelector.setVisible(isStep4);
    btnOctaveUp.setVisible(isStep4);
    btnOctaveDown.setVisible(isStep4);
    btnDuplicateBar.setVisible(isStep4);

    // Step 5
    step5Title.setVisible(isStep5);
    presetNameLabel.setVisible(isStep5);
    presetNameEditor.setVisible(isStep5);
    bpmLabel.setVisible(isStep5);
    bpmEditor.setVisible(isStep5);
    stepsLabel.setVisible(isStep5);
    stepsSelector.setVisible(isStep5);
    saveDirectBtn.setVisible(isStep5);
    saveAsBtn.setVisible(isStep5);
    copyJsonBtn.setVisible(isStep5);
    statusLabel.setVisible(isStep5);

    // Navigation buttons
    backBtn.setVisible(!isStep1);
    nextBtn.setVisible(!isStep5 && hasFileLoaded);
}

void ConverterWizardComponent::loadMidiFile(const juce::File& file) {
    std::string err;
    if (!converter.parseMidiFile(file.getFullPathName().toStdString(), parsedMidi, err)) {
        fileInfoLabel.setText("Failed to parse MIDI file: " + juce::String(err), juce::dontSendNotification);
        fileInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe53e3e));
        hasFileLoaded = false;
        nextBtn.setVisible(false);
        return;
    }

    hasFileLoaded = true;
    isLoadedFromPreset = false;
    tonalResult = converter.analyzeTonalCenter(parsedMidi);

    fileInfoLabel.setText(file.getFileName() + " (" + juce::String(parsedMidi.tracks.size()) + " tracks, "
                         + juce::String(parsedMidi.bpm, 1) + " BPM, "
                         + juce::String(parsedMidi.timeSigNum) + "/" + juce::String(parsedMidi.timeSigDen) + ")",
                         juce::dontSendNotification);
    fileInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xff48bb78));

    // Update Step 2 Controls
    detectedKeyBadge.setText(tonalResult.detectedChordName, juce::dontSendNotification);
    confidenceLabel.setText("Analysis Confidence: " + juce::String(static_cast<int>(tonalResult.confidence * 100.0f)) + "%", juce::dontSendNotification);

    rootSelector.setSelectedId(tonalResult.detectedRootPitchClass + 1, juce::dontSendNotification);

    int modeId = 1;
    switch (tonalResult.detectedMode) {
        case Harmonic::ScaleMode::Ionian:        modeId = 1; break;
        case Harmonic::ScaleMode::Dorian:        modeId = 2; break;
        case Harmonic::ScaleMode::Phrygian:      modeId = 3; break;
        case Harmonic::ScaleMode::Lydian:        modeId = 4; break;
        case Harmonic::ScaleMode::Mixolydian:    modeId = 5; break;
        case Harmonic::ScaleMode::Aeolian:       modeId = 6; break;
        case Harmonic::ScaleMode::Locrian:       modeId = 7; break;
        case Harmonic::ScaleMode::HarmonicMinor: modeId = 8; break;
        case Harmonic::ScaleMode::MelodicMinor:  modeId = 9; break;
        default: modeId = 1; break;
    }
    modeSelector.setSelectedId(modeId, juce::dontSendNotification);

    buildTrackRows();
    nextBtn.setVisible(true);
}

void ConverterWizardComponent::loadPresetFile(const juce::File& file) {
    std::string content = file.loadFileAsString().toStdString();
    if (content.empty()) {
        fileInfoLabel.setText("Failed to load preset: file is empty.", juce::dontSendNotification);
        fileInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe53e3e));
        return;
    }

    currentPreviewPattern = Sequencer::OrchestralPattern::fromJson(content);
    if (currentPreviewPattern.tracks.empty()) {
        fileInfoLabel.setText("Failed to parse preset: no tracks found in JSON.", juce::dontSendNotification);
        fileInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe53e3e));
        return;
    }

    hasFileLoaded = true;
    isLoadedFromPreset = true;

    tonalResult.detectedRootPitchClass = 0; // C default
    tonalResult.detectedChordQuality = Harmonic::ChordQuality::MajorTriad;
    tonalResult.detectedChordName = "C Major (Preset Default)";
    tonalResult.confidence = 1.0f;
    tonalResult.detectedMode = Harmonic::ScaleMode::Ionian;

    int totalSteps = currentPreviewPattern.barLength * 16;
    for (const auto& [inst, trk] : currentPreviewPattern.tracks) {
        if ((int)trk.stepCount > totalSteps) totalSteps = trk.stepCount;
    }
    int bars = std::max(1, totalSteps / 16);

    fileInfoLabel.setText("Loaded Preset: \"" + juce::String(currentPreviewPattern.name) + "\" ("
                         + juce::String(currentPreviewPattern.tracks.size()) + " tracks, "
                         + juce::String(currentPreviewPattern.bpm, 1) + " BPM, "
                         + juce::String(bars) + " Bars / " + juce::String(totalSteps) + " Steps)",
                         juce::dontSendNotification);
    fileInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xff48bb78));

    // Update Step 2 Controls
    detectedKeyBadge.setText(tonalResult.detectedChordName, juce::dontSendNotification);
    confidenceLabel.setText("Preset Loaded Directly", juce::dontSendNotification);
    rootSelector.setSelectedId(1, juce::dontSendNotification);
    modeSelector.setSelectedId(1, juce::dontSendNotification);

    // Build synthetic parsedMidi so Step 3 also works
    parsedMidi.fileName = currentPreviewPattern.name;
    parsedMidi.bpm = currentPreviewPattern.bpm;
    parsedMidi.timeSigNum = currentPreviewPattern.timeSigNumerator;
    parsedMidi.timeSigDen = currentPreviewPattern.timeSigDenominator;
    parsedMidi.tracks.clear();
    int tIdx = 0;
    for (const auto& [inst, trk] : currentPreviewPattern.tracks) {
        ParsedMidiTrack pmt;
        pmt.trackIndex = tIdx++;
        pmt.trackName = trk.trackName;
        pmt.channel = trk.midiChannel;
        pmt.suggestedInstrument = inst;
        pmt.suggestedSection = trk.section;
        pmt.suggestedArticulation = trk.articulation;
        pmt.suggestedArrangerMode = trk.arrangerMode;
        MidiNoteEvent mne;
        mne.pitch = 60;
        mne.startTick = 0;
        mne.durationTicks = 480;
        mne.velocity = 100;
        pmt.notes.push_back(mne);
        parsedMidi.tracks.push_back(pmt);
    }
    buildTrackRows();

    // Update Step 4 Controls
    auditionBpmSlider.setValue(currentPreviewPattern.bpm, juce::dontSendNotification);
    barLengthSelector.setSelectedId(currentPreviewPattern.barLength, juce::dontSendNotification);
    previewGrid.setPattern(currentPreviewPattern);
    audioPlayer.setPattern(currentPreviewPattern);
    audioPlayer.setBpm(currentPreviewPattern.bpm);
    audioPlayer.updateAuditionChord(0, Harmonic::ChordQuality::MajorTriad);
    previewGrid.setAuditionChord(0, Harmonic::ChordQuality::MajorTriad);

    // Update Step 5 Controls
    presetNameEditor.setText(currentPreviewPattern.name);
    bpmEditor.setText(juce::String(currentPreviewPattern.bpm, 1));
    stepsSelector.setSelectedId(currentPreviewPattern.barLength * 16, juce::dontSendNotification);

    nextBtn.setVisible(true);

    // Jump directly to Step 4 (Multi-Bar Editor & Audition)!
    setStep(WizardStep::SequencerAudition);
}

void ConverterWizardComponent::buildTrackRows() {
    trackRows.clear();
    trackListContainer.removeAllChildren();

    int rowHeight = 44;
    int y = 0;
    int w = getWidth() - 75;

    for (size_t i = 0; i < parsedMidi.tracks.size(); ++i) {
        const auto& trk = parsedMidi.tracks[i];
        if (trk.notes.empty()) continue; // Skip completely empty meta tracks

        auto row = std::make_unique<TrackMappingRowComponent>(trk.trackIndex, trk, nullptr);
        row->setBounds(0, y, w, rowHeight - 2);
        trackListContainer.addAndMakeVisible(row.get());
        trackRows.push_back(std::move(row));
        y += rowHeight;
    }
    trackListContainer.setSize(w, y + 10);
}

void ConverterWizardComponent::savePreset(bool promptCustomLocation) {
    if (presetNameEditor.getText().isNotEmpty()) {
        currentPreviewPattern.name = presetNameEditor.getText().toStdString();
    }
    if (currentPreviewPattern.name.empty()) {
        currentPreviewPattern.name = "Custom Orchestration";
    }

    if (bpmEditor.getText().isNotEmpty()) {
        currentPreviewPattern.bpm = bpmEditor.getText().getDoubleValue();
    }
    if (stepsSelector.getSelectedId() > 0) {
        currentPreviewPattern.barLength = std::max(1, stepsSelector.getSelectedId() / 16);
    }

    std::string jsonStr = currentPreviewPattern.toJson();

    if (promptCustomLocation) {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Save Orchestrator Preset...", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.json");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
            [this, chooser, jsonStr](const juce::FileChooser& fc) {
                auto file = fc.getResult();
                if (file != juce::File{}) {
                    if (file.replaceWithText(jsonStr)) {
                        statusLabel.setText("Preset successfully saved to: " + file.getFullPathName(), juce::dontSendNotification);
                        statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff48bb78));
                    } else {
                        statusLabel.setText("Error writing preset to file.", juce::dontSendNotification);
                        statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe53e3e));
                    }
                }
            });
        return;
    } else {
        juce::File folder = getPresetsFolder();
        folder.createDirectory();
        juce::String cleanName = juce::String(currentPreviewPattern.name).replaceCharacters(" /\\:*?\"<>|", "___________");
        if (!cleanName.endsWithIgnoreCase(".json")) cleanName += ".json";
        auto targetFile = folder.getChildFile(cleanName);
        if (targetFile.replaceWithText(jsonStr)) {
            statusLabel.setText("Preset successfully saved to: " + targetFile.getFullPathName(), juce::dontSendNotification);
            statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff48bb78));
        } else {
            statusLabel.setText("Error writing preset to file.", juce::dontSendNotification);
            statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe53e3e));
        }
    }
}

juce::File ConverterWizardComponent::getPresetsFolder() const {
    auto appSupport = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    return appSupport.getChildFile("Application Support").getChildFile("Automatic Orchestrator").getChildFile("Presets");
}

bool ConverterWizardComponent::isInterestedInFileDrag(const juce::StringArray& files) {
    for (const auto& f : files) {
        if (f.endsWithIgnoreCase(".mid") || f.endsWithIgnoreCase(".midi") || f.endsWithIgnoreCase(".json")) return true;
    }
    return false;
}

void ConverterWizardComponent::fileDragEnter(const juce::StringArray&, int, int) {
    isDraggingOver = true;
    repaint();
}

void ConverterWizardComponent::fileDragExit(const juce::StringArray&) {
    isDraggingOver = false;
    repaint();
}

void ConverterWizardComponent::filesDropped(const juce::StringArray& files, int, int) {
    isDraggingOver = false;
    repaint();
    for (const auto& f : files) {
        if (f.endsWithIgnoreCase(".json")) {
            loadPresetFile(juce::File(f));
            break;
        } else if (f.endsWithIgnoreCase(".mid") || f.endsWithIgnoreCase(".midi")) {
            loadMidiFile(juce::File(f));
            break;
        }
    }
}

void ConverterWizardComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff0b0d10));

    int w = getWidth();
    int h = getHeight();

    // Step Progress Indicator Bar (5 steps!)
    g.setColour(juce::Colour(0xff161b22));
    g.fillRect(0, 0, w, 52);
    g.setColour(juce::Colour(0xff30363d));
    g.drawHorizontalLine(52, 0, w);

    static const char* STEP_NAMES[5] = {
        "1. FILE LOAD",
        "2. TONAL KEY",
        "3. INSTRUMENTS",
        "4. AUDITION & EDIT",
        "5. SAVE PRESET"
    };

    int stepW = w / 5;
    for (int i = 0; i < 5; ++i) {
        bool isActive = (static_cast<int>(currentStep) == i);
        bool isDone = (static_cast<int>(currentStep) > i);

        juce::Colour textCol = isActive ? juce::Colour(0xff63b3ed) : (isDone ? juce::Colour(0xff48bb78) : juce::Colour(0xff718096));
        g.setColour(textCol);
        g.setFont(juce::Font(11.5f, isActive ? juce::Font::bold : juce::Font::plain));
        g.drawText(STEP_NAMES[i], i * stepW, 0, stepW, 48, juce::Justification::centred);

        if (isActive) {
            g.fillRect(i * stepW + 8, 48, stepW - 16, 4);
        }
    }

    // Step 1 Drag & Drop Zone
    if (currentStep == WizardStep::FileLoad) {
        auto dropZone = getLocalBounds().reduced(60, 90).withTrimmedBottom(40);
        g.setColour(isDraggingOver ? juce::Colour(0xff3182ce).withAlpha(0.2f) : juce::Colour(0xff161b22));
        g.fillRoundedRectangle(dropZone.toFloat(), 12.0f);

        g.setColour(isDraggingOver ? juce::Colour(0xff63b3ed) : juce::Colour(0xff30363d));
        float dashLengths[2] = {6.0f, 4.0f};
        g.drawDashedLine(juce::Line<float>(dropZone.getX(), dropZone.getY(), dropZone.getRight(), dropZone.getY()), dashLengths, 2, 1.5f);
        g.drawDashedLine(juce::Line<float>(dropZone.getRight(), dropZone.getY(), dropZone.getRight(), dropZone.getBottom()), dashLengths, 2, 1.5f);
        g.drawDashedLine(juce::Line<float>(dropZone.getX(), dropZone.getBottom(), dropZone.getRight(), dropZone.getBottom()), dashLengths, 2, 1.5f);
        g.drawDashedLine(juce::Line<float>(dropZone.getX(), dropZone.getY(), dropZone.getX(), dropZone.getBottom()), dashLengths, 2, 1.5f);

        g.setColour(juce::Colour(0xffedf2f7));
        g.setFont(juce::Font(15.0f, juce::Font::bold));
        g.drawText("DRAG & DROP ORCHESTRAL MIDI (.mid) OR PRESET (.json) HERE", dropZone.removeFromTop(dropZone.getHeight() / 2 + 10), juce::Justification::centred);
    }
}

void ConverterWizardComponent::resized() {
    int w = getWidth();
    int h = getHeight();

    // Navigation buttons at bottom
    backBtn.setBounds(30, h - 50, 100, 32);
    nextBtn.setBounds(w - 130, h - 50, 100, 32);

    // Step 1: File Load
    step1Title.setBounds(60, 68, 500, 24);
    int bW = 200;
    int bGap = 20;
    int startBx = w / 2 - (bW * 2 + bGap) / 2;
    browseBtn.setBounds(startBx, h / 2 + 25, bW, 38);
    browsePresetBtn.setBounds(startBx + bW + bGap, h / 2 + 25, bW, 38);
    fileInfoLabel.setBounds(60, h / 2 + 80, w - 120, 24);

    // Step 2: Tonal Analysis
    step2Title.setBounds(60, 70, 500, 24);
    detectedKeyBadge.setBounds(w / 2 - 170, 120, 340, 70);
    confidenceLabel.setBounds(w / 2 - 150, 200, 300, 24);

    int formY = 245;
    rootSelectorLabel.setBounds(w / 2 - 140, formY, 120, 24);
    rootSelector.setBounds(w / 2, formY, 140, 24);

    formY += 40;
    modeSelectorLabel.setBounds(w / 2 - 140, formY, 120, 24);
    modeSelector.setBounds(w / 2, formY, 140, 24);

    // Step 3: Track Mapping
    step3Title.setBounds(30, 68, 500, 24);
    trackListViewport.setBounds(30, 100, w - 60, h - 165);
    trackListContainer.setSize(w - 75, trackListContainer.getHeight());
    for (auto& row : trackRows) {
        if (row != nullptr) row->setSize(w - 75, row->getHeight());
    }

    // Step 4: Sequencer Preview & Audition (Multi-Bar Editor)
    step4Title.setBounds(24, 56, 440, 24);
    btnDuplicateBar1ToAll.setBounds(w - 380, 54, 140, 26);
    btnLoadPresetInStep4.setBounds(w - 232, 54, 110, 26);
    btnQuickSaveInStep4.setBounds(w - 116, 54, 92, 26);

    // Audition Top Bar
    int audY = 86;
    auditionPlayBtn.setBounds(24, audY, 125, 28);

    auditionBpmLabel.setBounds(156, audY + 3, 30, 22);
    auditionBpmSlider.setBounds(188, audY + 1, 75, 26);

    barLengthLabel.setBounds(270, audY + 3, 35, 22);
    barLengthSelector.setBounds(306, audY + 1, 105, 26);
    btnRemoveBar.setBounds(414, audY + 1, 24, 26);
    btnAddBar.setBounds(441, audY + 1, 46, 26);

    auditionChordLabel.setBounds(495, audY + 3, 42, 22);
    auditionRootSelector.setBounds(540, audY + 1, 50, 26);
    auditionQualitySelector.setBounds(593, audY + 1, 95, 26);

    // Quick chords
    int qcx = 695;
    btnChordC.setBounds(qcx, audY + 1, 28, 26); qcx += 31;
    btnChordDm.setBounds(qcx, audY + 1, 32, 26); qcx += 35;
    btnChordG7.setBounds(qcx, audY + 1, 32, 26); qcx += 35;
    btnChordEm.setBounds(qcx, audY + 1, 32, 26); qcx += 35;
    btnChordF.setBounds(qcx, audY + 1, 28, 26); qcx += 31;
    btnChordAm.setBounds(qcx, audY + 1, 32, 26); qcx += 35;
    btnChordOrig.setBounds(qcx, audY + 1, 55, 26);

    // Multi-track Grid Viewport
    int gridH = h - 250;
    gridViewport.setBounds(24, 122, w - 48, gridH);

    int totalSteps = currentPreviewPattern.barLength * 16;
    for (const auto& [inst, trk] : currentPreviewPattern.tracks) {
        if ((int)trk.stepCount > totalSteps) totalSteps = trk.stepCount;
    }
    if (totalSteps <= 0) totalSteps = 16;
    int gridContentW = std::max(w - 60, 170 + (totalSteps * 24) + 40);
    int gridContentH = std::max(gridH - 10, 26 + static_cast<int>(currentPreviewPattern.tracks.size()) * 36 + 10);
    previewGrid.setSize(gridContentW, gridContentH);

    // Step Detail Inspector Bar (Bottom)
    int inspY = h - 122;
    inspectorTitle.setBounds(24, inspY, 145, 18);
    inspectorTrackStepLabel.setBounds(175, inspY, 230, 18);
    inspectorPitchReadout.setBounds(415, inspY, 160, 18);

    btnDuplicateBar.setBounds(w - 340, inspY - 2, 140, 22);
    btnOctaveUp.setBounds(w - 190, inspY - 2, 75, 22);
    btnOctaveDown.setBounds(w - 110, inspY - 2, 75, 22);

    int row2Y = inspY + 26;
    inspectorActiveToggle.setBounds(24, row2Y, 92, 24);

    inspectorOffsetLabel.setBounds(120, row2Y, 55, 24);
    inspectorOffsetSlider.setBounds(178, row2Y, 65, 24);

    inspectorVelocityLabel.setBounds(248, row2Y, 52, 24);
    inspectorVelocitySlider.setBounds(302, row2Y, 68, 24);

    inspectorLengthLabel.setBounds(376, row2Y, 120, 24);
    inspectorLengthSlider.setBounds(500, row2Y, 70, 24);

    btnDur16th.setBounds(576, row2Y, 36, 24);
    btnDur8th.setBounds(615, row2Y, 34, 24);
    btnDurQuarter.setBounds(652, row2Y, 34, 24);
    btnDurHalf.setBounds(689, row2Y, 34, 24);
    btnDur1Bar.setBounds(726, row2Y, 44, 24);
    btnDur2Bars.setBounds(773, row2Y, 46, 24);

    inspectorArtLabel.setBounds(826, row2Y, 40, 24);
    inspectorArtSelector.setBounds(868, row2Y, 88, 24);

    // Step 5: Save & Export
    step5Title.setBounds(60, 70, 500, 24);

    int s5Y = 120;
    presetNameLabel.setBounds(w / 2 - 180, s5Y, 120, 24);
    presetNameEditor.setBounds(w / 2 - 50, s5Y, 230, 24);

    s5Y += 36;
    bpmLabel.setBounds(w / 2 - 180, s5Y, 120, 24);
    bpmEditor.setBounds(w / 2 - 50, s5Y, 70, 24);

    s5Y += 36;
    stepsLabel.setBounds(w / 2 - 180, s5Y, 120, 24);
    stepsSelector.setBounds(w / 2 - 50, s5Y, 150, 24);

    s5Y += 60;
    saveDirectBtn.setBounds(w / 2 - 200, s5Y, 400, 40);

    s5Y += 48;
    saveAsBtn.setBounds(w / 2 - 200, s5Y, 195, 32);
    copyJsonBtn.setBounds(w / 2 + 5, s5Y, 195, 32);

    s5Y += 44;
    statusLabel.setBounds(60, s5Y, w - 120, 24);
}

// -------------------------------------------------------------
// ConverterAppWindow Implementation
// -------------------------------------------------------------
ConverterAppWindow::ConverterAppWindow(juce::String name)
    : DocumentWindow(name,
                     juce::Colour(0xff0b0d10),
                     DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    setContentOwned(new ConverterWizardComponent(), true);
    setResizable(true, true);
    setResizeLimits(850, 520, 1600, 1000);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}

void ConverterAppWindow::closeButtonPressed() {
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

// -------------------------------------------------------------
// ConverterApplication Implementation
// -------------------------------------------------------------
void ConverterApplication::initialise(const juce::String&) {
    mainWindow = std::make_unique<ConverterAppWindow>(getApplicationName());
}

void ConverterApplication::shutdown() {
    mainWindow = nullptr;
}

void ConverterApplication::systemRequestedQuit() {
    quit();
}

void ConverterApplication::anotherInstanceStarted(const juce::String&) {}

} // namespace Converter

// JUCE Entry Point
START_JUCE_APPLICATION(Converter::ConverterApplication)
