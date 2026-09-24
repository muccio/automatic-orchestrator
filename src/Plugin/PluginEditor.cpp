#include "PluginEditor.h"

// -------------------------------------------------------------
// MidiDragComponent Implementation
// -------------------------------------------------------------
void MidiDragComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    bool isMaster = !stemId.has_value();

    juce::Colour bg = isMaster ? juce::Colour(0xff00d2ff) : juce::Colour(0xff2d3748);
    juce::Colour textCol = isMaster ? juce::Colours::black : juce::Colour(0xffe2e8f0);

    g.setColour(bg.withAlpha(0.9f));
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(isMaster ? juce::Colours::white : juce::Colour(0xff4a5568));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    g.setColour(textCol);
    g.setFont(juce::Font(10.5f, juce::Font::bold));
    g.drawFittedText(buttonText, getLocalBounds(), juce::Justification::centred, 1);
}

void MidiDragComponent::mouseDown(const juce::MouseEvent&) {
    isDragging = false;
}

void MidiDragComponent::mouseDrag(const juce::MouseEvent& e) {
    if (!isDragging && e.getDistanceFromDragStart() > 6) {
        isDragging = true;
        std::string midiPath = processor.exportMidiForDrag(2, stemId);
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
            container->performExternalDragDropOfFiles({juce::String(midiPath)}, false);
        }
    }
}

void MidiDragComponent::mouseUp(const juce::MouseEvent&) {
    isDragging = false;
}

// -------------------------------------------------------------
// InstrumentRowComponent Implementation
// -------------------------------------------------------------
InstrumentRowComponent::InstrumentRowComponent(AutomaticOrchestratorAudioProcessor& p,
                                               Harmonic::InstrumentId id,
                                               std::function<void(Harmonic::InstrumentId)> onSelect)
    : processor(p), instrumentId(id), onSelectedCallback(onSelect)
{
    titleLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    addAndMakeVisible(titleLabel);

    // Articulations
    articulationBox.addItem("Sustain", 1);
    articulationBox.addItem("Spiccato", 2);
    articulationBox.addItem("Staccato", 3);
    articulationBox.addItem("Marcato", 4);
    articulationBox.addItem("Tremolo", 5);
    articulationBox.addItem("Pizzicato", 6);
    articulationBox.addItem("Runs", 7);
    articulationBox.onChange = [this]() {
        int id = articulationBox.getSelectedId();
        Harmonic::ArticulationType arts[] = {
            Harmonic::ArticulationType::Sustain,
            Harmonic::ArticulationType::Spiccato,
            Harmonic::ArticulationType::Staccato,
            Harmonic::ArticulationType::Marcato,
            Harmonic::ArticulationType::Tremolo,
            Harmonic::ArticulationType::Pizzicato,
            Harmonic::ArticulationType::Runs
        };
        if (id >= 1 && id <= 7) {
            processor.setTrackArticulation(instrumentId, arts[id - 1]);
        }
    };
    addAndMakeVisible(articulationBox);

    // Mute / Solo
    muteBtn.setClickingTogglesState(true);
    muteBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffe53e3e));
    muteBtn.onClick = [this]() { processor.setTrackMute(instrumentId, muteBtn.getToggleState()); };
    addAndMakeVisible(muteBtn);

    soloBtn.setClickingTogglesState(true);
    soloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffecc94b));
    soloBtn.onClick = [this]() { processor.setTrackSolo(instrumentId, soloBtn.getToggleState()); };
    addAndMakeVisible(soloBtn);

    // Arranger Mode
    modeBox.addItem("Top", 1);
    modeBox.addItem("Lowest", 2);
    modeBox.addItem("Chord", 3);
    modeBox.addItem("Root", 4);
    modeBox.addItem("Arp Up", 5);
    modeBox.addItem("Arp Down", 6);
    modeBox.onChange = [this]() {
        juce::String text = modeBox.getText();
        processor.setTrackMode(instrumentId, text.toStdString());
    };
    addAndMakeVisible(modeBox);

    modeLabel.setFont(juce::Font(8.5f, juce::Font::bold));
    modeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff718096));
    addAndMakeVisible(modeLabel);

    // Octave
    octaveBox.addItem("-2", 1);
    octaveBox.addItem("-1", 2);
    octaveBox.addItem("0", 3);
    octaveBox.addItem("+1", 4);
    octaveBox.addItem("+2", 5);
    octaveBox.onChange = [this]() {
        int val = octaveBox.getSelectedId() - 3; // 1->-2, 2->-1, 3->0, 4->1, 5->2
        processor.setTrackOctave(instrumentId, val);
    };
    addAndMakeVisible(octaveBox);

    octaveLabel.setFont(juce::Font(8.5f, juce::Font::bold));
    octaveLabel.setColour(juce::Label::textColourId, juce::Colour(0xff718096));
    addAndMakeVisible(octaveLabel);

    // Volume Slider
    volumeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.85);
    volumeSlider.onValueChange = [this]() {
        processor.setTrackVolume(instrumentId, (float)volumeSlider.getValue());
    };
    addAndMakeVisible(volumeSlider);

    // Stem MIDI drag
    stemDragBtn = std::make_unique<MidiDragComponent>(processor, instrumentId, "MIDI");
    addAndMakeVisible(*stemDragBtn);
}

