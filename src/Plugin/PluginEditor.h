#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

// -------------------------------------------------------------
// MidiDragComponent: Single-latch external file drag
// -------------------------------------------------------------
class MidiDragComponent : public juce::Component {
public:
    MidiDragComponent(AutomaticOrchestratorAudioProcessor& p,
                      std::optional<Harmonic::InstrumentId> stem = std::nullopt,
                      const juce::String& label = "DRAG MIDI")
        : processor(p), stemId(stem), buttonText(label) {}

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    AutomaticOrchestratorAudioProcessor& processor;
    std::optional<Harmonic::InstrumentId> stemId;
    juce::String buttonText;
    bool isDragging = false;
};

// -------------------------------------------------------------
// InstrumentRowComponent: One instrument in the section rack
// -------------------------------------------------------------
class InstrumentRowComponent : public juce::Component {
public:
    InstrumentRowComponent(AutomaticOrchestratorAudioProcessor& p,
                           Harmonic::InstrumentId id,
                           std::function<void(Harmonic::InstrumentId)> onSelect);
    ~InstrumentRowComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    void setSelected(bool sel);
    bool isSelected() const { return selected; }
    Harmonic::InstrumentId getInstrumentId() const { return instrumentId; }

    void refreshFromTrack(const Sequencer::TrackPattern& track);

private:
    AutomaticOrchestratorAudioProcessor& processor;
    Harmonic::InstrumentId instrumentId;
    std::function<void(Harmonic::InstrumentId)> onSelectedCallback;
    bool selected = false;

    juce::Label titleLabel;
    juce::ComboBox articulationBox;
    juce::TextButton muteBtn{"M"};
    juce::TextButton soloBtn{"S"};
    juce::ComboBox modeBox;
    juce::ComboBox octaveBox;
    juce::Slider volumeSlider;
    juce::Label modeLabel{"ARRANGER MODE"};
    juce::Label octaveLabel{"OCTAVE"};

    std::unique_ptr<MidiDragComponent> stemDragBtn;
};

// -------------------------------------------------------------
// -------------------------------------------------------------
// StepGridComponent: 18 Relative Pitches (+9 to -8) x Multi-Bar Steps
// -------------------------------------------------------------
class StepGridComponent : public juce::Component {
public:
    StepGridComponent(AutomaticOrchestratorAudioProcessor& p);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void setActiveInstrument(Harmonic::InstrumentId inst);
    Harmonic::InstrumentId getActiveInstrument() const { return activeInstrument; }

    void setCurrentStep(int step);
    void setTool(int tool) { currentTool = tool; } // 0 = Pencil, 1 = Eraser
    void setNoteVelocity(int vel) { noteVelocity = vel; }

    void refreshFromPattern(const Sequencer::OrchestralPattern& pattern);

    void setBarView(int barView); // 0 = Show All Bars, 1 = Bar 1, 2 = Bar 2...
    int getBarView() const { return currentBarView; }
    int getTotalSteps() const { return totalSteps; }
    int getNumBars() const { return std::max(1, (totalSteps + 15) / 16); }

    void getVisibleStepRange(int& outStartStep, int& outNumSteps) const {
        if (currentBarView <= 0) {
            outStartStep = 0;
            outNumSteps = std::max(16, totalSteps);
        } else {
            outStartStep = (currentBarView - 1) * 16;
            outNumSteps = std::min(16, std::max(1, totalSteps - outStartStep));
        }
    }

    std::function<void(int newBarView)> onBarViewChanged;

private:
    AutomaticOrchestratorAudioProcessor& processor;
    Harmonic::InstrumentId activeInstrument = Harmonic::InstrumentId::Violins1;
    Sequencer::TrackPattern currentTrack;
    int currentStep = -1;
    int currentTool = 0; // 0 = Pencil, 1 = Eraser
    int noteVelocity = 100;
    int totalSteps = 16;
    int currentBarView = 0; // 0 = All, 1 = Bar 1, 2 = Bar 2...

