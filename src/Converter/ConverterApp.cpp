#include "ConverterApp.h"
#include <iomanip>

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
        case Harmonic::ArticulationType::Sustain: artId = 1; break;
        case Harmonic::ArticulationType::Staccato: artId = 2; break;
        case Harmonic::ArticulationType::Spiccato: artId = 3; break;
        case Harmonic::ArticulationType::Marcato: artId = 4; break;
        case Harmonic::ArticulationType::Tremolo: artId = 5; break;
        case Harmonic::ArticulationType::Pizzicato: artId = 6; break;
        case Harmonic::ArticulationType::Runs: artId = 7; break;
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

    juce::Colour bgCol;
    juce::String name;
    switch (sec) {
        case Harmonic::OrchestralSection::Strings:
            name = "STRINGS";
            bgCol = juce::Colour(0xffe53e3e);
            break;
        case Harmonic::OrchestralSection::Brass:
            name = "BRASS";
            bgCol = juce::Colour(0xffd69e2e);
            break;
        case Harmonic::OrchestralSection::Woodwinds:
            name = "WOODWINDS";
            bgCol = juce::Colour(0xff38a169);
            break;
        case Harmonic::OrchestralSection::Percussion:
            name = "PERCUSSION";
            bgCol = juce::Colour(0xff805ad5);
            break;
        case Harmonic::OrchestralSection::Keyboards:
            name = "KEYBOARDS";
            bgCol = juce::Colour(0xff3182ce);
            break;
        case Harmonic::OrchestralSection::Guitars:
            name = "GUITARS";
            bgCol = juce::Colour(0xffdd6b20);
            break;
        case Harmonic::OrchestralSection::Choir:
            name = "CHOIR";
            bgCol = juce::Colour(0xff319795);
            break;
        case Harmonic::OrchestralSection::Synths:
            name = "SYNTHS";
            bgCol = juce::Colour(0xffd53f8c);
            break;
    }
    sectionBadge.setText(name, juce::dontSendNotification);
    sectionBadge.setColour(juce::Label::backgroundColourId, bgCol.withAlpha(0.25f));
    sectionBadge.setColour(juce::Label::textColourId, bgCol.brighter(0.4f));
}

TrackMappingConfig TrackMappingRowComponent::getConfig() const {
    TrackMappingConfig cfg;
    cfg.enabled = enableToggle.getToggleState();
    cfg.instrument = idToInstrument(instrumentSelector.getSelectedId());
    cfg.section = Harmonic::getInstrumentSection(cfg.instrument);

    int artId = articulationSelector.getSelectedId();
    switch (artId) {
        case 1: cfg.articulation = Harmonic::ArticulationType::Sustain; break;
        case 2: cfg.articulation = Harmonic::ArticulationType::Staccato; break;
        case 3: cfg.articulation = Harmonic::ArticulationType::Spiccato; break;
        case 4: cfg.articulation = Harmonic::ArticulationType::Marcato; break;
        case 5: cfg.articulation = Harmonic::ArticulationType::Tremolo; break;
        case 6: cfg.articulation = Harmonic::ArticulationType::Pizzicato; break;
        case 7: cfg.articulation = Harmonic::ArticulationType::Runs; break;
        default: cfg.articulation = Harmonic::ArticulationType::Sustain; break;
    }

    cfg.arrangerMode = arrangerModeSelector.getText().toStdString();
    return cfg;
}

void TrackMappingRowComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    bool en = enableToggle.getToggleState();
    g.setColour(en ? juce::Colour(0xff161a22) : juce::Colour(0xff101216));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(en ? juce::Colour(0xff2d3748) : juce::Colour(0xff1a202c));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void TrackMappingRowComponent::resized() {
    int w = getWidth();
    int h = getHeight();

    enableToggle.setBounds(8, (h - 20) / 2, 24, 20);
    trackNameLabel.setBounds(34, 4, 160, 20);
    noteStatsLabel.setBounds(34, 22, 160, 16);

    int rightStart = 205;
    int colW = (w - rightStart - 10) / 4;

    instrumentSelector.setBounds(rightStart, (h - 24) / 2, colW - 6, 24);
    sectionBadge.setBounds(rightStart + colW, (h - 22) / 2, colW - 6, 22);
    articulationSelector.setBounds(rightStart + colW * 2, (h - 24) / 2, colW - 6, 24);
    arrangerModeSelector.setBounds(rightStart + colW * 3, (h - 24) / 2, colW - 6, 24);
}

