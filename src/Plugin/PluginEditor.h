#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

class MidiDragComponent : public juce::Component {
public:
    MidiDragComponent(HollywoodOrchestratorAudioProcessor& p,
                      std::optional<Harmonic::InstrumentId> stem = std::nullopt,
                      const juce::String& label = "DRAG MIDI")
        : processor(p), stemId(stem), buttonText(label) {}

    void paint(juce::Graphics& g) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    HollywoodOrchestratorAudioProcessor& processor;
    std::optional<Harmonic::InstrumentId> stemId;
    juce::String buttonText;
};

class HollywoodOrchestratorEditor : public juce::AudioProcessorEditor,
                                   public juce::Timer,
                                   public juce::DragAndDropContainer {
public:
    explicit HollywoodOrchestratorEditor(HollywoodOrchestratorAudioProcessor& p);
    ~HollywoodOrchestratorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    HollywoodOrchestratorAudioProcessor& audioProcessor;

    // Header Controls
    juce::Label chordDisplayLabel;
    juce::Label statusLabel;
    juce::ComboBox patternSelector;
    juce::ComboBox modeSelector;
    juce::ComboBox librarySelector;
    juce::ComboBox voicingSelector;

    // Sliders
    juce::Slider inversionJitterSlider;
    juce::Slider tensionInjectSlider;
    juce::Slider humanizeVelSlider;
    juce::Slider humanizeTimeSlider;
    juce::Slider graceWindowSlider;

    juce::Label jitterLabel;
    juce::Label tensionLabel;
    juce::Label humanizeLabel;
    juce::Label graceLabel;

    // Drag-and-drop components
    std::unique_ptr<MidiDragComponent> masterDragButton;
    std::vector<std::unique_ptr<MidiDragComponent>> stemDragButtons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HollywoodOrchestratorEditor)
};
