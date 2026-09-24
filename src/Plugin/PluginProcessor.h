#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Harmonic/GraceWindowBuffer.h"
#include "Harmonic/ChordDetector.h"
#include "Harmonic/ScaleQuantizer.h"
#include "Orchestration/VoicingEngine.h"
#include "Orchestration/HarmonicRandomizer.h"
#include "Sequencer/SequencerEngine.h"
#include "Articulation/ArticulationMap.h"
#include "MidiExport/StandardMidiWriter.h"
#include <atomic>
#include <mutex>

class AutomaticOrchestratorAudioProcessor : public juce::AudioProcessor {
public:
    AutomaticOrchestratorAudioProcessor();
    ~AutomaticOrchestratorAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Orchestrator Controls & State Access
    void setScaleMode(Harmonic::ScaleMode mode);
    Harmonic::ScaleMode getScaleMode() const { return currentScaleMode.load(); }

    void setVoicingStyle(Orchestration::VoicingStyle style);
    void setLibraryProfile(const Articulation::LibraryProfile& profile);

    void setStylePattern(const Sequencer::OrchestralPattern& pattern);
    const Sequencer::OrchestralPattern getCurrentPattern() const;

    // Interactive Arranger Track & Step Editing
    void setTrackStep(Harmonic::InstrumentId inst, int stepIndex, bool active, int stepOffset, int velocity, Harmonic::ArticulationType art) {
        sequencerEngine.setTrackStep(inst, stepIndex, active, stepOffset, velocity, art);
    }
    void setTrackArticulation(Harmonic::InstrumentId inst, Harmonic::ArticulationType art) {
        sequencerEngine.setTrackArticulation(inst, art);
    }
    void setTrackMode(Harmonic::InstrumentId inst, const std::string& mode) {
        sequencerEngine.setTrackMode(inst, mode);
    }
    void setTrackOctave(Harmonic::InstrumentId inst, int octave) {
        sequencerEngine.setTrackOctave(inst, octave);
    }
    void setTrackVolume(Harmonic::InstrumentId inst, float vol) {
        sequencerEngine.setTrackVolume(inst, vol);
    }
    void setTrackMute(Harmonic::InstrumentId inst, bool mute) {
        sequencerEngine.setTrackMute(inst, mute);
    }
    void setTrackSolo(Harmonic::InstrumentId inst, bool solo) {
        sequencerEngine.setTrackSolo(inst, solo);
    }
    void setTrackCc1(Harmonic::InstrumentId inst, int stepIndex, int cc1Val) {
        sequencerEngine.setTrackCc1(inst, stepIndex, cc1Val);
    }
    int getCurrentStep() const { return sequencerEngine.getCurrentStep(); }

    void setTempoBpm(double bpm) { sequencerEngine.setTempo(bpm); }
    double getTempoBpm() const { return sequencerEngine.getTempo(); }

    void setModalSnappingEnabled(bool enabled) { modalSnappingEnabled.store(enabled); }
    bool isModalSnappingEnabled() const { return modalSnappingEnabled.load(); }

    // Harmonic & Transport Query for GUI
    std::string getCurrentChordName() const;
    Harmonic::HarmonicFrame getCurrentHarmonicFrame() const;
    Harmonic::OrchestralVoicing getCurrentVoicing() const;

    // Export MIDI to temp path for Drag-and-Drop
    std::string exportMidiForDrag(int numBars = 2,
                                  std::optional<Harmonic::InstrumentId> singleStem = std::nullopt);

    // Randomizer controls
    void setVelocityHumanize(int amt) { randomizer.setVelocityHumanizeAmount(amt); }
    void setTimingHumanize(double ms) { randomizer.setTimingHumanizeMs(ms); }
    void setInversionJitter(double chance) { randomizer.setInversionJitterChance(chance); }
    void setTensionInjection(double chance) { randomizer.setTensionInjectionChance(chance); }

private:
    Harmonic::GraceWindowBuffer graceWindow;
    Harmonic::ChordDetector chordDetector;
    Harmonic::ScaleQuantizer scaleQuantizer;
    Orchestration::VoicingEngine voicingEngine;
    Orchestration::HarmonicRandomizer randomizer;
    Sequencer::SequencerEngine sequencerEngine;
    Articulation::LibraryProfile activeLibrary;
    MidiExport::StandardMidiWriter midiWriter;

    std::atomic<Harmonic::ScaleMode> currentScaleMode{Harmonic::ScaleMode::Ionian};
    std::atomic<Orchestration::VoicingStyle> currentVoicingStyle{Orchestration::VoicingStyle::AcousticPyramid};
    std::atomic<bool> modalSnappingEnabled{false};

    mutable std::mutex stateMutex;
    Harmonic::HarmonicFrame lastFrame;
    Harmonic::OrchestralVoicing lastVoicing;
    std::string lastChordName = "Ready";

    double currentSampleRate = 44100.0;
    std::vector<Sequencer::ScheduledMidiEvent> scheduledBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomaticOrchestratorAudioProcessor)
};

using HollywoodOrchestratorAudioProcessor = AutomaticOrchestratorAudioProcessor;
