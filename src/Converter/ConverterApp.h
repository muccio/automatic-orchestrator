#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "Converter/MidiPresetConverter.h"
#include "Sequencer/PatternModel.h"
#include "Orchestration/VoicingEngine.h"
#include <atomic>
#include <mutex>

namespace Converter {

// -------------------------------------------------------------
// TrackMappingRowComponent: Row for mapping a single MIDI track
// -------------------------------------------------------------
class TrackMappingRowComponent : public juce::Component {
public:
    TrackMappingRowComponent(int trackIdx, const ParsedMidiTrack& trk, std::function<void()> onChange);
    ~TrackMappingRowComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool isEnabled() const { return enableToggle.getToggleState(); }
    TrackMappingConfig getConfig() const;
    int getTrackIndex() const { return trackIndex; }

private:
    int trackIndex;
    juce::ToggleButton enableToggle;
    juce::Label trackNameLabel;
    juce::Label noteStatsLabel;
    juce::ComboBox instrumentSelector;
    juce::Label sectionBadge;
    juce::ComboBox articulationSelector;
    juce::ComboBox arrangerModeSelector;
    std::function<void()> onConfigChanged;

    void updateSectionBadge();
};

// -------------------------------------------------------------
// AuditionVoice & AuditionAudioPlayer: Real-Time Audio Preview Synth
// -------------------------------------------------------------
struct AuditionVoice {
    bool active = false;
    float phase = 0.0f;
    float phaseDelta = 0.0f;
    float env = 0.0f;
    float velocity = 0.8f;
    float attackRate = 0.015f;
    float releaseRate = 0.004f;
    bool isReleasing = false;
    double remainingSamples = 0.0;
    Harmonic::OrchestralSection section = Harmonic::OrchestralSection::Strings;
    float panL = 0.7f;
    float panR = 0.7f;
};

class AuditionAudioPlayer : public juce::AudioIODeviceCallback {
public:
    AuditionAudioPlayer();
    ~AuditionAudioPlayer() override;

    void startPlayback();
    void stopPlayback();
    bool isPlaying() const { return playing.load(); }

    void setBpm(double bpm) { currentBpm.store(bpm); }
    double getBpm() const { return currentBpm.load(); }

    void setPattern(const Sequencer::OrchestralPattern& pat);
    void updateAuditionChord(int rootPc, Harmonic::ChordQuality quality);
    int getCurrentStep() const { return currentStep.load(); }
    void setCurrentStep(int s) { currentStep.store(s); }

    // AudioIODeviceCallback
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                         int numInputChannels,
                                         float* const* outputChannelData,
                                         int numOutputChannels,
                                         int numSamples,
                                         const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    juce::AudioDeviceManager deviceManager;
    std::atomic<bool> playing{false};
    std::atomic<double> currentBpm{120.0};
    std::atomic<int> currentStep{0};
    double sampleRate = 44100.0;
    double stepSampleCounter = 0.0;

    std::mutex patternMutex;
    Sequencer::OrchestralPattern cachedPattern;
    Harmonic::HarmonicFrame cachedChord;
    std::vector<int> cachedPitchLadder;
    std::map<Harmonic::InstrumentId, int> cachedVoiceBases;
    int cachedStepCount = 16;

    static constexpr int NUM_VOICES = 48;
    AuditionVoice voices[NUM_VOICES];
    int nextVoiceIndex = 0;

    void renderVoices(float* outL, float* outR, int numSamples);
    void advanceStepAndTriggerNotes();
    void rebuildAuditionHarmony();
};

// -------------------------------------------------------------
// SequencerPreviewGridComponent: Interactive Multi-Track Step Grid
// -------------------------------------------------------------
class SequencerPreviewGridComponent : public juce::Component {
public:
    SequencerPreviewGridComponent();
    ~SequencerPreviewGridComponent() override = default;

    void setPattern(const Sequencer::OrchestralPattern& pat);
    const Sequencer::OrchestralPattern& getPattern() const { return pattern; }
    Sequencer::OrchestralPattern& getPatternRef() { return pattern; }

    void setAuditionChord(int rootPc, Harmonic::ChordQuality quality);
    void setPlayheadStep(int step);