// -------------------------------------------------------------
// ConverterWizardComponent Implementation
// -------------------------------------------------------------
ConverterWizardComponent::ConverterWizardComponent() {
    // Navigation
    backBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3748));
    backBtn.onClick = [this]() {
        int s = static_cast<int>(currentStep);
        if (s > 0) setStep(static_cast<WizardStep>(s - 1));
    };
    addAndMakeVisible(backBtn);

    nextBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00d2ff));
    nextBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    nextBtn.onClick = [this]() {
        int s = static_cast<int>(currentStep);
        if (s < 3) setStep(static_cast<WizardStep>(s + 1));
    };
    addAndMakeVisible(nextBtn);

    // Step 1: File Loading
    step1Title.setFont(juce::Font(16.0f, juce::Font::bold));
    step1Title.setColour(juce::Label::textColourId, juce::Colour(0xff00d2ff));
    addChildComponent(step1Title);

    browseBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b6cb0));
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
    addChildComponent(browseBtn);

    fileInfoLabel.setFont(juce::Font(12.5f));
    fileInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    fileInfoLabel.setJustificationType(juce::Justification::centred);
    addChildComponent(fileInfoLabel);

    // Step 2: Tonal & Harmonic Analysis
    step2Title.setFont(juce::Font(16.0f, juce::Font::bold));
    step2Title.setColour(juce::Label::textColourId, juce::Colour(0xff00d2ff));
    addChildComponent(step2Title);

    detectedKeyBadge.setFont(juce::Font(24.0f, juce::Font::bold));
    detectedKeyBadge.setColour(juce::Label::textColourId, juce::Colour(0xff00d2ff));
    detectedKeyBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff161a22));
    detectedKeyBadge.setJustificationType(juce::Justification::centred);
    addChildComponent(detectedKeyBadge);

    confidenceLabel.setFont(juce::Font(11.5f));
    confidenceLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa0aec0));
    confidenceLabel.setJustificationType(juce::Justification::centred);
    addChildComponent(confidenceLabel);

    rootSelectorLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    rootSelectorLabel.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    addChildComponent(rootSelectorLabel);

    const char* pitchNames[12] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
    for (int i = 0; i < 12; ++i) {
        rootSelector.addItem(pitchNames[i], i + 1);
    }
    rootSelector.setSelectedId(1, juce::dontSendNotification);
    addChildComponent(rootSelector);

    modeSelectorLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    modeSelectorLabel.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    addChildComponent(modeSelectorLabel);

    modeSelector.addItem("Major (Ionian)", 1);
    modeSelector.addItem("Minor (Aeolian)", 2);
    modeSelector.addItem("Dorian", 3);
    modeSelector.addItem("Phrygian", 4);
    modeSelector.addItem("Lydian", 5);
    modeSelector.addItem("Mixolydian", 6);
    modeSelector.addItem("Harmonic Minor", 7);
    modeSelector.addItem("Melodic Minor", 8);
    modeSelector.setSelectedId(1, juce::dontSendNotification);
    addChildComponent(modeSelector);

    // Step 3: Track & Instrument Mapping
    step3Title.setFont(juce::Font(16.0f, juce::Font::bold));
    step3Title.setColour(juce::Label::textColourId, juce::Colour(0xff00d2ff));
    addChildComponent(step3Title);

    trackListViewport.setViewedComponent(&trackListContainer, false);
    trackListViewport.setScrollBarsShown(true, false);
    addChildComponent(trackListViewport);

    // Step 4: Save & Export
    step4Title.setFont(juce::Font(16.0f, juce::Font::bold));
    step4Title.setColour(juce::Label::textColourId, juce::Colour(0xff00d2ff));
    addChildComponent(step4Title);

    presetNameLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    presetNameLabel.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    addChildComponent(presetNameLabel);

    presetNameEditor.setFont(juce::Font(14.0f));
    addChildComponent(presetNameEditor);

    bpmLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    bpmLabel.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    addChildComponent(bpmLabel);

    bpmEditor.setFont(juce::Font(14.0f));
    bpmEditor.setText("120");
    addChildComponent(bpmEditor);

    stepsLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    stepsLabel.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
    addChildComponent(stepsLabel);

    stepsSelector.addItem("1 Bar (16 Steps)", 1);
    stepsSelector.addItem("2 Bars (32 Steps)", 2);
    stepsSelector.setSelectedId(1, juce::dontSendNotification);
    addChildComponent(stepsSelector);

    saveDirectBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b6cb0));
    saveDirectBtn.onClick = [this]() { savePreset(false); };
    addChildComponent(saveDirectBtn);

    saveAsBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3748));
    saveAsBtn.onClick = [this]() { savePreset(true); };
    addChildComponent(saveAsBtn);

    copyJsonBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a202c));
    copyJsonBtn.onClick = [this]() {
        ConversionOptions opts;
        opts.presetName = presetNameEditor.getText().toStdString();
        opts.overrideRootPitchClass = rootSelector.getSelectedId() - 1;
        opts.tempoBpm = bpmEditor.getText().getDoubleValue();
        opts.lengthSteps = (stepsSelector.getSelectedId() == 2) ? 32 : 16;
        for (const auto& row : trackRows) {
            opts.trackConfigs[row->getTrackIndex()] = row->getConfig();
        }
        auto pat = converter.convertToPattern(parsedMidi, tonalResult, opts);
        juce::SystemClipboard::copyTextToClipboard(pat.toJson());
        statusLabel.setText("JSON copied to clipboard!", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff48bb78));
    };
    addChildComponent(copyJsonBtn);

    statusLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff48bb78));
    statusLabel.setJustificationType(juce::Justification::centred);
    addChildComponent(statusLabel);

    setStep(WizardStep::FileLoad);
    setSize(920, 620);
}

