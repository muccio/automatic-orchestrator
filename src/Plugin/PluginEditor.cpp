#include "PluginEditor.h"

// -------------------------------------------------------------
// MidiDragComponent Implementation
// -------------------------------------------------------------

void MidiDragComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    bool isMaster = !stemId.has_value();

    juce::Colour bg = isMaster ? juce::Colour(0xff00d2ff) : juce::Colour(0xff2d3748);
    juce::Colour textCol = isMaster ? juce::Colours::black : juce::Colour(0xffe2e8f0);

    g.setColour(bg.withAlpha(0.85f));
    g.fillRoundedRectangle(bounds, 5.0f);

    g.setColour(isMaster ? juce::Colours::white : juce::Colour(0xff4a5568));
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

    g.setColour(textCol);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawFittedText(buttonText, getLocalBounds(), juce::Justification::centred, 1);
}

void MidiDragComponent::mouseDrag(const juce::MouseEvent& e) {
    if (e.getDistanceFromDragStart() > 6) {
        std::string midiPath = processor.exportMidiForDrag(2, stemId);
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
            container->performExternalDragDropOfFiles({juce::String(midiPath)}, false);
        }
    }
}

// -------------------------------------------------------------
// HollywoodOrchestratorEditor Implementation
// -------------------------------------------------------------