    std::function<void(Harmonic::InstrumentId inst, int stepIndex)> onStepSelected;
    std::function<void()> onPatternModified;

    Harmonic::InstrumentId getSelectedInstrument() const { return selectedInst; }
    int getSelectedStep() const { return selectedStep; }

    void setSelectedStep(Harmonic::InstrumentId inst, int step);
    void updateSelectedStep(bool active, int stepOffset, int velocity, int lengthSteps, Harmonic::ArticulationType art);
    std::string calculateNoteNameForStep(Harmonic::InstrumentId inst, const Sequencer::StepDefinition& stepDef) const;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    Sequencer::OrchestralPattern pattern;
    int auditionRoot = 2; // D default
    Harmonic::ChordQuality auditionQuality = Harmonic::ChordQuality::Major7;
    int playheadStep = -1;

    Harmonic::InstrumentId selectedInst = Harmonic::InstrumentId::Violins1;
    int selectedStep = 0;
    std::vector<Harmonic::InstrumentId> trackList;

    bool isResizingDuration = false;
    Harmonic::InstrumentId resizingInst = Harmonic::InstrumentId::Violins1;
    int resizingStep = -1;
    int originalLength = 1;
    int dragStartX = 0;

    void rebuildTrackList();
};

// -------------------------------------------------------------
// ConverterWizardComponent: Multi-step guided wizard component
// -------------------------------------------------------------
class ConverterWizardComponent : public juce::Component,
                                 public juce::FileDragAndDropTarget,
                                 public juce::Timer {
public:
    ConverterWizardComponent();
    ~ConverterWizardComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void loadMidiFile(const juce::File& file);
    void loadPresetFile(const juce::File& file);
    void setPatternBarLength(int newBars);

private:
    MidiPresetConverter converter;
    ParsedMidiFile parsedMidi;
    TonalAnalysisResult tonalResult;
    bool hasFileLoaded = false;
    bool isLoadedFromPreset = false;
    bool isDraggingOver = false;

    enum class WizardStep {
        FileLoad = 0,
        TonalAnalysis = 1,
        TrackMapping = 2,
        SequencerAudition = 3,  // <-- Interactive Sequencer & Audition
        SaveExport = 4
    };
    WizardStep currentStep = WizardStep::FileLoad;
    void setStep(WizardStep step);

    // Navigation Controls
    juce::TextButton backBtn{"< BACK"};
    juce::TextButton nextBtn{"NEXT >"};

    // Step 1: File Loading
    juce::Label step1Title{"STEP 1: LOAD ORCHESTRAL MIDI FILE OR PRESET"};
    juce::TextButton browseBtn{"Browse MIDI File..."};
    juce::TextButton browsePresetBtn{"Load Existing Preset (.json)..."};
    juce::Label fileInfoLabel;

    // Step 2: Tonal & Harmonic Analysis
    juce::Label step2Title{"STEP 2: HARMONIC & TONAL ANALYSIS"};
    juce::Label detectedKeyBadge;
    juce::Label confidenceLabel;
    juce::Label rootSelectorLabel{"Root Note:"};
    juce::ComboBox rootSelector;
    juce::Label modeSelectorLabel{"Scale / Mode:"};
    juce::ComboBox modeSelector;

    // Step 3: Track & Instrument Mapping
    juce::Label step3Title{"STEP 3: ORCHESTRAL INSTRUMENT MAPPING"};
    juce::Viewport trackListViewport;
    juce::Component trackListContainer;
    std::vector<std::unique_ptr<TrackMappingRowComponent>> trackRows;

    // Step 4: Sequencer Preview & Audition
    juce::Label step4Title{"STEP 4: SEQUENCER PREVIEW & AUDITION (MULTI-BAR EDITOR)"};
    juce::TextButton auditionPlayBtn{"▶ PLAY AUDITION"};
    juce::Label auditionBpmLabel{"BPM:"};
    juce::Slider auditionBpmSlider;
    juce::Label barLengthLabel{"Bars:"};
    juce::ComboBox barLengthSelector;
    juce::TextButton btnRemoveBar{"-1 Bar"};
    juce::TextButton btnAddBar{"+1 Bar"};
    juce::TextButton btnDuplicateBar1ToAll{"Copy Bar 1 -> All"};
    juce::TextButton btnLoadPresetInStep4{"📂 Load Preset..."};
    juce::TextButton btnQuickSaveInStep4{"💾 Quick Save"};

    juce::Label auditionChordLabel{"Chord:"};
    juce::ComboBox auditionRootSelector;
    juce::ComboBox auditionQualitySelector;

    // Quick chord audition presets
    juce::TextButton btnChordC{"C"};
    juce::TextButton btnChordDm{"Dm"};
    juce::TextButton btnChordG7{"G7"};
    juce::TextButton btnChordEm{"Em"};
    juce::TextButton btnChordF{"F"};
    juce::TextButton btnChordAm{"Am"};
    juce::TextButton btnChordOrig{"Orig Key"};

    // Multi-track Sequencer Grid with Viewport
    juce::Viewport gridViewport;
    SequencerPreviewGridComponent previewGrid;

    // Step Inspector controls
    juce::Label inspectorTitle{"STEP DETAIL INSPECTOR:"};
    juce::Label inspectorTrackStepLabel{"No step selected"};
    juce::ToggleButton inspectorActiveToggle{"Step Active"};
    juce::Label inspectorOffsetLabel{"Degree Offset:"};
    juce::Slider inspectorOffsetSlider;
    juce::Label inspectorPitchReadout;
    juce::Label inspectorVelocityLabel{"Velocity:"};
    juce::Slider inspectorVelocitySlider;
    juce::Label inspectorLengthLabel{"Length (Steps):"};
    juce::Slider inspectorLengthSlider;
    juce::TextButton btnDur16th{"1/16"};
    juce::TextButton btnDur8th{"1/8"};
    juce::TextButton btnDurQuarter{"1/4"};
    juce::TextButton btnDurHalf{"1/2"};
    juce::TextButton btnDur1Bar{"1 Bar"};
    juce::TextButton btnDur2Bars{"2 Bars"};
    juce::Label inspectorArtLabel{"Articulation:"};
    juce::ComboBox inspectorArtSelector;
    juce::TextButton btnOctaveUp{"+1 Octave"};
    juce::TextButton btnOctaveDown{"-1 Octave"};
    juce::TextButton btnDuplicateBar{"Duplicate Bar 1 -> 2"};

    // Audio Engine & preview pattern
    AuditionAudioPlayer audioPlayer;
    Sequencer::OrchestralPattern currentPreviewPattern;

    // Step 5: Save & Export
    juce::Label step5Title{"STEP 5: PREVIEW & SAVE PRESET"};
    juce::Label presetNameLabel{"Preset Name:"};
    juce::TextEditor presetNameEditor;
    juce::Label bpmLabel{"Tempo (BPM):"};
    juce::TextEditor bpmEditor;
    juce::Label stepsLabel{"Length:"};
    juce::ComboBox stepsSelector;
    juce::TextButton saveDirectBtn{"SAVE TO AUTOMATIC ORCHESTRATOR PRESETS"};
    juce::TextButton saveAsBtn{"SAVE AS..."};
    juce::TextButton copyJsonBtn{"COPY JSON"};
    juce::Label statusLabel;

    void updateVisibilityForStep();
    void buildTrackRows();
    void buildPreviewPattern();
    void updateInspectorForSelectedStep();
    void savePreset(bool promptCustomLocation);
    juce::File getPresetsFolder() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConverterWizardComponent)
};

// -------------------------------------------------------------
// ConverterAppWindow: Top-level desktop window
// -------------------------------------------------------------
class ConverterAppWindow : public juce::DocumentWindow {
public:
    ConverterAppWindow(juce::String name);
    void closeButtonPressed() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConverterAppWindow)
};

// -------------------------------------------------------------
// ConverterApplication: JUCE Application entry
// -------------------------------------------------------------
class ConverterApplication : public juce::JUCEApplication {
public:
    ConverterApplication() = default;
    const juce::String getApplicationName() override { return "Automatic Orchestrator - MIDI Converter"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& commandLine) override;
    void shutdown() override;
    void systemRequestedQuit() override;
    void anotherInstanceStarted(const juce::String& commandLine) override;

private:
    std::unique_ptr<ConverterAppWindow> mainWindow;
};

} // namespace Converter