void ConverterWizardComponent::setStep(WizardStep step) {
    currentStep = step;
    updateVisibilityForStep();
    resized();
    repaint();
}

void ConverterWizardComponent::updateVisibilityForStep() {
    bool isStep1 = (currentStep == WizardStep::FileLoad);
    bool isStep2 = (currentStep == WizardStep::TonalAnalysis);
    bool isStep3 = (currentStep == WizardStep::TrackMapping);
    bool isStep4 = (currentStep == WizardStep::SaveExport);

    // Step 1
    step1Title.setVisible(isStep1);
    browseBtn.setVisible(isStep1);
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

    // Step 4
    step4Title.setVisible(isStep4);
    presetNameLabel.setVisible(isStep4);
    presetNameEditor.setVisible(isStep4);
    bpmLabel.setVisible(isStep4);
    bpmEditor.setVisible(isStep4);
    stepsLabel.setVisible(isStep4);
    stepsSelector.setVisible(isStep4);
    saveDirectBtn.setVisible(isStep4);
    saveAsBtn.setVisible(isStep4);
    copyJsonBtn.setVisible(isStep4);
    statusLabel.setVisible(isStep4);

    // Navigation buttons
    backBtn.setVisible(!isStep1);
    nextBtn.setVisible(!isStep4);
    nextBtn.setEnabled(hasFileLoaded);
}

bool ConverterWizardComponent::isInterestedInFileDrag(const juce::StringArray& files) {
    for (const auto& f : files) {
        if (f.endsWithIgnoreCase(".mid") || f.endsWithIgnoreCase(".midi")) return true;
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
    for (const auto& path : files) {
        if (path.endsWithIgnoreCase(".mid") || path.endsWithIgnoreCase(".midi")) {
            loadMidiFile(juce::File(path));
            break;
        }
    }
    repaint();
}

void ConverterWizardComponent::loadMidiFile(const juce::File& file) {
    std::string err;
    if (!converter.parseMidiFile(file.getFullPathName().toStdString(), parsedMidi, err)) {
        fileInfoLabel.setText("Error loading file: " + juce::String(err), juce::dontSendNotification);
        fileInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xfffc8181));
        hasFileLoaded = false;
        return;
    }

    hasFileLoaded = true;

    // Harmonic & Tonal Analysis
    tonalResult = converter.analyzeTonalCenter(parsedMidi);

    // Update UI elements
    juce::String details = "Loaded: " + file.getFileName() + "\n" +
                           "Tempo: " + juce::String(static_cast<int>(parsedMidi.bpm)) + " BPM | " +
                           "Time Sig: " + juce::String(parsedMidi.timeSigNum) + "/" + juce::String(parsedMidi.timeSigDen) + " | " +
                           "Tracks: " + juce::String(static_cast<int>(parsedMidi.tracks.size()));
    fileInfoLabel.setText(details, juce::dontSendNotification);
    fileInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));

    detectedKeyBadge.setText(tonalResult.detectedChordName, juce::dontSendNotification);
    confidenceLabel.setText("Harmonic Detection Confidence: " + juce::String(static_cast<int>(tonalResult.confidence * 100.0f)) + "%", juce::dontSendNotification);

    rootSelector.setSelectedId(tonalResult.detectedRootPitchClass + 1, juce::dontSendNotification);
    if (tonalResult.detectedMode == Harmonic::ScaleMode::Ionian) modeSelector.setSelectedId(1, juce::dontSendNotification);
    else if (tonalResult.detectedMode == Harmonic::ScaleMode::Aeolian) modeSelector.setSelectedId(2, juce::dontSendNotification);
    else modeSelector.setSelectedId(1, juce::dontSendNotification);

    presetNameEditor.setText(file.getFileNameWithoutExtension());
    bpmEditor.setText(juce::String(static_cast<int>(parsedMidi.bpm)));

    buildTrackRows();
    setStep(WizardStep::TonalAnalysis);
}

