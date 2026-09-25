#include "PluginEditor.h"

static juce::String getShortInstrumentName(Harmonic::InstrumentId id) {
    switch (id) {
        case Harmonic::InstrumentId::Violins1: return "Vln 1";
        case Harmonic::InstrumentId::Violins2: return "Vln 2";
        case Harmonic::InstrumentId::Violas: return "Vla";
        case Harmonic::InstrumentId::Cellos: return "Vc";
        case Harmonic::InstrumentId::DoubleBasses: return "Cb";
        case Harmonic::InstrumentId::Harp: return "Hrp";
        case Harmonic::InstrumentId::FrenchHorns: return "Hrn";
        case Harmonic::InstrumentId::Trumpets: return "Tpt";
        case Harmonic::InstrumentId::Trombones: return "Trb";
        case Harmonic::InstrumentId::Tuba: return "Tba";
        case Harmonic::InstrumentId::Flutes: return "Flt";
        case Harmonic::InstrumentId::Oboes: return "Ob";
        case Harmonic::InstrumentId::Clarinets: return "Cl";
        case Harmonic::InstrumentId::Bassoons: return "Bsn";
        case Harmonic::InstrumentId::Timpani: return "Timp";
        case Harmonic::InstrumentId::OrchestralPerc: return "Perc";
        case Harmonic::InstrumentId::Celesta: return "Cel";
        case Harmonic::InstrumentId::Piano: return "Pno";
        case Harmonic::InstrumentId::ChurchOrgan: return "Org";
        case Harmonic::InstrumentId::AcousticGuitar: return "AGtr";
        case Harmonic::InstrumentId::ElectricGuitar: return "EGtr";
        case Harmonic::InstrumentId::BassGuitar: return "EBass";
        case Harmonic::InstrumentId::ChoirFull: return "Chr";
        case Harmonic::InstrumentId::SynthesizerLead: return "SynL";
        case Harmonic::InstrumentId::SynthesizerPad: return "SynP";
        default: return "Inst";
    }
}

static juce::Colour getInstrumentColor(Harmonic::InstrumentId id) {
    switch (id) {
        case Harmonic::InstrumentId::Violins1: return juce::Colour(0xff00d2ff); // Cyan
        case Harmonic::InstrumentId::Violins2: return juce::Colour(0xff00b4d8); // Sky blue
        case Harmonic::InstrumentId::Violas: return juce::Colour(0xffffb703); // Warm amber
        case Harmonic::InstrumentId::Cellos: return juce::Colour(0xff52b788); // Mint green
        case Harmonic::InstrumentId::DoubleBasses: return juce::Colour(0xffb5179e); // Purple
        case Harmonic::InstrumentId::Harp: return juce::Colour(0xfff72585); // Vibrant pink
        case Harmonic::InstrumentId::FrenchHorns: return juce::Colour(0xfffb8500); // Horn orange
        case Harmonic::InstrumentId::Trumpets: return juce::Colour(0xffff5400); // Bright orange
        case Harmonic::InstrumentId::Trombones: return juce::Colour(0xffff0054); // Coral red
        case Harmonic::InstrumentId::Tuba: return juce::Colour(0xff9d4edd); // Violet
        case Harmonic::InstrumentId::Flutes: return juce::Colour(0xff48cae4); // Light cyan
        case Harmonic::InstrumentId::Oboes: return juce::Colour(0xff80b918); // Yellow-green
        case Harmonic::InstrumentId::Clarinets: return juce::Colour(0xff2ec4b6); // Teal
        case Harmonic::InstrumentId::Bassoons: return juce::Colour(0xffcb997e); // Sand/wood
        case Harmonic::InstrumentId::Timpani: return juce::Colour(0xffe63946); // Ruby red
        case Harmonic::InstrumentId::OrchestralPerc: return juce::Colour(0xffff006e); // Magenta
        case Harmonic::InstrumentId::Celesta: return juce::Colour(0xff7209b7); // Violet
        case Harmonic::InstrumentId::Piano: return juce::Colour(0xff4361ee); // Blue
        case Harmonic::InstrumentId::ChurchOrgan: return juce::Colour(0xff3a0ca3); // Deep blue
        case Harmonic::InstrumentId::AcousticGuitar: return juce::Colour(0xffd4a373); // Acoustic wood
        case Harmonic::InstrumentId::ElectricGuitar: return juce::Colour(0xffe76f51); // Terracotta
        case Harmonic::InstrumentId::BassGuitar: return juce::Colour(0xff2a9d8f); // Petrol green
        case Harmonic::InstrumentId::ChoirFull: return juce::Colour(0xffe9c46a); // Gold
        case Harmonic::InstrumentId::SynthesizerLead: return juce::Colour(0xff06d6a0); // Neon mint
        case Harmonic::InstrumentId::SynthesizerPad: return juce::Colour(0xff118ab2); // Neon blue
        default: return juce::Colour(0xff00d2ff);
    }
}

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
        processor.removeTrackStepOffset(activeInstrument, step, targetOffset);
    } else { // Pencil
        // Check if currently active at this offset
        bool isAlreadyActive = false;
        if (step < (int)currentTrack.steps.size()) {
            const auto& s = currentTrack.steps[step];
            if (s.active && (s.stepOffset == targetOffset || std::find(s.extraOffsets.begin(), s.extraOffsets.end(), targetOffset) != s.extraOffsets.end())) {
                isAlreadyActive = true;
            }
        }

        if (isAlreadyActive) {
            // Toggle off
            processor.removeTrackStepOffset(activeInstrument, step, targetOffset);
        } else {
            // Add note at this step and pitch offset
            processor.addTrackStepOffset(activeInstrument, step, targetOffset, noteVelocity, currentTrack.articulation);
        }
    }

    refreshFromPattern(processor.getCurrentPattern());
}