HollywoodOrchestratorEditor::HollywoodOrchestratorEditor(HollywoodOrchestratorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Header Chord Display
    chordDisplayLabel.setText("READY", juce::dontSendNotification);
    chordDisplayLabel.setFont(juce::Font(28.0f, juce::Font::bold));
    chordDisplayLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00d2ff)); // Electric cyan
    chordDisplayLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff181a1f));
    chordDisplayLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(chordDisplayLabel);

    statusLabel.setText("Real-Time Harmonic Recognition Active", juce::dontSendNotification);
    statusLabel.setFont(juce::Font(11.0f, juce::Font::plain));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff718096));
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);

    // Style / Pattern Selector
    patternSelector.addItem("Action Ostinato", 1);
    patternSelector.addItem("Epic Brass Fanfare", 2);
    patternSelector.addItem("Lyrical Adagio", 3);
    patternSelector.addItem("Suspense Mystery", 4);
    patternSelector.setSelectedId(1, juce::dontSendNotification);
    patternSelector.onChange = [this]() {
        int id = patternSelector.getSelectedId();
        if (id == 1) audioProcessor.setStylePattern(Sequencer::createActionOstinatoPattern());
        else if (id == 2) audioProcessor.setStylePattern(Sequencer::createEpicFanfarePattern());
        else if (id == 3) audioProcessor.setStylePattern(Sequencer::createLyricalAdagioPattern());
        else if (id == 4) audioProcessor.setStylePattern(Sequencer::createSuspenseMysteryPattern());
    };
    addAndMakeVisible(patternSelector);

    // Scale / Mode Selector
    modeSelector.addItem("Ionian (Major)", 1);
    modeSelector.addItem("Dorian", 2);
    modeSelector.addItem("Phrygian", 3);
    modeSelector.addItem("Lydian (#11)", 4);
    modeSelector.addItem("Mixolydian", 5);
    modeSelector.addItem("Aeolian (Minor)", 6);
    modeSelector.addItem("Harmonic Minor", 7);
    modeSelector.addItem("Whole-Tone", 8);
    modeSelector.addItem("Octatonic", 9);
    modeSelector.setSelectedId(1, juce::dontSendNotification);
    modeSelector.onChange = [this]() {
        int id = modeSelector.getSelectedId();
        Harmonic::ScaleMode modes[] = {
            Harmonic::ScaleMode::Ionian, Harmonic::ScaleMode::Dorian, Harmonic::ScaleMode::Phrygian,
            Harmonic::ScaleMode::Lydian, Harmonic::ScaleMode::Mixolydian, Harmonic::ScaleMode::Aeolian,
            Harmonic::ScaleMode::HarmonicMinor, Harmonic::ScaleMode::WholeTone, Harmonic::ScaleMode::Octatonic
        };
        if (id >= 1 && id <= 9) audioProcessor.setScaleMode(modes[id - 1]);
    };
    addAndMakeVisible(modeSelector);

    // Library Profile Selector
    librarySelector.addItem("Cinematic Studio Series (CC58)", 1);
    librarySelector.addItem("Spitfire Audio (UACC CC32)", 2);
    librarySelector.addItem("EastWest Hollywood Opus (KS)", 3);
    librarySelector.addItem("VSL Synchron", 4);
    librarySelector.addItem("Kontakt Factory Library", 5);
    librarySelector.setSelectedId(1, juce::dontSendNotification);
    librarySelector.onChange = [this]() {
        int id = librarySelector.getSelectedId();
        if (id == 1) audioProcessor.setLibraryProfile(Articulation::createCSSProfile());
        else if (id == 2) audioProcessor.setLibraryProfile(Articulation::createSpitfireUACCProfile());
        else if (id == 3) audioProcessor.setLibraryProfile(Articulation::createEastWestOpusProfile());
        else if (id == 4) audioProcessor.setLibraryProfile(Articulation::createVSLProfile());
        else if (id == 5) audioProcessor.setLibraryProfile(Articulation::createKontaktProfile());
    };
    addAndMakeVisible(librarySelector);

    // Voicing Selector
    voicingSelector.addItem("Acoustic Pyramid", 1);
    voicingSelector.addItem("Drop-2 Voicing", 2);
    voicingSelector.addItem("Drop-4 Voicing", 3);
    voicingSelector.setSelectedId(1, juce::dontSendNotification);
    voicingSelector.onChange = [this]() {
        int id = voicingSelector.getSelectedId();
        if (id == 1) audioProcessor.setVoicingStyle(Orchestration::VoicingStyle::AcousticPyramid);
        else if (id == 2) audioProcessor.setVoicingStyle(Orchestration::VoicingStyle::Drop2);
        else if (id == 3) audioProcessor.setVoicingStyle(Orchestration::VoicingStyle::Drop4);
    };
    addAndMakeVisible(voicingSelector);

    // Master Drag Button
    masterDragButton = std::make_unique<MidiDragComponent>(audioProcessor, std::nullopt, "DRAG MASTER MIDI (16 CH)");
    addAndMakeVisible(*masterDragButton);

    // Stem Drag Buttons for Sections
    Harmonic::InstrumentId stemIds[] = {
        Harmonic::InstrumentId::Violins1,
        Harmonic::InstrumentId::Cellos,
        Harmonic::InstrumentId::FrenchHorns,
        Harmonic::InstrumentId::Trumpets,
        Harmonic::InstrumentId::Flutes,
        Harmonic::InstrumentId::Timpani
    };
    const char* stemNames[] = {"Vln 1", "Cello", "Horns", "Tpt", "Flute", "Timp"};

    for (int i = 0; i < 6; ++i) {
        auto stemBtn = std::make_unique<MidiDragComponent>(audioProcessor, stemIds[i], stemNames[i]);
        stemDragButtons.push_back(std::move(stemBtn));
        addAndMakeVisible(*stemDragButtons.back());
    }

    // Sliders
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name, double minVal, double maxVal, double defaultVal, const juce::String& suffix) {
        s.setRange(minVal, maxVal);
        s.setValue(defaultVal);
        s.setTextValueSuffix(suffix);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 18);
        addAndMakeVisible(s);

        l.setText(name, juce::dontSendNotification);
        l.setFont(juce::Font(10.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, juce::Colour(0xffa0aec0));
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
    };

    setupSlider(inversionJitterSlider, jitterLabel, "INV JITTER", 0.0, 1.0, 0.0, "");
    inversionJitterSlider.onValueChange = [this]() { audioProcessor.setInversionJitter(inversionJitterSlider.getValue()); };

    setupSlider(tensionInjectSlider, tensionLabel, "MODAL COLOR", 0.0, 1.0, 0.0, "");
    tensionInjectSlider.onValueChange = [this]() { audioProcessor.setTensionInjection(tensionInjectSlider.getValue()); };

    setupSlider(humanizeVelSlider, humanizeLabel, "HUMANIZE", 0, 20, 5, "");
    humanizeVelSlider.onValueChange = [this]() { audioProcessor.setVelocityHumanize((int)humanizeVelSlider.getValue()); };

    setSize(920, 640);
    startTimerHz(30); // 30 FPS UI refresh
}