InstrumentRowComponent::~InstrumentRowComponent() {}

void InstrumentRowComponent::setSelected(bool sel) {
    if (selected != sel) {
        selected = sel;
        repaint();
    }
}

void InstrumentRowComponent::mouseDown(const juce::MouseEvent&) {
    if (onSelectedCallback) {
        onSelectedCallback(instrumentId);
    }
}

void InstrumentRowComponent::refreshFromTrack(const Sequencer::TrackPattern& track) {
    titleLabel.setText(track.trackName.empty() ? Harmonic::instrumentToString(track.instrument) : track.trackName, juce::dontSendNotification);

    // Articulation
    int artId = 1;
    switch (track.articulation) {
        case Harmonic::ArticulationType::Sustain: artId = 1; break;
        case Harmonic::ArticulationType::Spiccato: artId = 2; break;
        case Harmonic::ArticulationType::Staccato: artId = 3; break;
        case Harmonic::ArticulationType::Marcato: artId = 4; break;
        case Harmonic::ArticulationType::Tremolo: artId = 5; break;
        case Harmonic::ArticulationType::Pizzicato: artId = 6; break;
        case Harmonic::ArticulationType::Runs: artId = 7; break;
        default: artId = 1; break;
    }
    articulationBox.setSelectedId(artId, juce::dontSendNotification);

    muteBtn.setToggleState(track.isMuted, juce::dontSendNotification);
    soloBtn.setToggleState(track.isSolo, juce::dontSendNotification);

    // Mode
    if (track.arrangerMode == "Top") modeBox.setSelectedId(1, juce::dontSendNotification);
    else if (track.arrangerMode == "Lowest") modeBox.setSelectedId(2, juce::dontSendNotification);
    else if (track.arrangerMode == "Chord") modeBox.setSelectedId(3, juce::dontSendNotification);
    else if (track.arrangerMode == "Root") modeBox.setSelectedId(4, juce::dontSendNotification);
    else if (track.arrangerMode == "Arp Up") modeBox.setSelectedId(5, juce::dontSendNotification);
    else if (track.arrangerMode == "Arp Down") modeBox.setSelectedId(6, juce::dontSendNotification);
    else modeBox.setSelectedId(1, juce::dontSendNotification);

    // Octave: track.octaveOffset in [-2..2] -> ID in [1..5]
    octaveBox.setSelectedId(std::clamp(track.octaveOffset + 3, 1, 5), juce::dontSendNotification);
    volumeSlider.setValue(track.volume, juce::dontSendNotification);
    repaint();
}

void InstrumentRowComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    if (selected) {
        g.setColour(juce::Colour(0xff1e232d));
        g.fillRoundedRectangle(bounds, 5.0f);
        g.setColour(juce::Colour(0xff00d2ff)); // Cyan highlight
        g.drawRoundedRectangle(bounds, 5.0f, 1.5f);
    } else {
        g.setColour(juce::Colour(0xff14161b));
        g.fillRoundedRectangle(bounds, 5.0f);
        g.setColour(juce::Colour(0xff232730));
        g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
    }
}

void InstrumentRowComponent::resized() {
    titleLabel.setBounds(10, 4, 170, 20);
    muteBtn.setBounds(getWidth() - 95, 6, 22, 18);
    soloBtn.setBounds(getWidth() - 70, 6, 22, 18);
    if (stemDragBtn != nullptr) {
        stemDragBtn->setBounds(getWidth() - 44, 5, 38, 20);
    }

    articulationBox.setBounds(10, 26, 170, 22);

    modeLabel.setBounds(10, 52, 95, 12);
    modeBox.setBounds(10, 65, 95, 20);

    octaveLabel.setBounds(112, 52, 50, 12);
    octaveBox.setBounds(112, 65, 50, 20);

    volumeSlider.setBounds(getWidth() - 65, 32, 55, 55);
}

// -------------------------------------------------------------
// StepGridComponent Implementation
// -------------------------------------------------------------
StepGridComponent::StepGridComponent(AutomaticOrchestratorAudioProcessor& p)
    : processor(p)
{
    refreshFromPattern(processor.getCurrentPattern());
}

void StepGridComponent::setActiveInstrument(Harmonic::InstrumentId inst) {
    activeInstrument = inst;
    refreshFromPattern(processor.getCurrentPattern());
    repaint();
}

void StepGridComponent::setCurrentStep(int step) {
    if (currentStep != step) {
        currentStep = step;
        repaint();
    }
}