void StepGridComponent::mouseMove(const juce::MouseEvent& e) {
    float gridX = (float)e.x - labelWidth;
    if (gridX < 0) {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    float gridW = (float)getWidth() - labelWidth;
    float cellW = gridW / (float)numSteps;
    float cellH = (float)getHeight() / (float)numPitchRows;

    bool nearRightEdge = false;
    for (size_t s = 0; s < currentTrack.steps.size() && s < (size_t)numSteps; ++s) {
        const auto& stepDef = currentTrack.steps[s];
        if (stepDef.active && stepDef.action != Harmonic::StepActionType::Rest) {
            std::vector<int> allOffsets = { stepDef.stepOffset };
            for (int eo : stepDef.extraOffsets) allOffsets.push_back(eo);

            for (int offVal : allOffsets) {
                int row = getRowForPitchOffset(offVal);
                if (row >= 0 && row < numPitchRows) {
                    float startX = labelWidth + s * cellW;
                    int len = std::clamp(stepDef.lengthSteps, 1, 16 - (int)s);
                    float endX = startX + len * cellW;
                    float y = row * cellH;

                    if (e.x >= endX - 8.0f && e.x <= endX + 4.0f && e.y >= y && e.y <= y + cellH) {
                        nearRightEdge = true;
                        break;
                    }
                }
            }
            if (nearRightEdge) break;
        }
    }

    if (nearRightEdge) {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    } else {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void StepGridComponent::mouseDown(const juce::MouseEvent& e) {
    float gridX = (float)e.x - labelWidth;
    if (gridX < 0) return;

    float gridW = (float)getWidth() - labelWidth;
    float cellW = gridW / (float)numSteps;
    float cellH = (float)getHeight() / (float)numPitchRows;

    // Check if clicking near right edge of an existing active note to resize
    for (size_t s = 0; s < currentTrack.steps.size() && s < (size_t)numSteps; ++s) {
        const auto& stepDef = currentTrack.steps[s];
        if (stepDef.active && stepDef.action != Harmonic::StepActionType::Rest) {
            std::vector<int> allOffsets = { stepDef.stepOffset };
            for (int eo : stepDef.extraOffsets) allOffsets.push_back(eo);

            for (int offVal : allOffsets) {
                int row = getRowForPitchOffset(offVal);
                if (row >= 0 && row < numPitchRows) {
                    float startX = labelWidth + s * cellW;
                    int len = std::clamp(stepDef.lengthSteps, 1, 16 - (int)s);
                    float endX = startX + len * cellW;
                    float y = row * cellH;

                    if (e.x >= endX - 8.0f && e.x <= endX + 4.0f && e.y >= y && e.y <= y + cellH) {
                        isResizing = true;
                        resizeStep = (int)s;
                        originalLength = len;
                        dragStartX = startX;
                        return;
                    }
                }
            }
        }
    }

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

    if (isResizing && resizeStep >= 0) {
        float currentX = (float)e.x;
        float diffX = currentX - dragStartX;
        int newLength = std::clamp(static_cast<int>(std::round(diffX / cellW)), 1, 16 - resizeStep);
        if (resizeStep < (int)currentTrack.steps.size()) {
            currentTrack.steps[resizeStep].lengthSteps = newLength;
            processor.setTrackStepLength(activeInstrument, resizeStep, newLength);
            repaint();
        }
        return;
    }

    int step = static_cast<int>(gridX / cellW);
    int row = static_cast<int>((float)e.y / cellH);

    if (step >= 0 && step < numSteps && row >= 0 && row < numPitchRows) {
        int targetOffset = getPitchOffsetForRow(row);
        if (currentTool == 1) {
            processor.removeTrackStepOffset(activeInstrument, step, targetOffset);
        } else {
            processor.addTrackStepOffset(activeInstrument, step, targetOffset, noteVelocity, currentTrack.articulation);
        }
        refreshFromPattern(processor.getCurrentPattern());
    }
}

void StepGridComponent::mouseUp(const juce::MouseEvent&) {
    if (isResizing) {
        isResizing = false;
        resizeStep = -1;
        refreshFromPattern(processor.getCurrentPattern());
        setMouseCursor(juce::MouseCursor::NormalCursor);
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

    // -------------------------------------------------------------
    // GHOST NOTES: Draw translucent notes of ALL OTHER TRACKS in the active section!
    // -------------------------------------------------------------
    auto fullPattern = processor.getCurrentPattern();
    Harmonic::OrchestralSection curSec = Harmonic::getInstrumentSection(activeInstrument);

    for (const auto& [trackInst, trk] : fullPattern.tracks) {
        if (trackInst == activeInstrument) continue;
        Harmonic::OrchestralSection instSec = Harmonic::getInstrumentSection(trackInst);
        if (instSec != curSec) continue;

        juce::Colour ghostCol = getInstrumentColor(trackInst);
        juce::String instShort = getShortInstrumentName(trackInst);

        for (size_t s = 0; s < trk.steps.size() && s < (size_t)numSteps; ++s) {
            const auto& sDef = trk.steps[s];
            if (sDef.active && sDef.action != Harmonic::StepActionType::Rest) {
                std::vector<int> allOffsets = { sDef.stepOffset };
                for (int eo : sDef.extraOffsets) allOffsets.push_back(eo);

                for (int offVal : allOffsets) {
                    int row = getRowForPitchOffset(offVal);
                    if (row >= 0 && row < numPitchRows) {
                        float gx = labelWidth + s * cellW + 1.5f;
                        float gy = row * cellH + 1.5f;
                        int gLen = std::clamp(sDef.lengthSteps, 1, 16 - (int)s);
                        float gBlockW = (gLen * cellW) - 3.0f;
                        float gBlockH = cellH - 3.0f;

                        // Translucent body
                        g.setColour(ghostCol.withAlpha(0.24f));
                        g.fillRoundedRectangle(gx, gy, gBlockW, gBlockH, 3.0f);

                        // Translucent subtle outline
                        g.setColour(ghostCol.withAlpha(0.50f));
                        g.drawRoundedRectangle(gx, gy, gBlockW, gBlockH, 3.0f, 0.75f);

                        // Instrument name badge in ghost note
                        g.setFont(juce::Font(8.0f));
                        g.setColour(ghostCol.withAlpha(0.75f));
                        g.drawText(instShort, (int)gx + 3, (int)gy, (int)gBlockW - 6, (int)gBlockH, juce::Justification::centredLeft);
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------
    // ACTIVE TRACK NOTES: Vivid solid color with resize handles
    // -------------------------------------------------------------
    juce::Colour activeColor = getInstrumentColor(activeInstrument);

    for (size_t s = 0; s < currentTrack.steps.size() && s < (size_t)numSteps; ++s) {
        const auto& stepDef = currentTrack.steps[s];
        if (stepDef.active && stepDef.action != Harmonic::StepActionType::Rest) {
            std::vector<int> allOffsets = { stepDef.stepOffset };
            for (int eo : stepDef.extraOffsets) allOffsets.push_back(eo);

            for (int offVal : allOffsets) {
                int row = getRowForPitchOffset(offVal);
                if (row >= 0 && row < numPitchRows) {
                    float x = labelWidth + s * cellW + 1.5f;
                    float y = row * cellH + 1.5f;
                    int len = std::clamp(stepDef.lengthSteps, 1, 16 - (int)s);
                    float blockW = (len * cellW) - 3.0f;
                    float blockH = cellH - 3.0f;

                    // Glowing note block
                    g.setColour(activeColor.withAlpha(0.92f));
                    g.fillRoundedRectangle(x, y, blockW, blockH, 3.0f);

                    g.setColour(juce::Colours::white);
                    g.drawRoundedRectangle(x, y, blockW, blockH, 3.0f, 1.2f);

                    // Right-edge resize grip handle
                    float gripX = x + blockW - 5.0f;
                    g.setColour(juce::Colours::white.withAlpha(0.85f));
                    g.drawLine(gripX, y + 3.0f, gripX, y + blockH - 3.0f, 1.5f);
                    g.drawLine(gripX + 2.0f, y + 4.0f, gripX + 2.0f, y + blockH - 4.0f, 1.0f);

                    // Pitch offset label
                    juce::String txt = offVal > 0 ? "+" + juce::String(offVal) : (offVal == 0 ? "0" : juce::String(offVal));
                    g.setFont(juce::Font(9.0f, juce::Font::bold));
                    g.setColour(juce::Colours::white);
                    g.drawText(txt, (int)x + 3, (int)y, 22, (int)blockH, juce::Justification::centredLeft);
                }
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
// MixerChannelStrip Implementation
// -------------------------------------------------------------
MixerChannelStrip::MixerChannelStrip(AutomaticOrchestratorAudioProcessor& p,
                                     Harmonic::InstrumentId instId,
                                     int chNum)
    : processor(p), instrument(instId), channelNumber(chNum)
{
    chBadge.setText("CH " + juce::String(channelNumber), juce::dontSendNotification);
    chBadge.setFont(juce::Font(9.0f, juce::Font::bold));
    chBadge.setColour(juce::Label::textColourId, juce::Colour(0xff718096));
    chBadge.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(chBadge);

    nameLabel.setText(getShortInstrumentName(instrument), juce::dontSendNotification);
    nameLabel.setFont(juce::Font(10.5f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    nameLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(nameLabel);

    Harmonic::OrchestralSection sec = Harmonic::getInstrumentSection(instrument);
    juce::String secStr = "STR";
    if (sec == Harmonic::OrchestralSection::Brass) secStr = "BRS";
    else if (sec == Harmonic::OrchestralSection::Woodwinds) secStr = "WND";
    else if (sec == Harmonic::OrchestralSection::Percussion) secStr = "PRC";
    else if (sec == Harmonic::OrchestralSection::Keyboards) secStr = "KEY";
    else if (sec == Harmonic::OrchestralSection::Guitars) secStr = "GTR";
    else if (sec == Harmonic::OrchestralSection::Choir) secStr = "CHR";
    else if (sec == Harmonic::OrchestralSection::Synths) secStr = "SYN";

    sectionBadge.setText(secStr, juce::dontSendNotification);
    sectionBadge.setFont(juce::Font(8.0f, juce::Font::bold));
    sectionBadge.setColour(juce::Label::textColourId, getInstrumentColor(instrument));
    sectionBadge.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(sectionBadge);

    // Pan slider (Rotary)
    panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider.setRange(-1.0, 1.0, 0.05);
    panSlider.setValue(0.0);
    panSlider.onValueChange = [this]() {
        processor.setTrackPan(instrument, (float)panSlider.getValue());
    };
    addAndMakeVisible(panSlider);

    // Mute / Solo
    muteBtn.setClickingTogglesState(true);
    muteBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffe53e3e));
    muteBtn.onClick = [this]() {
        processor.setTrackMute(instrument, muteBtn.getToggleState());
    };
    addAndMakeVisible(muteBtn);

    soloBtn.setClickingTogglesState(true);
    soloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffecc94b));
    soloBtn.onClick = [this]() {
        processor.setTrackSolo(instrument, soloBtn.getToggleState());
    };
    addAndMakeVisible(soloBtn);

    // Volume Slider (Vertical Linear Fader)
    volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setRange(0.0, 1.25, 0.01);
    volumeSlider.setValue(0.85);
    volumeSlider.onValueChange = [this]() {
        float val = (float)volumeSlider.getValue();
        processor.setTrackVolume(instrument, val);
        float db = (val > 0.0001f) ? (20.0f * std::log10(val)) : -60.0f;
        if (db < -59.0f) dbLabel.setText("-inf dB", juce::dontSendNotification);
        else dbLabel.setText(juce::String(db, 1) + " dB", juce::dontSendNotification);
    };
    addAndMakeVisible(volumeSlider);

    dbLabel.setText("-1.4 dB", juce::dontSendNotification);
    dbLabel.setFont(juce::Font(9.0f));
    dbLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa0aec0));
    dbLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(dbLabel);
}

void MixerChannelStrip::setMeterLevel(float level) {
    currentMeterLevel = std::clamp(level, 0.0f, 1.0f);
    repaint();
}

void MixerChannelStrip::refreshFromTrack(const Sequencer::TrackPattern& track) {
    panSlider.setValue(track.pan, juce::dontSendNotification);
    volumeSlider.setValue(track.volume, juce::dontSendNotification);
    muteBtn.setToggleState(track.isMuted, juce::dontSendNotification);
    soloBtn.setToggleState(track.isSolo, juce::dontSendNotification);

    float db = (track.volume > 0.0001f) ? (20.0f * std::log10(track.volume)) : -60.0f;
    if (db < -59.0f) dbLabel.setText("-inf dB", juce::dontSendNotification);
    else dbLabel.setText(juce::String(db, 1) + " dB", juce::dontSendNotification);
}

void MixerChannelStrip::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    // Strip background
    g.setColour(juce::Colour(0xff14171e));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(juce::Colour(0xff232732));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Section color top accent bar
    g.setColour(getInstrumentColor(instrument));
    g.fillRect(bounds.getX(), bounds.getY(), bounds.getWidth(), 3.0f);

    // Meter slot background
    float meterX = bounds.getX() + bounds.getWidth() - 12.0f;
    float meterY = bounds.getY() + 130.0f;
    float meterW = 5.0f;
    float meterH = bounds.getHeight() - 165.0f;

    g.setColour(juce::Colour(0xff090b0e));
    g.fillRect(meterX, meterY, meterW, meterH);

    // Meter active LED bar
    if (currentMeterLevel > 0.01f) {
        float fillH = meterH * currentMeterLevel;
        float fillY = meterY + meterH - fillH;

        juce::ColourGradient grad(juce::Colour(0xff00e676), meterX, meterY + meterH,
                                  juce::Colour(0xffff1744), meterX, meterY, false);
        grad.addColour(0.7, juce::Colour(0xffffea00));
        g.setGradientFill(grad);
        g.fillRect(meterX, fillY, meterW, fillH);
    }
}

void MixerChannelStrip::resized() {
    int w = getWidth();
    int h = getHeight();

    chBadge.setBounds(2, 6, w - 4, 14);
    nameLabel.setBounds(2, 22, w - 4, 16);
    sectionBadge.setBounds(w / 2 - 16, 40, 32, 12);

    panSlider.setBounds(w / 2 - 18, 56, 36, 36);

    muteBtn.setBounds(w / 2 - 24, 96, 22, 18);
    soloBtn.setBounds(w / 2 + 2, 96, 22, 18);

    volumeSlider.setBounds(6, 124, w - 20, h - 162);
    dbLabel.setBounds(2, h - 26, w - 4, 16);
}

// -------------------------------------------------------------
// OrchestralMixerComponent Implementation
// -------------------------------------------------------------
OrchestralMixerComponent::OrchestralMixerComponent(AutomaticOrchestratorAudioProcessor& p)
    : processor(p)
{
    // Build 16 orchestral channel strips
    std::vector<Harmonic::InstrumentId> instOrder = {
        Harmonic::InstrumentId::Violins1,
        Harmonic::InstrumentId::Violins2,
        Harmonic::InstrumentId::Violas,
        Harmonic::InstrumentId::Cellos,
        Harmonic::InstrumentId::DoubleBasses,
        Harmonic::InstrumentId::FrenchHorns,
        Harmonic::InstrumentId::Trumpets,
        Harmonic::InstrumentId::Trombones,
        Harmonic::InstrumentId::Tuba,
        Harmonic::InstrumentId::Flutes,
        Harmonic::InstrumentId::Oboes,
        Harmonic::InstrumentId::Clarinets,
        Harmonic::InstrumentId::Bassoons,
        Harmonic::InstrumentId::Timpani,
        Harmonic::InstrumentId::OrchestralPerc
    };

    int ch = 1;
    for (auto id : instOrder) {
        auto strip = std::make_unique<MixerChannelStrip>(processor, id, ch++);
        addAndMakeVisible(*strip);
        strips.push_back(std::move(strip));
    }

    // Channel 16: Extra
    auto strip16 = std::make_unique<MixerChannelStrip>(processor, Harmonic::InstrumentId::Violins1, 16);
    addAndMakeVisible(*strip16);
    strips.push_back(std::move(strip16));

    // Master Bus Controls
    masterTitle.setFont(juce::Font(11.0f, juce::Font::bold));
    masterTitle.setColour(juce::Label::textColourId, juce::Colour(0xff00d2ff));
    masterTitle.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(masterTitle);

    masterFader.setSliderStyle(juce::Slider::LinearVertical);
    masterFader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    masterFader.setRange(0.0, 1.25, 0.01);
    masterFader.setValue(1.0);
    masterFader.onValueChange = [this]() {
        float val = (float)masterFader.getValue();
        float db = (val > 0.0001f) ? (20.0f * std::log10(val)) : -60.0f;
        if (db < -59.0f) masterDbLabel.setText("-inf dB", juce::dontSendNotification);
        else masterDbLabel.setText(juce::String(db, 1) + " dB", juce::dontSendNotification);
    };
    addAndMakeVisible(masterFader);

    masterDbLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    masterDbLabel.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    masterDbLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(masterDbLabel);

    masterDragBtn = std::make_unique<MidiDragComponent>(processor, std::nullopt, "EXPORT MIDI");
    addAndMakeVisible(*masterDragBtn);
}

void OrchestralMixerComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff0c0e12));

    // Master bus backing panel
    int masterX = getWidth() - 95;
    auto mBounds = juce::Rectangle<float>((float)masterX, 4.0f, 90.0f, (float)getHeight() - 8.0f);
    g.setColour(juce::Colour(0xff161a22));
    g.fillRoundedRectangle(mBounds, 6.0f);
    g.setColour(juce::Colour(0xff00d2ff).withAlpha(0.6f));
    g.drawRoundedRectangle(mBounds, 6.0f, 1.0f);

    // Master Meter
    float meterX = mBounds.getX() + mBounds.getWidth() - 16.0f;
    float meterY = mBounds.getY() + 45.0f;
    float meterW = 7.0f;
    float meterH = mBounds.getHeight() - 95.0f;

    g.setColour(juce::Colour(0xff090b0e));
    g.fillRect(meterX, meterY, meterW, meterH);

    if (masterMeterLevel > 0.01f) {
        float fillH = meterH * masterMeterLevel;
        float fillY = meterY + meterH - fillH;
        juce::ColourGradient grad(juce::Colour(0xff00e676), meterX, meterY + meterH,
                                  juce::Colour(0xffff1744), meterX, meterY, false);
        grad.addColour(0.7, juce::Colour(0xffffea00));
        g.setGradientFill(grad);
        g.fillRect(meterX, fillY, meterW, fillH);
    }
}

void OrchestralMixerComponent::resized() {
    int w = getWidth();
    int h = getHeight();

    int masterW = 90;
    int masterX = w - masterW - 4;

    masterTitle.setBounds(masterX + 4, 8, masterW - 8, 18);
    masterFader.setBounds(masterX + 8, 38, 48, h - 90);
    masterDbLabel.setBounds(masterX + 4, h - 46, masterW - 8, 16);
    if (masterDragBtn) {
        masterDragBtn->setBounds(masterX + 6, h - 28, masterW - 12, 22);
    }

    // Available width for 16 strips
    int stripsAreaW = masterX - 8;
    int stripW = std::max(56, stripsAreaW / 16);

    for (int i = 0; i < (int)strips.size(); ++i) {
        int sx = 4 + i * stripW;
        strips[i]->setBounds(sx, 4, stripW - 4, h - 8);
    }
}

void OrchestralMixerComponent::refreshFromPattern(const Sequencer::OrchestralPattern& pattern) {
    for (auto& s : strips) {
        auto inst = s->getInstrumentId();
        if (pattern.tracks.find(inst) != pattern.tracks.end()) {
            s->refreshFromTrack(pattern.tracks.at(inst));
        }
    }
    repaint();
}

void OrchestralMixerComponent::updateMeters(int currentStep) {
    auto pattern = processor.getCurrentPattern();
    float masterMax = 0.0f;

    for (auto& s : strips) {
        auto inst = s->getInstrumentId();
        float target = 0.0f;
        if (pattern.tracks.find(inst) != pattern.tracks.end()) {
            const auto& trk = pattern.tracks.at(inst);
            if (!trk.isMuted && !trk.steps.empty() && currentStep >= 0) {
                const auto& step = trk.steps[currentStep % trk.steps.size()];
                if (step.active && step.action != Harmonic::StepActionType::Rest) {
                    target = (step.velocity / 127.0f) * trk.volume;
                }
            }
        }
        s->setMeterLevel(target);
        masterMax = std::max(masterMax, target);
    }

    masterMeterLevel = masterMax;
    repaint();
}

// -------------------------------------------------------------
// HollywoodOrchestratorEditor Implementation
// -------------------------------------------------------------
HollywoodOrchestratorEditor::HollywoodOrchestratorEditor(AutomaticOrchestratorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Mode toggles
    mainModeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3748));
    mainModeBtn.onClick = [this]() { updateViewMode(false); };
    addAndMakeVisible(mainModeBtn);

    mixerModeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a202c));
    mixerModeBtn.onClick = [this]() { updateViewMode(true); };
    addAndMakeVisible(mixerModeBtn);

    // Preset navigation & actions
    prevPresetBtn.onClick = [this]() {
        int id = presetSelector.getSelectedId();
        if (id > 1) presetSelector.setSelectedId(id - 1);
        else presetSelector.setSelectedId(presetSelector.getNumItems());
    };
    addAndMakeVisible(prevPresetBtn);

    populatePresetSelector();
    presetSelector.setSelectedId(1, juce::dontSendNotification);
    presetSelector.onChange = [this]() {
        int id = presetSelector.getSelectedId();
        if (id <= 0) return;

        if (id == 1) audioProcessor.setStylePattern(Sequencer::createActionOstinatoPattern());
        else if (id == 2) audioProcessor.setStylePattern(Sequencer::createEpicFanfarePattern());
        else if (id == 3) audioProcessor.setStylePattern(Sequencer::createLyricalAdagioPattern());
        else if (id == 4) audioProcessor.setStylePattern(Sequencer::createSuspenseMysteryPattern());
        else if (id == 5) audioProcessor.setStylePattern(Sequencer::createWarDrumsPattern());
        else if (id == 6) audioProcessor.setStylePattern(Sequencer::createFantasyAdventurePattern());
        else if (id >= 100) {
            juce::File presetDir = audioProcessor.getPresetsFolder();
            juce::String presetName = presetSelector.getText();
            juce::File file = presetDir.getChildFile(presetName + ".json");
            if (file.existsAsFile()) {
                audioProcessor.loadPresetFromFile(file);
            }
        }

        loadCurrentPatternIntoUi();
        if (mixerComponent != nullptr && isMixerView) {
            mixerComponent->refreshFromPattern(audioProcessor.getCurrentPattern());
        }
    };
    addAndMakeVisible(presetSelector);

    nextPresetBtn.onClick = [this]() {
        int id = presetSelector.getSelectedId();
        if (id < presetSelector.getNumItems()) presetSelector.setSelectedId(id + 1);
        else presetSelector.setSelectedId(1);
    };
    addAndMakeVisible(nextPresetBtn);

    savePresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3748));
    savePresetBtn.onClick = [this]() { saveCurrentPreset(); };
    addAndMakeVisible(savePresetBtn);

    saveAsPresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3748));
    saveAsPresetBtn.onClick = [this]() { saveAsNewPreset(); };
    addAndMakeVisible(saveAsPresetBtn);

    // Live Chord Recognition Badge
    chordDisplayBadge.setText("READY", juce::dontSendNotification);
    chordDisplayBadge.setFont(juce::Font(20.0f, juce::Font::bold));
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

    addInstrumentBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e232d));
    addInstrumentBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00d2ff));
    addInstrumentBtn.onClick = [this]() { promptAddInstrument(); };

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

    // Orchestral Mixer Component
    mixerComponent = std::make_unique<OrchestralMixerComponent>(audioProcessor);
    addChildComponent(*mixerComponent);

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
    updateViewMode(false);

    setSize(1080, 720);
    startTimerHz(30);
}