void ConverterWizardComponent::buildTrackRows() {
    trackRows.clear();
    trackListContainer.removeAllChildren();

    int rowH = 46;
    int y = 0;
    for (size_t i = 0; i < parsedMidi.tracks.size(); ++i) {
        auto row = std::make_unique<TrackMappingRowComponent>(static_cast<int>(i), parsedMidi.tracks[i], []() {
            // Updated configuration callback
        });
        trackListContainer.addAndMakeVisible(*row);
        row->setBounds(0, y, 860, rowH);
        y += rowH + 4;
        trackRows.push_back(std::move(row));
    }

    trackListContainer.setBounds(0, 0, 860, y);
}

juce::File ConverterWizardComponent::getPresetsFolder() const {
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("Application Support/Automatic Orchestrator/Presets");
    if (!dir.exists()) {
        dir.createDirectory();
    }
    return dir;
}

void ConverterWizardComponent::savePreset(bool promptCustomLocation) {
    juce::String presetName = presetNameEditor.getText().trim();
    if (presetName.isEmpty()) {
        presetName = "Converted Preset";
    }

    ConversionOptions opts;
    opts.presetName = presetName.toStdString();
    opts.overrideRootPitchClass = rootSelector.getSelectedId() - 1;

    int modeId = modeSelector.getSelectedId();
    if (modeId == 1) opts.overrideMode = Harmonic::ScaleMode::Ionian;
    else if (modeId == 2) opts.overrideMode = Harmonic::ScaleMode::Aeolian;
    else if (modeId == 3) opts.overrideMode = Harmonic::ScaleMode::Dorian;
    else if (modeId == 4) opts.overrideMode = Harmonic::ScaleMode::Phrygian;
    else if (modeId == 5) opts.overrideMode = Harmonic::ScaleMode::Lydian;
    else if (modeId == 6) opts.overrideMode = Harmonic::ScaleMode::Mixolydian;
    else if (modeId == 7) opts.overrideMode = Harmonic::ScaleMode::HarmonicMinor;
    else if (modeId == 8) opts.overrideMode = Harmonic::ScaleMode::MelodicMinor;
    opts.useOverrideMode = true;

    opts.tempoBpm = bpmEditor.getText().getDoubleValue();
    opts.lengthSteps = (stepsSelector.getSelectedId() == 2) ? 32 : 16;

    for (const auto& row : trackRows) {
        opts.trackConfigs[row->getTrackIndex()] = row->getConfig();
    }

    auto pat = converter.convertToPattern(parsedMidi, tonalResult, opts);

    if (promptCustomLocation) {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Save Preset JSON", getPresetsFolder().getChildFile(presetName + ".json"), "*.json");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
            [this, pat, chooser](const juce::FileChooser& fc) {
                auto dest = fc.getResult();
                if (dest.getFullPathName().isNotEmpty()) {
                    if (pat.saveToFile(dest.getFullPathName().toStdString())) {
                        statusLabel.setText("Preset saved to: " + dest.getFileName(), juce::dontSendNotification);
                        statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff48bb78));
                    } else {
                        statusLabel.setText("Error saving file!", juce::dontSendNotification);
                        statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xfffc8181));
                    }
                }
            });
    } else {
        juce::File dest = getPresetsFolder().getChildFile(presetName + ".json");
        if (pat.saveToFile(dest.getFullPathName().toStdString())) {
            statusLabel.setText("Saved to Automatic Orchestrator Presets: " + dest.getFileName(), juce::dontSendNotification);
            statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff48bb78));
        } else {
            statusLabel.setText("Error saving preset to Application Support!", juce::dontSendNotification);
            statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xfffc8181));
        }
    }
}

void ConverterWizardComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff0b0d10));

    // Top Header Banner
    g.setColour(juce::Colour(0xff12151b));
    g.fillRect(0, 0, getWidth(), 64);
    g.setColour(juce::Colour(0xff1e232d));
    g.drawHorizontalLine(64, 0.0f, static_cast<float>(getWidth()));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("AUTOMATIC ORCHESTRATOR", 20, 14, 250, 18, juce::Justification::left);

    g.setColour(juce::Colour(0xff00d2ff));
    g.setFont(juce::Font(9.5f, juce::Font::bold));
    g.drawText("MIDI TO PRESET CONVERTER", 20, 31, 250, 15, juce::Justification::left);

    // Step indicators
    int stepStartX = getWidth() - 560;
    const char* stepLabels[] = {"1. FILE", "2. TONALITY", "3. INSTRUMENTS", "4. SAVE"};

    for (int i = 0; i < 4; ++i) {
        int x = stepStartX + i * 135;
        bool isActive = (static_cast<int>(currentStep) == i);
        bool isPast = (static_cast<int>(currentStep) > i);

        juce::Colour pillBg = isActive ? juce::Colour(0xff00d2ff) : (isPast ? juce::Colour(0xff2d3748) : juce::Colour(0xff161a22));
        juce::Colour textCol = isActive ? juce::Colours::black : (isPast ? juce::Colours::white : juce::Colour(0xff718096));

        g.setColour(pillBg);
        g.fillRoundedRectangle(static_cast<float>(x), 18.0f, 125.0f, 26.0f, 4.0f);

        g.setColour(textCol);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(stepLabels[i], x, 18, 125, 26, juce::Justification::centred);
    }

    // Step 1: Drag and Drop Area
    if (currentStep == WizardStep::FileLoad) {
        auto dropBounds = juce::Rectangle<float>(60.0f, 120.0f, static_cast<float>(getWidth() - 120), 240.0f);
        g.setColour(isDraggingOver ? juce::Colour(0xff00d2ff).withAlpha(0.2f) : juce::Colour(0xff141820));
        g.fillRoundedRectangle(dropBounds, 8.0f);

        g.setColour(isDraggingOver ? juce::Colour(0xff00d2ff) : juce::Colour(0xff2d3748));
        float dashLengths[2] = {6.0f, 6.0f};
        g.drawDashedLine(juce::Line<float>(dropBounds.getX(), dropBounds.getY(), dropBounds.getRight(), dropBounds.getY()), dashLengths, 2, 2.0f);
        g.drawDashedLine(juce::Line<float>(dropBounds.getRight(), dropBounds.getY(), dropBounds.getRight(), dropBounds.getBottom()), dashLengths, 2, 2.0f);
        g.drawDashedLine(juce::Line<float>(dropBounds.getRight(), dropBounds.getBottom(), dropBounds.getX(), dropBounds.getBottom()), dashLengths, 2, 2.0f);
        g.drawDashedLine(juce::Line<float>(dropBounds.getX(), dropBounds.getBottom(), dropBounds.getX(), dropBounds.getY()), dashLengths, 2, 2.0f);

        g.setColour(isDraggingOver ? juce::Colour(0xff00d2ff) : juce::Colour(0xffa0aec0));
        g.setFont(juce::Font(16.0f, juce::Font::bold));
        g.drawText("DRAG & DROP ORCHESTRAL MIDI FILE HERE (.MID)", 60, 160, getWidth() - 120, 24, juce::Justification::centred);

        g.setFont(juce::Font(12.0f));
        g.drawText("or click below to choose a file", 60, 190, getWidth() - 120, 20, juce::Justification::centred);
    }

    // Step 2: Pitch Class Distribution Histogram
    if (currentStep == WizardStep::TonalAnalysis && !tonalResult.pitchClassDistribution.empty()) {
        int chartX = 60;
        int chartY = 270;
        int chartW = getWidth() - 120;
        int chartH = 140;

        g.setColour(juce::Colour(0xff141820));
        g.fillRoundedRectangle(static_cast<float>(chartX), static_cast<float>(chartY), static_cast<float>(chartW), static_cast<float>(chartH), 6.0f);

        g.setColour(juce::Colour(0xff718096));
        g.setFont(juce::Font(10.5f, juce::Font::bold));
        g.drawText("PITCH CLASS DISTRIBUTION HISTOGRAM", chartX + 15, chartY + 8, 300, 16, juce::Justification::left);

        const char* pNames[12] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
        float barSlotW = static_cast<float>(chartW - 30) / 12.0f;
        float maxVal = 0.001f;
        for (float v : tonalResult.pitchClassDistribution) {
            maxVal = std::max(maxVal, v);
        }

        for (int i = 0; i < 12; ++i) {
            float bx = chartX + 15.0f + i * barSlotW;
            float val = tonalResult.pitchClassDistribution[static_cast<size_t>(i)];
            float barH = (val / maxVal) * 80.0f;
            float by = chartY + chartH - 24.0f - barH;

            bool isRoot = (i == tonalResult.detectedRootPitchClass);
            g.setColour(isRoot ? juce::Colour(0xff00d2ff) : juce::Colour(0xff4a5568));
            g.fillRect(bx + 4.0f, by, barSlotW - 8.0f, barH);

            g.setColour(isRoot ? juce::Colour(0xff00d2ff) : juce::Colour(0xffa0aec0));
            g.setFont(juce::Font(10.0f, isRoot ? juce::Font::bold : juce::Font::plain));
            g.drawText(pNames[i], static_cast<int>(bx), chartY + chartH - 20, static_cast<int>(barSlotW), 16, juce::Justification::centred);
        }
    }

    // Bottom Navigation Bar
    g.setColour(juce::Colour(0xff12151b));
    g.fillRect(0, getHeight() - 56, getWidth(), 56);
    g.setColour(juce::Colour(0xff1e232d));
    g.drawHorizontalLine(getHeight() - 56, 0.0f, static_cast<float>(getWidth()));
}