void StepGridComponent::refreshFromPattern(const Sequencer::OrchestralPattern& pattern) {
    if (pattern.tracks.find(activeInstrument) != pattern.tracks.end()) {
        currentTrack = pattern.tracks.at(activeInstrument);
    }
    repaint();
}

void StepGridComponent::handleCellClick(int step, int row) {
    if (step < 0 || step >= numSteps || row < 0 || row >= numPitchRows) return;
    int targetOffset = getPitchOffsetForRow(row);

    if (currentTool == 1) { // Eraser
        processor.setTrackStep(activeInstrument, step, false, targetOffset, 0, currentTrack.articulation);
    } else { // Pencil
        // Check if currently active at this offset
        bool isAlreadyActive = false;
        if (step < (int)currentTrack.steps.size()) {
            const auto& s = currentTrack.steps[step];
            if (s.active && s.stepOffset == targetOffset) {
                isAlreadyActive = true;
            }
        }

        if (isAlreadyActive) {
            // Toggle off
            processor.setTrackStep(activeInstrument, step, false, targetOffset, 0, currentTrack.articulation);
        } else {
            // Set note at this step and pitch offset
            processor.setTrackStep(activeInstrument, step, true, targetOffset, noteVelocity, currentTrack.articulation);
        }
    }

    refreshFromPattern(processor.getCurrentPattern());
}

void StepGridComponent::mouseDown(const juce::MouseEvent& e) {
    float gridX = (float)e.x - labelWidth;
    if (gridX < 0) return;

    float gridW = (float)getWidth() - labelWidth;
    float cellW = gridW / (float)numSteps;
    float cellH = (float)getHeight() / (float)numPitchRows;

    int step = static_cast<int>(gridX / cellW);
    int row = static_cast<int>((float)e.y / cellH);

    handleCellClick(step, row);
}

void StepGridComponent::mouseDrag(const juce::MouseEvent& e) {
    float gridX = (float)e.x - labelWidth;
    if (gridX < 0) return;

    float gridW = (float)getWidth() - labelWidth;
    float cellW = gridW / (float)numSteps;
    float cellH = (float)getHeight() / (float)numPitchRows;

    int step = static_cast<int>(gridX / cellW);
    int row = static_cast<int>((float)e.y / cellH);

    if (step >= 0 && step < numSteps && row >= 0 && row < numPitchRows) {
        int targetOffset = getPitchOffsetForRow(row);
        if (currentTool == 1) {
            processor.setTrackStep(activeInstrument, step, false, targetOffset, 0, currentTrack.articulation);
        } else {
            processor.setTrackStep(activeInstrument, step, true, targetOffset, noteVelocity, currentTrack.articulation);
        }
        refreshFromPattern(processor.getCurrentPattern());
    }
}

void StepGridComponent::paint(juce::Graphics& g) {
    float w = (float)getWidth();
    float h = (float)getHeight();

    // Background
    g.fillAll(juce::Colour(0xff0d0f13));

    float gridW = w - labelWidth;
    float cellW = gridW / (float)numSteps;
    float cellH = h / (float)numPitchRows;

    // Y-Axis Labels Column
    g.setColour(juce::Colour(0xff13161c));
    g.fillRect(0.0f, 0.0f, labelWidth, h);

    for (int r = 0; r < numPitchRows; ++r) {
        float y = r * cellH;
        int offset = getPitchOffsetForRow(r);

        juce::String labelText;
        if (offset > 0) labelText = "Steps +" + juce::String(offset);
        else if (offset == 0) labelText = "Lowest";
        else labelText = "Steps " + juce::String(offset);

        // Highlight Lowest / Root row
        if (offset == 0) {
            g.setColour(juce::Colour(0xff1e293b));
            g.fillRect(0.0f, y, labelWidth, cellH);
            g.setColour(juce::Colour(0xff00d2ff));
        } else {
            g.setColour(juce::Colour(0xff718096));
        }

        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText(labelText, 4, (int)y, (int)(labelWidth - 8), (int)cellH, juce::Justification::centredRight);

        // Horizontal grid line
        g.setColour(offset == 0 ? juce::Colour(0xff2d3748) : juce::Colour(0xff181b22));
        g.drawHorizontalLine((int)(y + cellH), 0.0f, w);
    }

    // Step columns
    for (int s = 0; s < numSteps; ++s) {
        float x = labelWidth + s * cellW;

        // Alternating beat background shading
        if ((s / 4) % 2 == 1) {
            g.setColour(juce::Colour(0xff11141a));
            g.fillRect(x, 0.0f, cellW, h);
        }

        // Vertical grid lines
        bool isBeat = (s % 4 == 0);
        bool isBar = (s % 8 == 0);
        g.setColour(isBar ? juce::Colour(0xff3b4252) : (isBeat ? juce::Colour(0xff232730) : juce::Colour(0xff161920)));
        g.drawVerticalLine((int)x, 0.0f, h);
    }

    // Draw active notes
    juce::Colour noteColor = juce::Colour(0xff00d2ff); // Default Cyan for Strings
    // Color code according to instrument section
    int instVal = static_cast<int>(activeInstrument);
    if (instVal >= 0 && instVal <= 4) noteColor = juce::Colour(0xff00d2ff); // Strings (Cyan)
    else if (instVal >= 5 && instVal <= 8) noteColor = juce::Colour(0xfff6ad55); // Brass (Gold)
    else if (instVal >= 9 && instVal <= 12) noteColor = juce::Colour(0xff68d391); // Woodwinds (Emerald)
    else noteColor = juce::Colour(0xfffc8181); // Percussion (Coral)

    for (size_t s = 0; s < currentTrack.steps.size() && s < (size_t)numSteps; ++s) {
        const auto& stepDef = currentTrack.steps[s];
        if (stepDef.active && stepDef.action != Harmonic::StepActionType::Rest) {
            int row = getRowForPitchOffset(stepDef.stepOffset);
            if (row >= 0 && row < numPitchRows) {
                float x = labelWidth + s * cellW + 1.5f;
                float y = row * cellH + 1.5f;
                float blockW = cellW - 3.0f;
                float blockH = cellH - 3.0f;

                // Glowing note block
                g.setColour(noteColor.withAlpha(0.85f));
                g.fillRoundedRectangle(x, y, blockW, blockH, 3.0f);

                g.setColour(juce::Colours::white);
                g.drawRoundedRectangle(x, y, blockW, blockH, 3.0f, 1.0f);
            }
        }
    }

    // Playhead line
    if (currentStep >= 0 && currentStep < numSteps) {
        float playX = labelWidth + currentStep * cellW;
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawVerticalLine((int)playX, 0.0f, h);
        g.setColour(juce::Colour(0xff00d2ff).withAlpha(0.25f));
        g.fillRect(playX, 0.0f, cellW, h);
    }
}

