#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "Converter/MidiPresetConverter.h"

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
// ConverterWizardComponent: Multi-step guided wizard component
// -------------------------------------------------------------
class ConverterWizardComponent : public juce::Component,
                                 public juce::FileDragAndDropTarget {
public:
    ConverterWizardComponent();
    ~ConverterWizardComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void loadMidiFile(const juce::File& file);

private:
    MidiPresetConverter converter;
    ParsedMidiFile parsedMidi;
    TonalAnalysisResult tonalResult;
    bool hasFileLoaded = false;
    bool isDraggingOver = false;

    enum class WizardStep {
        FileLoad = 0,
        TonalAnalysis = 1,
        TrackMapping = 2,
        SaveExport = 3
    };
    WizardStep currentStep = WizardStep::FileLoad;
    void setStep(WizardStep step);

    // Navigation Controls
    juce::TextButton backBtn{"< BACK"};
    juce::TextButton nextBtn{"NEXT >"};

    // Step 1: File Loading
    juce::Label step1Title{"STEP 1: LOAD ORCHESTRAL MIDI FILE"};
    juce::TextButton browseBtn{"Browse MIDI File..."};
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

    // Step 4: Save & Export
    juce::Label step4Title{"STEP 4: PREVIEW & SAVE PRESET"};
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