HollywoodOrchestratorEditor::~HollywoodOrchestratorEditor() {
    stopTimer();
}

void HollywoodOrchestratorEditor::timerCallback() {
    std::string chordName = audioProcessor.getCurrentChordName();
    chordDisplayLabel.setText(chordName, juce::dontSendNotification);
    repaint();
}

void HollywoodOrchestratorEditor::paint(juce::Graphics& g) {
    // Modern sleek dark background
    g.fillAll(juce::Colour(0xff0d0e11));

    // Header background banner
    g.setColour(juce::Colour(0xff16181d));
    g.fillRect(0, 0, getWidth(), 105);

    g.setColour(juce::Colour(0xff232730));
    g.drawHorizontalLine(105, 0.0f, (float)getWidth());
    g.drawHorizontalLine(getHeight() - 110, 0.0f, (float)getWidth());

    // Title branding
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(15.0f, juce::Font::bold));
    g.drawText("HOLLYWOOD ORCHESTRATOR", 20, 15, 240, 20, juce::Justification::left);

    g.setColour(juce::Colour(0xff00d2ff));
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("VST3 MIDI SOURCE / ENGINE", 20, 35, 200, 15, juce::Justification::left);

    // Section Matrix Graphic Representation
    int matrixY = 120;
    const char* sections[] = {"STRINGS (Ch 1-5)", "BRASS (Ch 6-9)", "WOODWINDS (Ch 10-13)", "PERCUSSION (Ch 14-16)"};
    juce::Colour sectionColours[] = {juce::Colour(0xff4fd1c5), juce::Colour(0xfff6ad55), juce::Colour(0xff68d391), juce::Colour(0xfffc8181)};

    for (int s = 0; s < 4; ++s) {
        int panelY = matrixY + s * 95;
        g.setColour(juce::Colour(0xff16181d));
        g.fillRoundedRectangle(20.0f, (float)panelY, (float)(getWidth() - 40), 85.0f, 6.0f);

        g.setColour(sectionColours[s]);
        g.fillRect(20, panelY, 4, 85);

        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText(sections[s], 35, panelY + 8, 200, 18, juce::Justification::left);

        // Simulated step sequencer mini lanes
        for (int step = 0; step < 16; ++step) {
            float stepX = 220.0f + step * 40.0f;
            g.setColour(juce::Colour(0xff232730));
            g.fillRoundedRectangle(stepX, (float)(panelY + 30), 34.0f, 40.0f, 3.0f);

            // Step tick indicator
            g.setColour((step % 4 == 0) ? sectionColours[s].withAlpha(0.7f) : juce::Colour(0xff3b4252));
            g.fillRoundedRectangle(stepX + 2.0f, (float)(panelY + 32), 30.0f, 15.0f, 2.0f);
        }
    }
}

void HollywoodOrchestratorEditor::resized() {
    // Header Row Layout
    chordDisplayLabel.setBounds(getWidth() / 2 - 130, 15, 260, 48);
    statusLabel.setBounds(getWidth() / 2 - 130, 68, 260, 18);

    patternSelector.setBounds(20, 65, 170, 26);
    modeSelector.setBounds(200, 65, 130, 26);
    librarySelector.setBounds(getWidth() - 360, 65, 180, 26);
    if (masterDragButton != nullptr) {
        masterDragButton->setBounds(getWidth() - 170, 65, 150, 26);
    }

    // Stem drag buttons placed in the section matrix
    int stemIdx = 0;
    for (auto& btn : stemDragButtons) {
        if (btn != nullptr) {
            int col = stemIdx % 6;
            btn->setBounds(20 + col * 90, getHeight() - 100, 80, 24);
            stemIdx++;
        }
    }

    // Bottom controls
    int bottomY = getHeight() - 95;
    voicingSelector.setBounds(560, bottomY + 20, 130, 26);

    inversionJitterSlider.setBounds(700, bottomY, 65, 65);
    jitterLabel.setBounds(700, bottomY + 68, 65, 15);

    tensionInjectSlider.setBounds(770, bottomY, 65, 65);
    tensionLabel.setBounds(770, bottomY + 68, 65, 15);

    humanizeVelSlider.setBounds(840, bottomY, 65, 65);
    humanizeLabel.setBounds(840, bottomY + 68, 65, 15);
}