HollywoodOrchestratorEditor::~HollywoodOrchestratorEditor() {
    stopTimer();
}

void HollywoodOrchestratorEditor::updateViewMode(bool mixerView) {
    isMixerView = mixerView;

    mainModeBtn.setColour(juce::TextButton::buttonColourId, isMixerView ? juce::Colour(0xff1a202c) : juce::Colour(0xff2d3748));
    mainModeBtn.setColour(juce::TextButton::textColourOffId, isMixerView ? juce::Colour(0xff718096) : juce::Colours::white);

    mixerModeBtn.setColour(juce::TextButton::buttonColourId, isMixerView ? juce::Colour(0xff2d3748) : juce::Colour(0xff1a202c));
    mixerModeBtn.setColour(juce::TextButton::textColourOffId, isMixerView ? juce::Colours::white : juce::Colour(0xff718096));

    if (mixerComponent != nullptr) {
        mixerComponent->setVisible(isMixerView);
        if (isMixerView) {
            mixerComponent->refreshFromPattern(audioProcessor.getCurrentPattern());
        }
    }

    // Toggle Arranger Main View components
    bool showArranger = !isMixerView;
    woodwindsTab.setVisible(showArranger);
    woodwindsMuteBtn.setVisible(showArranger);
    woodwindsSoloBtn.setVisible(showArranger);
    brassTab.setVisible(showArranger);
    brassMuteBtn.setVisible(showArranger);
    brassSoloBtn.setVisible(showArranger);
    percussionTab.setVisible(showArranger);
    percussionMuteBtn.setVisible(showArranger);
    percussionSoloBtn.setVisible(showArranger);
    stringsTab.setVisible(showArranger);
    stringsMuteBtn.setVisible(showArranger);
    stringsSoloBtn.setVisible(showArranger);

    rackContainer.setVisible(showArranger);
    voice1Btn.setVisible(showArranger);
    voice2Btn.setVisible(showArranger);
    activeInstrumentTitle.setVisible(showArranger);
    noteGridBox.setVisible(showArranger);
    pencilBtn.setVisible(showArranger);
    eraserBtn.setVisible(showArranger);
    clearBtn.setVisible(showArranger);
    if (stepGrid != nullptr) stepGrid->setVisible(showArranger);
    if (cc1Lane != nullptr) cc1Lane->setVisible(showArranger);

    resized();
    repaint();
}