    // Note Resizing State
    bool isResizing = false;
    int resizeStep = -1;
    int originalLength = 1;
    float dragStartX = 0.0f;

    static constexpr int numPitchRows = 18; // +9 down to -8
    static constexpr float labelWidth = 62.0f;
    static constexpr float rulerHeight = 22.0f;

    int getPitchOffsetForRow(int row) const {
        // row 0 = +9, row 9 = 0 (Lowest), row 17 = -8
        return 9 - row;
    }

    int getRowForPitchOffset(int offset) const {
        return 9 - offset;
    }

    void handleCellClick(int step, int row);
};

// -------------------------------------------------------------
// Cc1LaneComponent: Interactive Dynamics / Modulation Curve
// -------------------------------------------------------------
class Cc1LaneComponent : public juce::Component {
public:
    Cc1LaneComponent(AutomaticOrchestratorAudioProcessor& p);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    void setActiveInstrument(Harmonic::InstrumentId inst);
    void refreshFromPattern(const Sequencer::OrchestralPattern& pattern);
    void setBarView(int barView);

    void getVisibleStepRange(int& outStartStep, int& outNumSteps) const {
        if (currentBarView <= 0) {
            outStartStep = 0;
            outNumSteps = std::max(16, totalSteps);
        } else {
            outStartStep = (currentBarView - 1) * 16;
            outNumSteps = std::min(16, std::max(1, totalSteps - outStartStep));
        }
    }

private:
    AutomaticOrchestratorAudioProcessor& processor;
    Harmonic::InstrumentId activeInstrument = Harmonic::InstrumentId::Violins1;
    std::vector<int> cc1Curve;
    int totalSteps = 16;
    int currentBarView = 0;

    void updateCc1At(float mouseX, float mouseY);
};

// -------------------------------------------------------------
// MixerChannelStrip: Single channel console strip
// -------------------------------------------------------------
class MixerChannelStrip : public juce::Component {
public:
    MixerChannelStrip(AutomaticOrchestratorAudioProcessor& p, Harmonic::InstrumentId instId, int chNum);
    ~MixerChannelStrip() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void refreshFromTrack(const Sequencer::TrackPattern& track);
    void setMeterLevel(float level);

    Harmonic::InstrumentId getInstrumentId() const { return instrument; }
    int getMidiChannel() const { return channelNumber; }

private:
    AutomaticOrchestratorAudioProcessor& processor;
    Harmonic::InstrumentId instrument;
    int channelNumber;

    juce::Label chBadge;
    juce::Label nameLabel;
    juce::Label sectionBadge;
    juce::Slider panSlider;
    juce::TextButton muteBtn{"M"};
    juce::TextButton soloBtn{"S"};
    juce::Slider volumeSlider;
    juce::Label dbLabel;

    float currentMeterLevel = 0.0f;
};

// -------------------------------------------------------------
// OrchestralMixerComponent: 16-Channel Console + Master Strip
// -------------------------------------------------------------
class OrchestralMixerComponent : public juce::Component {
public:
    explicit OrchestralMixerComponent(AutomaticOrchestratorAudioProcessor& p);
    ~OrchestralMixerComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void refreshFromPattern(const Sequencer::OrchestralPattern& pattern);
    void updateMeters(int currentStep);

private:
    AutomaticOrchestratorAudioProcessor& processor;
    std::vector<std::unique_ptr<MixerChannelStrip>> strips;

    // Master Bus Controls
    juce::Label masterTitle{"MASTER"};
    juce::Slider masterFader;
    juce::Label masterDbLabel{"0.0 dB"};
    std::unique_ptr<MidiDragComponent> masterDragBtn;
    float masterMeterLevel = 0.0f;
};