// -------------------------------------------------------------
// Cc1LaneComponent Implementation
// -------------------------------------------------------------
Cc1LaneComponent::Cc1LaneComponent(AutomaticOrchestratorAudioProcessor& p)
    : processor(p)
{
    refreshFromPattern(processor.getCurrentPattern());
}

void Cc1LaneComponent::setActiveInstrument(Harmonic::InstrumentId inst) {
    activeInstrument = inst;
    refreshFromPattern(processor.getCurrentPattern());
    repaint();
}

void Cc1LaneComponent::refreshFromPattern(const Sequencer::OrchestralPattern& pattern) {
    if (pattern.tracks.find(activeInstrument) != pattern.tracks.end()) {
        cc1Curve = pattern.tracks.at(activeInstrument).cc1Curve;
    }
    if (cc1Curve.empty()) {
        cc1Curve.assign(16, 80);
    }
    repaint();
}

void Cc1LaneComponent::updateCc1At(float mouseX, float mouseY) {
    float labelWidth = 62.0f;
    float laneX = mouseX - labelWidth;
    if (laneX < 0) return;

    float laneW = (float)getWidth() - labelWidth;
    float stepW = laneW / 16.0f;
    int step = static_cast<int>(laneX / stepW);
    step = std::clamp(step, 0, 15);

    float h = (float)getHeight();
    float normalizedVal = 1.0f - std::clamp(mouseY / h, 0.0f, 1.0f);
    int ccVal = static_cast<int>(normalizedVal * 127.0f);

    if (step < (int)cc1Curve.size()) {
        cc1Curve[step] = ccVal;
        processor.setTrackCc1(activeInstrument, step, ccVal);
        repaint();
    }
}

void Cc1LaneComponent::mouseDown(const juce::MouseEvent& e) {
    updateCc1At((float)e.x, (float)e.y);
}

void Cc1LaneComponent::mouseDrag(const juce::MouseEvent& e) {
    updateCc1At((float)e.x, (float)e.y);
}

void Cc1LaneComponent::paint(juce::Graphics& g) {
    float w = (float)getWidth();
    float h = (float)getHeight();
    float labelWidth = 62.0f;

    g.fillAll(juce::Colour(0xff090b0e));

    // CC1 Label
    g.setColour(juce::Colour(0xff13161c));
    g.fillRect(0.0f, 0.0f, labelWidth, h);

    g.setColour(juce::Colour(0xff00d2ff));
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText("CC1", 4, 0, (int)(labelWidth - 8), (int)h, juce::Justification::centred);

    // Border line
    g.setColour(juce::Colour(0xff232730));
    g.drawHorizontalLine(0, 0.0f, w);

    float laneW = w - labelWidth;
    float stepW = laneW / 16.0f;

    // Draw CC1 Bars and Curve
    juce::Path curvePath;
    curvePath.startNewSubPath(labelWidth, h);

    for (int s = 0; s < 16; ++s) {
        float x = labelWidth + s * stepW;
        int val = (s < (int)cc1Curve.size()) ? cc1Curve[s] : 80;
        float barH = (val / 127.0f) * (h - 6.0f);
        float barY = h - barH;

        // Bar Fill
        g.setColour(juce::Colour(0xff00d2ff).withAlpha(0.35f));
        g.fillRect(x + 1.0f, barY, stepW - 2.0f, barH);

        // Top line
        g.setColour(juce::Colour(0xff00d2ff));
        g.drawHorizontalLine((int)barY, x + 1.0f, x + stepW - 1.0f);

        curvePath.lineTo(x + stepW * 0.5f, barY);
    }

    curvePath.lineTo(w, h);
    curvePath.closeSubPath();

    g.setColour(juce::Colour(0xff00d2ff).withAlpha(0.15f));
    g.fillPath(curvePath);
}

