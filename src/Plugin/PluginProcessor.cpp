#include "PluginProcessor.h"
#include "PluginEditor.h"

HollywoodOrchestratorAudioProcessor::HollywoodOrchestratorAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    activeLibrary = Articulation::createCSSProfile(); // Default to Cinematic Studio Series
    sequencerEngine.setPattern(Sequencer::createActionOstinatoPattern());

    graceWindow.setGracePeriodMs(25.0);
    graceWindow.setOnChordReadyCallback([this](const std::vector<Harmonic::MidiNoteEvent>& notes) {
        if (notes.empty()) {
            std::lock_guard<std::mutex> lock(stateMutex);
            lastChordName = "Ready";
            lastFrame = Harmonic::HarmonicFrame();
            lastVoicing = Harmonic::OrchestralVoicing();
            sequencerEngine.updateVoicing(lastVoicing);
            return;
        }

        std::vector<int> pitches;
        for (const auto& n : notes) pitches.push_back(n.pitch);

        auto detected = chordDetector.detectChord(pitches);
        auto modal = scaleQuantizer.transformToMode(detected, currentScaleMode.load());
        auto rawVoicing = voicingEngine.generateVoicing(modal, currentVoicingStyle.load());
        auto finalVoicing = randomizer.processVoicing(rawVoicing, modal);

        {
            std::lock_guard<std::mutex> lock(stateMutex);
            lastFrame = modal;
            lastVoicing = finalVoicing;
            lastChordName = modal.chordName;
        }

        sequencerEngine.updateVoicing(finalVoicing);
    });
}

HollywoodOrchestratorAudioProcessor::~HollywoodOrchestratorAudioProcessor() {}

const juce::String HollywoodOrchestratorAudioProcessor::getName() const {
    return "Hollywood Orchestrator";
}

bool HollywoodOrchestratorAudioProcessor::acceptsMidi() const { return true; }
bool HollywoodOrchestratorAudioProcessor::producesMidi() const { return true; }
bool HollywoodOrchestratorAudioProcessor::isMidiEffect() const { return false; }
double HollywoodOrchestratorAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int HollywoodOrchestratorAudioProcessor::getNumPrograms() { return 1; }
int HollywoodOrchestratorAudioProcessor::getCurrentProgram() { return 0; }
void HollywoodOrchestratorAudioProcessor::setCurrentProgram(int) {}
const juce::String HollywoodOrchestratorAudioProcessor::getProgramName(int) { return {}; }
void HollywoodOrchestratorAudioProcessor::changeProgramName(int, const juce::String&) {}

void HollywoodOrchestratorAudioProcessor::prepareToPlay(double sampleRate, int) {
    currentSampleRate = sampleRate;
    graceWindow.reset();
    voicingEngine.resetHistory();
}

void HollywoodOrchestratorAudioProcessor::releaseResources() {
    graceWindow.reset();
}

bool HollywoodOrchestratorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled();
}

void HollywoodOrchestratorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    buffer.clear(); // We are a MIDI generating instrument; audio remains clean/silent

    // Ingest incoming MIDI notes and CCs into GraceWindow
    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        double timestampSec = static_cast<double>(metadata.samplePosition) / currentSampleRate;

        if (msg.isNoteOn()) {
            graceWindow.handleNoteOn(msg.getNoteNumber(), msg.getVelocity(), timestampSec);
        } else if (msg.isNoteOff()) {
            graceWindow.handleNoteOff(msg.getNoteNumber(), timestampSec);
        } else if (msg.isController()) {
            graceWindow.handleController(msg.getControllerNumber(), msg.getControllerValue(), timestampSec);
        }
    }

    // Advance grace window clock
    double deltaSec = static_cast<double>(buffer.getNumSamples()) / currentSampleRate;
    graceWindow.advanceTime(deltaSec);

    // Host Transport Sync
    double hostPpq = 0.0;
    double bpm = 120.0;
    bool isPlaying = false;

    if (auto* playHead = getPlayHead()) {
        auto posOpt = playHead->getPosition();
        if (posOpt.has_value()) {
            if (posOpt->getBpm().has_value()) bpm = *posOpt->getBpm();
            if (posOpt->getPpqPosition().has_value()) hostPpq = *posOpt->getPpqPosition();
            isPlaying = posOpt->getIsPlaying();
        }
    }
    sequencerEngine.setTempo(bpm);

    // Render scheduled MIDI events for current audio block
    scheduledBuffer.clear();
    sequencerEngine.processBlock(buffer.getNumSamples(), currentSampleRate, hostPpq, isPlaying, scheduledBuffer);

    // Clear incoming buffer and populate with our 16-channel orchestrated output
    midiMessages.clear();

    for (const auto& ev : scheduledBuffer) {
        int ch = std::clamp(ev.channel, 1, 16);
        int sampleOffset = std::clamp(ev.sampleOffset, 0, buffer.getNumSamples() - 1);

        if (ev.isNoteOn) {
            // Check articulation trigger for keyswitch or CC
            // For example: if instrument profile defines a CC, emit it just before NoteOn
            midiMessages.addEvent(juce::MidiMessage::noteOn(ch, ev.pitch, static_cast<juce::uint8>(ev.velocity)), sampleOffset);
        } else {
            midiMessages.addEvent(juce::MidiMessage::noteOff(ch, ev.pitch), sampleOffset);
        }
    }
}

bool HollywoodOrchestratorAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* HollywoodOrchestratorAudioProcessor::createEditor() {
    return new HollywoodOrchestratorEditor(*this);
}

void HollywoodOrchestratorAudioProcessor::getStateInformation(juce::MemoryBlock&) {}
void HollywoodOrchestratorAudioProcessor::setStateInformation(const void*, int) {}

void HollywoodOrchestratorAudioProcessor::setScaleMode(Harmonic::ScaleMode mode) {
    currentScaleMode.store(mode);
}

void HollywoodOrchestratorAudioProcessor::setVoicingStyle(Orchestration::VoicingStyle style) {
    currentVoicingStyle.store(style);
}

void HollywoodOrchestratorAudioProcessor::setLibraryProfile(const Articulation::LibraryProfile& profile) {
    activeLibrary = profile;
}

void HollywoodOrchestratorAudioProcessor::setStylePattern(const Sequencer::OrchestralPattern& pattern) {
    sequencerEngine.setPattern(pattern);
}

const Sequencer::OrchestralPattern& HollywoodOrchestratorAudioProcessor::getCurrentPattern() const {
    return sequencerEngine.getPattern();
}

std::string HollywoodOrchestratorAudioProcessor::getCurrentChordName() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return lastChordName;
}

Harmonic::HarmonicFrame HollywoodOrchestratorAudioProcessor::getCurrentHarmonicFrame() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return lastFrame;
}

Harmonic::OrchestralVoicing HollywoodOrchestratorAudioProcessor::getCurrentVoicing() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return lastVoicing;
}

std::string HollywoodOrchestratorAudioProcessor::exportMidiForDrag(int numBars,
                                                                 std::optional<Harmonic::InstrumentId> singleStem) {
    Harmonic::OrchestralVoicing voicingCopy;
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        voicingCopy = lastVoicing;
    }

    std::string filename = singleStem.has_value()
        ? "/tmp/HollywoodOrch_" + Harmonic::instrumentToString(*singleStem) + ".mid"
        : "/tmp/HollywoodOrch_Master.mid";

    // Clean whitespace from filename
    std::replace(filename.begin(), filename.end(), ' ', '_');

    midiWriter.exportMidiFile(sequencerEngine.getPattern(),
                             voicingCopy,
                             sequencerEngine.getTempo(),
                             numBars,
                             filename,
                             singleStem);

    return filename;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new HollywoodOrchestratorAudioProcessor();
}