// -------------------------------------------------------------
// AutomaticOrchestratorEditor (Full Hollywood Orchestrator UI)
// -------------------------------------------------------------
class HollywoodOrchestratorEditor : public juce::AudioProcessorEditor,
                                   public juce::Timer,
                                   public juce::DragAndDropContainer,
                                   public juce::FileDragAndDropTarget {
public:
    explicit HollywoodOrchestratorEditor(AutomaticOrchestratorAudioProcessor& p);
    ~HollywoodOrchestratorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    AutomaticOrchestratorAudioProcessor& audioProcessor;

    // View Mode
    bool isMixerView = false;
    void updateViewMode(bool mixerView);

    // Top Bar Controls
    juce::TextButton mainModeBtn{"MAIN"};
    juce::TextButton mixerModeBtn{"MIXER"};

    juce::TextButton prevPresetBtn{"<"};
    juce::ComboBox presetSelector;
    juce::TextButton nextPresetBtn{">"};
    juce::TextButton loadPresetBtn{"LOAD..."};
    juce::TextButton savePresetBtn{"SAVE"};
    juce::TextButton saveAsPresetBtn{"SAVE AS..."};

    juce::Label chordDisplayBadge;
    juce::Label tempoBadge;
    juce::ComboBox librarySelector;
    juce::ComboBox voicingSelector;

    // Section Selector Tabs
    Harmonic::OrchestralSection activeSection = Harmonic::OrchestralSection::Strings;
    juce::TextButton woodwindsTab{"WOODWINDS"};
    juce::TextButton brassTab{"BRASS"};
    juce::TextButton percussionTab{"PERCUSSION"};
    juce::TextButton stringsTab{"STRINGS"};

    juce::TextButton woodwindsMuteBtn{"M"};
    juce::TextButton woodwindsSoloBtn{"S"};
    juce::TextButton brassMuteBtn{"M"};
    juce::TextButton brassSoloBtn{"S"};
    juce::TextButton percussionMuteBtn{"M"};
    juce::TextButton percussionSoloBtn{"S"};
    juce::TextButton stringsMuteBtn{"M"};
    juce::TextButton stringsSoloBtn{"S"};

    // Left Panel: Instrument Rack
    juce::Component rackContainer;
    std::vector<std::unique_ptr<InstrumentRowComponent>> instrumentRows;
    juce::TextButton addInstrumentBtn{"+ Add Instrument"};
    Harmonic::InstrumentId selectedInstrument = Harmonic::InstrumentId::Violins2;

    // Right Panel: Step Arranger
    juce::Label voice1Btn{"VOICE 1"};
    juce::Label voice2Btn{"VOICE 2"};
    juce::Label activeInstrumentTitle;

    // Bar Navigation & View Mode
    juce::ComboBox barViewSelector;
    juce::TextButton prevBarBtn{"◀"};
    juce::TextButton nextBarBtn{"▶"};
    juce::TextButton copyBarBtn{"Copy Bar 1 -> All"};

    juce::ComboBox noteGridBox;
    juce::TextButton pencilBtn{"Pencil"};
    juce::TextButton eraserBtn{"Eraser"};
    juce::TextButton clearBtn{"Clear"};

    std::unique_ptr<StepGridComponent> stepGrid;
    std::unique_ptr<Cc1LaneComponent> cc1Lane;

    // Orchestral Mixer Component
    std::unique_ptr<OrchestralMixerComponent> mixerComponent;

    // Bottom Bar Controls
    juce::Label velocityLabel{"VELOCITY"};
    juce::Slider velocitySlider;
    juce::Label sigBadge{"4/4"};
    juce::ComboBox lengthSelector;
    std::unique_ptr<MidiDragComponent> masterDragBtn;

    void switchSection(Harmonic::OrchestralSection section);
    void selectInstrument(Harmonic::InstrumentId inst);
    void loadCurrentPatternIntoUi();
    void populatePresetSelector();
    void saveCurrentPreset();
    void saveAsNewPreset();
    void promptAddInstrument();
    void loadPresetFile(const juce::File& file);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HollywoodOrchestratorEditor)
};

