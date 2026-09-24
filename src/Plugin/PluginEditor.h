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
// StepGridComponent: 18 Relative Pitches (+9 to -8) x 16 Steps
// -------------------------------------------------------------
class StepGridComponent : public juce::Component {
public:
    StepGridComponent(AutomaticOrchestratorAudioProcessor& p);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    void setActiveInstrument(Harmonic::InstrumentId inst);
    Harmonic::InstrumentId getActiveInstrument() const { return activeInstrument; }

    void setCurrentStep(int step);
    void setTool(int tool) { currentTool = tool; } // 0 = Pencil, 1 = Eraser
    void setNoteVelocity(int vel) { noteVelocity = vel; }

    void refreshFromPattern(const Sequencer::OrchestralPattern& pattern);

private:
    AutomaticOrchestratorAudioProcessor& processor;
    Harmonic::InstrumentId activeInstrument = Harmonic::InstrumentId::Violins1;
    Sequencer::TrackPattern currentTrack;
    int currentStep = -1;
    int currentTool = 0; // 0 = Pencil, 1 = Eraser
    int noteVelocity = 100;

    static constexpr int numPitchRows = 18; // +9 down to -8
    static constexpr int numSteps = 16;
    static constexpr float labelWidth = 62.0f;

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

private:
    AutomaticOrchestratorAudioProcessor& processor;
    Harmonic::InstrumentId activeInstrument = Harmonic::InstrumentId::Violins1;
    std::vector<int> cc1Curve;

    void updateCc1At(float mouseX, float mouseY);
};

// -------------------------------------------------------------
// AutomaticOrchestratorEditor (Full Hollywood Orchestrator UI)
// -------------------------------------------------------------
class HollywoodOrchestratorEditor : public juce::AudioProcessorEditor,
                                   public juce::Timer,
                                   public juce::DragAndDropContainer {
public:
    explicit HollywoodOrchestratorEditor(AutomaticOrchestratorAudioProcessor& p);
    ~HollywoodOrchestratorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    AutomaticOrchestratorAudioProcessor& audioProcessor;

    // Top Bar Controls
    juce::TextButton mainModeBtn{"MAIN"};
    juce::TextButton mixerModeBtn{"MIXER"};

    juce::TextButton prevPresetBtn{"<"};
    juce::ComboBox presetSelector;
    juce::TextButton nextPresetBtn{">"};

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
    Harmonic::InstrumentId selectedInstrument = Harmonic::InstrumentId::Violins2;

    // Right Panel: Step Arranger
    juce::Label voice1Btn{"VOICE 1"};
    juce::Label voice2Btn{"VOICE 2"};
    juce::Label activeInstrumentTitle;
    juce::ComboBox noteGridBox;
    juce::TextButton pencilBtn{"Pencil"};
    juce::TextButton eraserBtn{"Eraser"};
    juce::TextButton clearBtn{"Clear"};

    std::unique_ptr<StepGridComponent> stepGrid;
    std::unique_ptr<Cc1LaneComponent> cc1Lane;

    // Bottom Bar Controls
    juce::Label velocityLabel{"VELOCITY"};
    juce::Slider velocitySlider;
    juce::Label sigBadge{"4/4"};
    juce::Label lengthBadge{"2 BARS"};
    std::unique_ptr<MidiDragComponent> masterDragBtn;

    void switchSection(Harmonic::OrchestralSection section);
    void selectInstrument(Harmonic::InstrumentId inst);
    void loadCurrentPatternIntoUi();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HollywoodOrchestratorEditor)
};