// -------------------------------------------------------------
// HollywoodOrchestratorEditor Implementation
// -------------------------------------------------------------
HollywoodOrchestratorEditor::HollywoodOrchestratorEditor(AutomaticOrchestratorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Mode toggles
    mainModeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3748));
    addAndMakeVisible(mainModeBtn);
    mixerModeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a202c));
    addAndMakeVisible(mixerModeBtn);

    // Preset navigation
    prevPresetBtn.onClick = [this]() {
        int id = presetSelector.getSelectedId();
        if (id > 1) presetSelector.setSelectedId(id - 1);
        else presetSelector.setSelectedId(presetSelector.getNumItems());
    };
    addAndMakeVisible(prevPresetBtn);

    presetSelector.addItem("Action Ostinato", 1);
    presetSelector.addItem("Epic Brass Fanfare", 2);
    presetSelector.addItem("Lyrical Adagio", 3);
    presetSelector.addItem("Suspense Mystery", 4);
    presetSelector.addItem("War Drums & Percussion", 5);
    presetSelector.addItem("Fantasy Adventure", 6);
    presetSelector.setSelectedId(1, juce::dontSendNotification);
    presetSelector.onChange = [this]() {
        int id = presetSelector.getSelectedId();
        if (id == 1) audioProcessor.setStylePattern(Sequencer::createActionOstinatoPattern());
        else if (id == 2) audioProcessor.setStylePattern(Sequencer::createEpicFanfarePattern());
        else if (id == 3) audioProcessor.setStylePattern(Sequencer::createLyricalAdagioPattern());
        else if (id == 4) audioProcessor.setStylePattern(Sequencer::createSuspenseMysteryPattern());
        else if (id == 5) audioProcessor.setStylePattern(Sequencer::createWarDrumsPattern());
        else if (id == 6) audioProcessor.setStylePattern(Sequencer::createFantasyAdventurePattern());

        loadCurrentPatternIntoUi();
    };
    addAndMakeVisible(presetSelector);

    nextPresetBtn.onClick = [this]() {
        int id = presetSelector.getSelectedId();
        if (id < presetSelector.getNumItems()) presetSelector.setSelectedId(id + 1);
        else presetSelector.setSelectedId(1);
    };
    addAndMakeVisible(nextPresetBtn);

    // Live Chord Recognition Badge
    chordDisplayBadge.setText("READY", juce::dontSendNotification);
    chordDisplayBadge.setFont(juce::Font(22.0f, juce::Font::bold));
    chordDisplayBadge.setColour(juce::Label::textColourId, juce::Colour(0xff00d2ff)); // Neon Cyan
    chordDisplayBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff161b22));
    chordDisplayBadge.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(chordDisplayBadge);

    // Tempo Badge
    tempoBadge.setText("130 bpm", juce::dontSendNotification);
    tempoBadge.setFont(juce::Font(12.0f, juce::Font::bold));
    tempoBadge.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    tempoBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1a202c));
    tempoBadge.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(tempoBadge);

    // Library Selector
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

    // Section Selector Tabs
    auto setupTab = [this](juce::TextButton& btn, Harmonic::OrchestralSection sec) {
        btn.onClick = [this, sec]() { switchSection(sec); };
        addAndMakeVisible(btn);
    };
    setupTab(woodwindsTab, Harmonic::OrchestralSection::Woodwinds);
    setupTab(brassTab, Harmonic::OrchestralSection::Brass);
    setupTab(percussionTab, Harmonic::OrchestralSection::Percussion);
    setupTab(stringsTab, Harmonic::OrchestralSection::Strings);

    woodwindsMuteBtn.setClickingTogglesState(true);
    woodwindsMuteBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffe53e3e));
    addAndMakeVisible(woodwindsMuteBtn);
    woodwindsSoloBtn.setClickingTogglesState(true);
    woodwindsSoloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffecc94b));
    addAndMakeVisible(woodwindsSoloBtn);

    brassMuteBtn.setClickingTogglesState(true);
    brassMuteBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffe53e3e));
    addAndMakeVisible(brassMuteBtn);
    brassSoloBtn.setClickingTogglesState(true);
    brassSoloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffecc94b));
    addAndMakeVisible(brassSoloBtn);

    percussionMuteBtn.setClickingTogglesState(true);
    percussionMuteBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffe53e3e));
    addAndMakeVisible(percussionMuteBtn);
    percussionSoloBtn.setClickingTogglesState(true);
    percussionSoloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffecc94b));
    addAndMakeVisible(percussionSoloBtn);

    stringsMuteBtn.setClickingTogglesState(true);
    stringsMuteBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffe53e3e));
    addAndMakeVisible(stringsMuteBtn);
    stringsSoloBtn.setClickingTogglesState(true);
    stringsSoloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffecc94b));
    addAndMakeVisible(stringsSoloBtn);

    // Left Panel Rack
    addAndMakeVisible(rackContainer);

    // Right Panel Header
    voice1Btn.setFont(juce::Font(11.0f, juce::Font::bold));
    voice1Btn.setColour(juce::Label::backgroundColourId, juce::Colour(0xff2d3748));
    voice1Btn.setColour(juce::Label::textColourId, juce::Colours::white);
    voice1Btn.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(voice1Btn);

    voice2Btn.setFont(juce::Font(11.0f, juce::Font::bold));
    voice2Btn.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1a202c));
    voice2Btn.setColour(juce::Label::textColourId, juce::Colour(0xff718096));
    voice2Btn.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(voice2Btn);

    activeInstrumentTitle.setFont(juce::Font(14.0f, juce::Font::bold));
    activeInstrumentTitle.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    activeInstrumentTitle.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(activeInstrumentTitle);

    noteGridBox.addItem("1/16", 1);
    noteGridBox.addItem("1/16T", 2);
    noteGridBox.addItem("1/8", 3);
    noteGridBox.addItem("1/4", 4);
    noteGridBox.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(noteGridBox);

    pencilBtn.setClickingTogglesState(true);
    pencilBtn.setToggleState(true, juce::dontSendNotification);
    pencilBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00d2ff));
    pencilBtn.onClick = [this]() {
        pencilBtn.setToggleState(true, juce::dontSendNotification);
        eraserBtn.setToggleState(false, juce::dontSendNotification);
        if (stepGrid != nullptr) stepGrid->setTool(0);
    };
    addAndMakeVisible(pencilBtn);

    eraserBtn.setClickingTogglesState(true);
    eraserBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xfffc8181));
    eraserBtn.onClick = [this]() {
        eraserBtn.setToggleState(true, juce::dontSendNotification);
        pencilBtn.setToggleState(false, juce::dontSendNotification);
        if (stepGrid != nullptr) stepGrid->setTool(1);
    };
    addAndMakeVisible(eraserBtn);

    clearBtn.onClick = [this]() {
        for (int s = 0; s < 16; ++s) {
            audioProcessor.setTrackStep(selectedInstrument, s, false, 0, 0, Harmonic::ArticulationType::Sustain);
        }
        loadCurrentPatternIntoUi();
    };
    addAndMakeVisible(clearBtn);

    // Step Grid Component
    stepGrid = std::make_unique<StepGridComponent>(audioProcessor);
    addAndMakeVisible(*stepGrid);

    // CC1 Lane Component
    cc1Lane = std::make_unique<Cc1LaneComponent>(audioProcessor);
    addAndMakeVisible(*cc1Lane);

    // Bottom Bar
    velocityLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    velocityLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa0aec0));
    addAndMakeVisible(velocityLabel);

    velocitySlider.setRange(1, 127, 1);
    velocitySlider.setValue(100);
    velocitySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    velocitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 45, 18);
    velocitySlider.onValueChange = [this]() {
        if (stepGrid != nullptr) stepGrid->setNoteVelocity((int)velocitySlider.getValue());
    };
    addAndMakeVisible(velocitySlider);

    sigBadge.setFont(juce::Font(11.0f, juce::Font::bold));
    sigBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1a202c));
    sigBadge.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    sigBadge.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(sigBadge);

    lengthBadge.setFont(juce::Font(11.0f, juce::Font::bold));
    lengthBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1a202c));
    lengthBadge.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    lengthBadge.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(lengthBadge);

    masterDragBtn = std::make_unique<MidiDragComponent>(audioProcessor, std::nullopt, "DRAG MASTER MIDI (16 CH)");
    addAndMakeVisible(*masterDragBtn);

    // Initial section build
    switchSection(Harmonic::OrchestralSection::Strings);

    setSize(1080, 720);
    startTimerHz(30);
}