void ConverterWizardComponent::resized() {
    int w = getWidth();
    int h = getHeight();

    // Bottom Navigation Buttons
    backBtn.setBounds(20, h - 44, 90, 32);
    nextBtn.setBounds(w - 110, h - 44, 90, 32);

    // Step 1: File Loading
    step1Title.setBounds(60, 80, 400, 24);
    browseBtn.setBounds(w / 2 - 100, 290, 200, 36);
    fileInfoLabel.setBounds(60, 380, w - 120, 80);

    // Step 2: Tonal & Harmonic Analysis
    step2Title.setBounds(60, 80, 500, 24);
    detectedKeyBadge.setBounds(w / 2 - 160, 120, 320, 60);
    confidenceLabel.setBounds(w / 2 - 160, 185, 320, 20);

    int optY = 215;
    rootSelectorLabel.setBounds(w / 2 - 160, optY, 80, 24);
    rootSelector.setBounds(w / 2 - 80, optY, 65, 24);

    modeSelectorLabel.setBounds(w / 2 + 10, optY, 95, 24);
    modeSelector.setBounds(w / 2 + 110, optY, 150, 24);

    // Step 3: Track & Instrument Mapping
    step3Title.setBounds(30, 76, 500, 24);
    trackListViewport.setBounds(30, 108, w - 60, h - 175);
    trackListContainer.setSize(w - 75, trackListContainer.getHeight());
    for (auto& row : trackRows) {
        if (row != nullptr) {
            row->setSize(w - 75, row->getHeight());
        }
    }

    // Step 4: Save & Export
    step4Title.setBounds(60, 80, 500, 24);

    int formY = 125;
    presetNameLabel.setBounds(w / 2 - 180, formY, 120, 24);
    presetNameEditor.setBounds(w / 2 - 50, formY, 230, 24);

    formY += 36;
    bpmLabel.setBounds(w / 2 - 180, formY, 120, 24);
    bpmEditor.setBounds(w / 2 - 50, formY, 70, 24);

    formY += 36;
    stepsLabel.setBounds(w / 2 - 180, formY, 120, 24);
    stepsSelector.setBounds(w / 2 - 50, formY, 150, 24);

    formY += 60;
    saveDirectBtn.setBounds(w / 2 - 200, formY, 400, 40);

    formY += 48;
    saveAsBtn.setBounds(w / 2 - 200, formY, 195, 32);
    copyJsonBtn.setBounds(w / 2 + 5, formY, 195, 32);

    formY += 44;
    statusLabel.setBounds(60, formY, w - 120, 24);
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
    setResizeLimits(800, 500, 1400, 900);
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