void HollywoodOrchestratorEditor::populatePresetSelector() {
    presetSelector.clear(juce::dontSendNotification);

    // Factory Presets
    presetSelector.addItem("Action Ostinato", 1);
    presetSelector.addItem("Epic Brass Fanfare", 2);
    presetSelector.addItem("Lyrical Adagio", 3);
    presetSelector.addItem("Suspense Mystery", 4);
    presetSelector.addItem("War Drums & Percussion", 5);
    presetSelector.addItem("Fantasy Adventure", 6);

    // User Presets from presets directory
    juce::File presetDir = audioProcessor.getPresetsFolder();
    juce::Array<juce::File> files = presetDir.findChildFiles(juce::File::findFiles, false, "*.json");

    int userPresetId = 100;
    for (const auto& f : files) {
        presetSelector.addItem(f.getFileNameWithoutExtension(), userPresetId++);
    }
}

void HollywoodOrchestratorEditor::saveCurrentPreset() {
    int id = presetSelector.getSelectedId();
    juce::String currentName = presetSelector.getText();

    if (id >= 100 && currentName.isNotEmpty()) {
        juce::File presetDir = audioProcessor.getPresetsFolder();
        juce::File file = presetDir.getChildFile(currentName + ".json");
        audioProcessor.savePresetToFile(file, currentName);
    } else {
        saveAsNewPreset();
    }
}