HollywoodOrchestratorEditor::~HollywoodOrchestratorEditor() {
    stopTimer();
}

void HollywoodOrchestratorEditor::switchSection(Harmonic::OrchestralSection section) {
    activeSection = section;

    // Reset tab button highlight styles
    stringsTab.setColour(juce::TextButton::buttonColourId, (section == Harmonic::OrchestralSection::Strings) ? juce::Colour(0xffe53e3e) : juce::Colour(0xff1a202c));
    brassTab.setColour(juce::TextButton::buttonColourId, (section == Harmonic::OrchestralSection::Brass) ? juce::Colour(0xffd69e2e) : juce::Colour(0xff1a202c));
    woodwindsTab.setColour(juce::TextButton::buttonColourId, (section == Harmonic::OrchestralSection::Woodwinds) ? juce::Colour(0xff38a169) : juce::Colour(0xff1a202c));
    percussionTab.setColour(juce::TextButton::buttonColourId, (section == Harmonic::OrchestralSection::Percussion) ? juce::Colour(0xffe53e3e) : juce::Colour(0xff1a202c));

    // Clear and repopulate instrument rack for active section
    instrumentRows.clear();

    std::vector<Harmonic::InstrumentId> insts;
    if (section == Harmonic::OrchestralSection::Strings) {
        insts = {
            Harmonic::InstrumentId::Violins1,
            Harmonic::InstrumentId::Violins2,
            Harmonic::InstrumentId::Violas,
            Harmonic::InstrumentId::Cellos,
            Harmonic::InstrumentId::DoubleBasses
        };
    } else if (section == Harmonic::OrchestralSection::Brass) {
        insts = {
            Harmonic::InstrumentId::FrenchHorns,
            Harmonic::InstrumentId::Trumpets,
            Harmonic::InstrumentId::Trombones,
            Harmonic::InstrumentId::Tuba
        };
    } else if (section == Harmonic::OrchestralSection::Woodwinds) {
        insts = {
            Harmonic::InstrumentId::Flutes,
            Harmonic::InstrumentId::Oboes,
            Harmonic::InstrumentId::Clarinets,
            Harmonic::InstrumentId::Bassoons
        };
    } else {
        insts = {
            Harmonic::InstrumentId::Timpani,
            Harmonic::InstrumentId::OrchestralPerc
        };
    }

    for (auto id : insts) {
        auto row = std::make_unique<InstrumentRowComponent>(audioProcessor, id, [this](Harmonic::InstrumentId selectedId) {
            selectInstrument(selectedId);
        });
        rackContainer.addAndMakeVisible(*row);
        instrumentRows.push_back(std::move(row));
    }

    if (!insts.empty()) {
        selectInstrument(insts.front());
    }

    loadCurrentPatternIntoUi();
    resized();
}

void HollywoodOrchestratorEditor::selectInstrument(Harmonic::InstrumentId inst) {
    selectedInstrument = inst;

    for (auto& row : instrumentRows) {
        if (row != nullptr) {
            row->setSelected(row->getInstrumentId() == inst);
        }
    }

    auto pat = audioProcessor.getCurrentPattern();
    if (pat.tracks.find(inst) != pat.tracks.end()) {
        const auto& t = pat.tracks.at(inst);
        activeInstrumentTitle.setText(t.trackName.empty() ? Harmonic::instrumentToString(inst).c_str() : t.trackName.c_str(), juce::dontSendNotification);
    }

    if (stepGrid != nullptr) {
        stepGrid->setActiveInstrument(inst);
    }
    if (cc1Lane != nullptr) {
        cc1Lane->setActiveInstrument(inst);
    }
}

void HollywoodOrchestratorEditor::loadCurrentPatternIntoUi() {
    auto pat = audioProcessor.getCurrentPattern();

    // Update tempo badge
    tempoBadge.setText(juce::String((int)pat.bpm) + " bpm", juce::dontSendNotification);

    // Refresh each instrument row
    for (auto& row : instrumentRows) {
        if (row != nullptr) {
            Harmonic::InstrumentId id = row->getInstrumentId();
            if (pat.tracks.find(id) != pat.tracks.end()) {
                row->refreshFromTrack(pat.tracks.at(id));
            }
        }
    }

    // Refresh active instrument title and grid
    if (pat.tracks.find(selectedInstrument) != pat.tracks.end()) {
        const auto& t = pat.tracks.at(selectedInstrument);
        activeInstrumentTitle.setText(t.trackName.empty() ? Harmonic::instrumentToString(selectedInstrument).c_str() : t.trackName.c_str(), juce::dontSendNotification);
    }

    if (stepGrid != nullptr) {
        stepGrid->refreshFromPattern(pat);
    }
    if (cc1Lane != nullptr) {
        cc1Lane->refreshFromPattern(pat);
    }
}

void HollywoodOrchestratorEditor::timerCallback() {
    std::string chordName = audioProcessor.getCurrentChordName();
    chordDisplayBadge.setText(chordName, juce::dontSendNotification);

    int step = audioProcessor.getCurrentStep();
    if (stepGrid != nullptr) {
        stepGrid->setCurrentStep(step);
    }
}

void HollywoodOrchestratorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff0b0d10));

    // Top Header Banner
    g.setColour(juce::Colour(0xff12151b));
    g.fillRect(0, 0, getWidth(), 78);

    g.setColour(juce::Colour(0xff1e232d));
    g.drawHorizontalLine(78, 0.0f, (float)getWidth());
    g.drawHorizontalLine(getHeight() - 65, 0.0f, (float)getWidth());

    // Branding Title in Header
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("AUTOMATIC ORCHESTRATOR", 15, 12, 220, 18, juce::Justification::left);

    g.setColour(juce::Colour(0xff00d2ff));
    g.setFont(juce::Font(9.5f, juce::Font::bold));
    g.drawText("PRO ORCHESTRAL MIDI ARRANGER", 15, 29, 220, 15, juce::Justification::left);

    // Section bar background
    g.setColour(juce::Colour(0xff14171e));
    g.fillRect(0, 79, getWidth(), 34);
    g.setColour(juce::Colour(0xff232730));
    g.drawHorizontalLine(113, 0.0f, (float)getWidth());

    // Bottom Branding
    g.setColour(juce::Colour(0xff4a5568));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("AUTOMATIC ORCHESTRATOR", getWidth() / 2 - 120, getHeight() - 25, 240, 20, juce::Justification::centred);
}

void HollywoodOrchestratorEditor::resized() {
    int w = getWidth();
    int h = getHeight();

    // Top Row Controls
    mainModeBtn.setBounds(15, 48, 55, 22);
    mixerModeBtn.setBounds(75, 48, 55, 22);

    prevPresetBtn.setBounds(145, 48, 25, 22);
    presetSelector.setBounds(172, 48, 180, 22);
    nextPresetBtn.setBounds(354, 48, 25, 22);

    chordDisplayBadge.setBounds(w / 2 - 80, 15, 160, 48);
    tempoBadge.setBounds(w - 380, 48, 75, 22);
    librarySelector.setBounds(w - 295, 48, 175, 22);
    voicingSelector.setBounds(w - 110, 48, 95, 22);

    // Section Tabs Row (Y = 82)
    int tabW = (w - 20) / 4;
    woodwindsTab.setBounds(10, 83, tabW - 55, 26);
    woodwindsMuteBtn.setBounds(10 + tabW - 50, 85, 20, 22);
    woodwindsSoloBtn.setBounds(10 + tabW - 26, 85, 20, 22);

    brassTab.setBounds(10 + tabW, 83, tabW - 55, 26);
    brassMuteBtn.setBounds(10 + tabW + tabW - 50, 85, 20, 22);
    brassSoloBtn.setBounds(10 + tabW + tabW - 26, 85, 20, 22);

    percussionTab.setBounds(10 + tabW * 2, 83, tabW - 55, 26);
    percussionMuteBtn.setBounds(10 + tabW * 2 + tabW - 50, 85, 20, 22);
    percussionSoloBtn.setBounds(10 + tabW * 2 + tabW - 26, 85, 20, 22);

    stringsTab.setBounds(10 + tabW * 3, 83, tabW - 55, 26);
    stringsMuteBtn.setBounds(10 + tabW * 3 + tabW - 50, 85, 20, 22);
    stringsSoloBtn.setBounds(10 + tabW * 3 + tabW - 26, 85, 20, 22);

    // Main Area: Left Rack (~360px) and Right Step Arranger
    int contentY = 118;
    int contentH = h - contentY - 68;
    int rackW = 350;

    rackContainer.setBounds(10, contentY, rackW, contentH);
    int rowY = 0;
    int rowH = 92;
    for (auto& row : instrumentRows) {
        if (row != nullptr) {
            row->setBounds(0, rowY, rackW, rowH);
            rowY += rowH + 4;
        }
    }

    // Right Step Arranger Area
    int rightX = rackW + 20;
    int rightW = w - rightX - 10;

    // Header controls for step grid
    voice1Btn.setBounds(rightX, contentY, 60, 22);
    voice2Btn.setBounds(rightX + 65, contentY, 60, 22);
    activeInstrumentTitle.setBounds(rightX + 135, contentY, 200, 22);

    noteGridBox.setBounds(rightX + rightW - 220, contentY, 65, 22);
    pencilBtn.setBounds(rightX + rightW - 150, contentY, 45, 22);
    eraserBtn.setBounds(rightX + rightW - 100, contentY, 45, 22);
    clearBtn.setBounds(rightX + rightW - 50, contentY, 45, 22);

    // Step grid and CC1 lane
    int laneH = 65;
    int gridY = contentY + 28;
    int gridH = contentH - 28 - laneH - 6;

    if (stepGrid != nullptr) {
        stepGrid->setBounds(rightX, gridY, rightW, gridH);
    }
    if (cc1Lane != nullptr) {
        cc1Lane->setBounds(rightX, gridY + gridH + 6, rightW, laneH);
    }

    // Bottom Bar (Y = h - 60)
    int bottomY = h - 56;
    velocityLabel.setBounds(15, bottomY + 2, 60, 18);
    velocitySlider.setBounds(75, bottomY, 180, 22);
    sigBadge.setBounds(270, bottomY, 45, 22);
    lengthBadge.setBounds(325, bottomY, 75, 22);

    if (masterDragBtn != nullptr) {
        masterDragBtn->setBounds(w - 240, bottomY, 225, 26);
    }
}