void HollywoodOrchestratorEditor::saveAsNewPreset() {
    auto* aw = new juce::AlertWindow("Save Orchestral Preset", "Enter a name for the new preset:", juce::AlertWindow::QuestionIcon);
    aw->addTextEditor("presetName", presetSelector.getText().isEmpty() ? "Custom Preset" : presetSelector.getText());
    aw->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    aw->enterModalState(true, juce::ModalCallbackFunction::create([this, aw](int result) {
        if (result == 1) {
            juce::String name = aw->getTextEditorContents("presetName").trim();
            if (name.isNotEmpty()) {
                juce::File presetDir = audioProcessor.getPresetsFolder();
                juce::File file = presetDir.getChildFile(name + ".json");
                audioProcessor.savePresetToFile(file, name);
                populatePresetSelector();
                presetSelector.setText(name, juce::dontSendNotification);
            }
        }
    }), true);
}

void HollywoodOrchestratorEditor::promptAddInstrument() {
    juce::PopupMenu menu;

    juce::String secName = (activeSection == Harmonic::OrchestralSection::Strings) ? "Strings" :
                           (activeSection == Harmonic::OrchestralSection::Brass) ? "Brass" :
                           (activeSection == Harmonic::OrchestralSection::Woodwinds) ? "Woodwinds" :
                           (activeSection == Harmonic::OrchestralSection::Percussion) ? "Percussion" :
                           (activeSection == Harmonic::OrchestralSection::Keyboards) ? "Keyboards" :
                           (activeSection == Harmonic::OrchestralSection::Guitars) ? "Guitars" :
                           (activeSection == Harmonic::OrchestralSection::Choir) ? "Choir" : "Synths";
    menu.addSectionHeader("Add Instrument to " + secName);

    struct InstOption {
        Harmonic::InstrumentId id;
        Harmonic::OrchestralSection sec;
    };

    std::vector<InstOption> options = {
        {Harmonic::InstrumentId::Violins1, Harmonic::OrchestralSection::Strings},
        {Harmonic::InstrumentId::Violins2, Harmonic::OrchestralSection::Strings},
        {Harmonic::InstrumentId::Violas, Harmonic::OrchestralSection::Strings},
        {Harmonic::InstrumentId::Cellos, Harmonic::OrchestralSection::Strings},
        {Harmonic::InstrumentId::DoubleBasses, Harmonic::OrchestralSection::Strings},
        {Harmonic::InstrumentId::Harp, Harmonic::OrchestralSection::Strings},

        {Harmonic::InstrumentId::FrenchHorns, Harmonic::OrchestralSection::Brass},
        {Harmonic::InstrumentId::Trumpets, Harmonic::OrchestralSection::Brass},
        {Harmonic::InstrumentId::Trombones, Harmonic::OrchestralSection::Brass},
        {Harmonic::InstrumentId::Tuba, Harmonic::OrchestralSection::Brass},

        {Harmonic::InstrumentId::Flutes, Harmonic::OrchestralSection::Woodwinds},
        {Harmonic::InstrumentId::Oboes, Harmonic::OrchestralSection::Woodwinds},
        {Harmonic::InstrumentId::Clarinets, Harmonic::OrchestralSection::Woodwinds},
        {Harmonic::InstrumentId::Bassoons, Harmonic::OrchestralSection::Woodwinds},

        {Harmonic::InstrumentId::Timpani, Harmonic::OrchestralSection::Percussion},
        {Harmonic::InstrumentId::OrchestralPerc, Harmonic::OrchestralSection::Percussion},
        {Harmonic::InstrumentId::Celesta, Harmonic::OrchestralSection::Percussion},

        {Harmonic::InstrumentId::Piano, Harmonic::OrchestralSection::Keyboards},
        {Harmonic::InstrumentId::ChurchOrgan, Harmonic::OrchestralSection::Keyboards},

        {Harmonic::InstrumentId::AcousticGuitar, Harmonic::OrchestralSection::Guitars},
        {Harmonic::InstrumentId::ElectricGuitar, Harmonic::OrchestralSection::Guitars},
        {Harmonic::InstrumentId::BassGuitar, Harmonic::OrchestralSection::Guitars},

        {Harmonic::InstrumentId::ChoirFull, Harmonic::OrchestralSection::Choir},

        {Harmonic::InstrumentId::SynthesizerLead, Harmonic::OrchestralSection::Synths},
        {Harmonic::InstrumentId::SynthesizerPad, Harmonic::OrchestralSection::Synths}
    };

    int menuId = 1;
    std::map<int, Harmonic::InstrumentId> idMap;

    for (const auto& opt : options) {
        if (opt.sec == activeSection) {
            menu.addItem(menuId, Harmonic::instrumentToString(opt.id));
            idMap[menuId] = opt.id;
            menuId++;
        }
    }

    juce::PopupMenu otherMenu;
    for (const auto& opt : options) {
        if (opt.sec != activeSection) {
            otherMenu.addItem(menuId, Harmonic::instrumentToString(opt.id));
            idMap[menuId] = opt.id;
            menuId++;
        }
    }
    menu.addSubMenu("Other Orchestral Instruments", otherMenu);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&addInstrumentBtn),
        [this, idMap](int result) {
            if (result <= 0) return;
            auto it = idMap.find(result);
            if (it != idMap.end()) {
                Harmonic::InstrumentId inst = it->second;
                int defaultChan = Harmonic::getDefaultInstrumentChannel(inst);
                audioProcessor.addTrack(inst, Harmonic::instrumentToString(inst), activeSection, defaultChan, Harmonic::ArticulationType::Sustain);
                switchSection(activeSection);
                selectInstrument(inst);
                loadCurrentPatternIntoUi();
                if (mixerComponent != nullptr && isMixerView) {
                    mixerComponent->refreshFromPattern(audioProcessor.getCurrentPattern());
                }
            }
        });
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

    auto pat = audioProcessor.getCurrentPattern();
    std::vector<Harmonic::InstrumentId> insts;

    for (const auto& [inst, trk] : pat.tracks) {
        if (trk.section == section) {
            insts.push_back(inst);
        }
    }

    if (insts.empty()) {
        if (section == Harmonic::OrchestralSection::Strings) {
            insts = {
                Harmonic::InstrumentId::Violins1,
                Harmonic::InstrumentId::Violins2,
                Harmonic::InstrumentId::Violas,
                Harmonic::InstrumentId::Cellos,
                Harmonic::InstrumentId::DoubleBasses,
                Harmonic::InstrumentId::Harp
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
        } else if (section == Harmonic::OrchestralSection::Percussion) {
            insts = {
                Harmonic::InstrumentId::Timpani,
                Harmonic::InstrumentId::OrchestralPerc,
                Harmonic::InstrumentId::Celesta
            };
        } else if (section == Harmonic::OrchestralSection::Keyboards) {
            insts = {
                Harmonic::InstrumentId::Piano,
                Harmonic::InstrumentId::ChurchOrgan
            };
        } else if (section == Harmonic::OrchestralSection::Guitars) {
            insts = {
                Harmonic::InstrumentId::AcousticGuitar,
                Harmonic::InstrumentId::ElectricGuitar,
                Harmonic::InstrumentId::BassGuitar
            };
        } else if (section == Harmonic::OrchestralSection::Choir) {
            insts = {
                Harmonic::InstrumentId::ChoirFull
            };
        } else if (section == Harmonic::OrchestralSection::Synths) {
            insts = {
                Harmonic::InstrumentId::SynthesizerLead,
                Harmonic::InstrumentId::SynthesizerPad
            };
        }
    }

    for (auto id : insts) {
        auto row = std::make_unique<InstrumentRowComponent>(audioProcessor, id, [this](Harmonic::InstrumentId selectedId) {
            selectInstrument(selectedId);
        });
        rackContainer.addAndMakeVisible(*row);
        instrumentRows.push_back(std::move(row));
    }

    rackContainer.addAndMakeVisible(addInstrumentBtn);

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
    if (mixerComponent != nullptr && isMixerView) {
        mixerComponent->updateMeters(step);
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

    // Section bar background (only in main view)
    if (!isMixerView) {
        g.setColour(juce::Colour(0xff14171e));
        g.fillRect(0, 79, getWidth(), 34);
        g.setColour(juce::Colour(0xff232730));
        g.drawHorizontalLine(113, 0.0f, (float)getWidth());
    }

    // Bottom Branding
    g.setColour(juce::Colour(0xff4a5568));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("AUTOMATIC ORCHESTRATOR", getWidth() / 2 - 120, getHeight() - 25, 240, 20, juce::Justification::centred);
}

void HollywoodOrchestratorEditor::resized() {
    int w = getWidth();
    int h = getHeight();

    // Top Row Controls
    mainModeBtn.setBounds(15, 48, 50, 22);
    mixerModeBtn.setBounds(68, 48, 50, 22);

    prevPresetBtn.setBounds(124, 48, 22, 22);
    presetSelector.setBounds(148, 48, 145, 22);
    nextPresetBtn.setBounds(295, 48, 22, 22);

    savePresetBtn.setBounds(320, 48, 45, 22);
    saveAsPresetBtn.setBounds(368, 48, 68, 22);

    chordDisplayBadge.setBounds(w / 2 - 75, 15, 150, 48);
    tempoBadge.setBounds(w - 380, 48, 75, 22);
    librarySelector.setBounds(w - 295, 48, 175, 22);
    voicingSelector.setBounds(w - 110, 48, 95, 22);

    if (isMixerView) {
        if (mixerComponent != nullptr) {
            mixerComponent->setBounds(10, 82, w - 20, h - 82 - 62);
        }
    } else {
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

        // Main Area: Left Rack (~350px) and Right Step Arranger
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
        addInstrumentBtn.setBounds(0, rowY + 2, rackW, 24);

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
    }

    // Bottom Bar (Y = h - 56)
    int bottomY = h - 56;
    velocityLabel.setBounds(15, bottomY + 2, 60, 18);
    velocitySlider.setBounds(75, bottomY, 180, 22);
    sigBadge.setBounds(270, bottomY, 45, 22);
    lengthBadge.setBounds(325, bottomY, 75, 22);

    if (masterDragBtn != nullptr) {
        masterDragBtn->setBounds(w - 240, bottomY, 225, 26);
    }
}
