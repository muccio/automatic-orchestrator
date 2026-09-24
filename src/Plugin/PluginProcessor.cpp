#include "PluginProcessor.h"
#include "PluginEditor.h"

AutomaticOrchestratorAudioProcessor::AutomaticOrchestratorAudioProcessor()
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

        // Ground truth chord recognition from played keys
        auto detected = chordDetector.detectChord(pitches);

        Harmonic::HarmonicFrame activeFrame = detected;
        if (modalSnappingEnabled.load()) {
            activeFrame = scaleQuantizer.transformToMode(detected, currentScaleMode.load());
        }

        auto rawVoicing = voicingEngine.generateVoicing(activeFrame, currentVoicingStyle.load());
        auto finalVoicing = randomizer.processVoicing(rawVoicing, activeFrame);

        {
            std::lock_guard<std::mutex> lock(stateMutex);
            lastFrame = activeFrame;
            lastVoicing = finalVoicing;
            // The display ALWAYS reflects the user's detected chord (e.g. Cmaj7)
            lastChordName = detected.chordName;
        }

        sequencerEngine.updateVoicing(finalVoicing);
    });
}

AutomaticOrchestratorAudioProcessor::~AutomaticOrchestratorAudioProcessor() {}

const juce::String AutomaticOrchestratorAudioProcessor::getName() const {
    return "Automatic Orchestrator";
}

bool AutomaticOrchestratorAudioProcessor::acceptsMidi() const { return true; }
bool AutomaticOrchestratorAudioProcessor::producesMidi() const { return true; }
bool AutomaticOrchestratorAudioProcessor::isMidiEffect() const { return false; }
double AutomaticOrchestratorAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int AutomaticOrchestratorAudioProcessor::getNumPrograms() { return 1; }
int AutomaticOrchestratorAudioProcessor::getCurrentProgram() { return 0; }
void AutomaticOrchestratorAudioProcessor::setCurrentProgram(int) {}
const juce::String AutomaticOrchestratorAudioProcessor::getProgramName(int) { return {}; }
void AutomaticOrchestratorAudioProcessor::changeProgramName(int, const juce::String&) {}

void AutomaticOrchestratorAudioProcessor::prepareToPlay(double sampleRate, int) {
    currentSampleRate = sampleRate;
    graceWindow.reset();
    voicingEngine.resetHistory();
}

void AutomaticOrchestratorAudioProcessor::releaseResources() {
    graceWindow.reset();
}

bool AutomaticOrchestratorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled();
}

void AutomaticOrchestratorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    buffer.clear(); // Pure MIDI Generator; audio buffers remain clean

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
        if (posOpt.hasValue()) {
            isPlaying = posOpt->getIsPlaying();
            if (posOpt->getBpm().hasValue()) {
                bpm = *(posOpt->getBpm());
                sequencerEngine.setTempo(bpm);
            }
            if (posOpt->getPpqPosition().hasValue()) {
                hostPpq = *(posOpt->getPpqPosition());
            }
        }
    }

    // Clear incoming buffer and populate with generated orchestral MIDI events
    midiMessages.clear();
    scheduledBuffer.clear();

    sequencerEngine.processBlock(buffer.getNumSamples(), currentSampleRate, hostPpq, isPlaying, scheduledBuffer);

    for (const auto& evt : scheduledBuffer) {
        int samplePos = std::clamp(evt.sampleOffset, 0, buffer.getNumSamples() - 1);

        if (evt.isController) {
            // CC Event (e.g. CC1 Dynamics automation)
            midiMessages.addEvent(juce::MidiMessage::controllerEvent(evt.channel, evt.pitch, (juce::uint8)evt.velocity), samplePos);
        } else if (evt.isNoteOn) {
            // Inject Articulation Switch for this specific instrument profile
            const auto& prof = activeLibrary.getInstrumentProfile(evt.instrument);
            auto trig = prof.getTrigger(evt.articulation);
            if (trig.method == Articulation::TriggerMethod::ContinuousController) {
                midiMessages.addEvent(juce::MidiMessage::controllerEvent(evt.channel, trig.param1, (juce::uint8)trig.param2), samplePos);
            } else if (trig.method == Articulation::TriggerMethod::Keyswitch) {
                midiMessages.addEvent(juce::MidiMessage::noteOn(evt.channel, trig.param1, (juce::uint8)100), samplePos);
                midiMessages.addEvent(juce::MidiMessage::noteOff(evt.channel, trig.param1, (juce::uint8)0), samplePos);
            }

            // Note On
            midiMessages.addEvent(juce::MidiMessage::noteOn(evt.channel, evt.pitch, (juce::uint8)evt.velocity), samplePos);
        } else {
            // Note Off
            midiMessages.addEvent(juce::MidiMessage::noteOff(evt.channel, evt.pitch, (juce::uint8)0), samplePos);
        }
    }
}

juce::AudioProcessorEditor* AutomaticOrchestratorAudioProcessor::createEditor() {
    return new HollywoodOrchestratorEditor(*this);
}

bool AutomaticOrchestratorAudioProcessor::hasEditor() const {
    return true;
}

void AutomaticOrchestratorAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::DynamicObject::Ptr stateObj = new juce::DynamicObject();
    stateObj->setProperty("version", 2);
    stateObj->setProperty("scaleMode", static_cast<int>(currentScaleMode.load()));
    stateObj->setProperty("voicingStyle", static_cast<int>(currentVoicingStyle.load()));
    stateObj->setProperty("modalSnapping", modalSnappingEnabled.load());
    stateObj->setProperty("tempo", sequencerEngine.getTempo());

    std::string patternJson = sequencerEngine.getPattern().toJson();
    stateObj->setProperty("patternJson", juce::String(patternJson));

    juce::var stateVar(stateObj.get());
    juce::MemoryOutputStream stream(destData, false);
    juce::JSON::writeToStream(stream, stateVar);
}

void AutomaticOrchestratorAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto stateVar = juce::JSON::parse(juce::String::createStringFromData(data, sizeInBytes));
    if (stateVar.isObject()) {
        if (stateVar.hasProperty("scaleMode")) {
            currentScaleMode.store(static_cast<Harmonic::ScaleMode>(static_cast<int>(stateVar["scaleMode"])));
        }
        if (stateVar.hasProperty("voicingStyle")) {
            currentVoicingStyle.store(static_cast<Orchestration::VoicingStyle>(static_cast<int>(stateVar["voicingStyle"])));
        }
        if (stateVar.hasProperty("modalSnapping")) {
            modalSnappingEnabled.store(static_cast<bool>(stateVar["modalSnapping"]));
        }
        if (stateVar.hasProperty("tempo")) {
            sequencerEngine.setTempo(static_cast<double>(stateVar["tempo"]));
        }
        if (stateVar.hasProperty("patternJson")) {
            std::string pJson = stateVar["patternJson"].toString().toStdString();
            if (!pJson.empty()) {
                sequencerEngine.setPattern(Sequencer::OrchestralPattern::fromJson(pJson));
            }
        }
    }
}

juce::File AutomaticOrchestratorAudioProcessor::getPresetsFolder() const {
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("Application Support/Automatic Orchestrator/Presets");
    if (!dir.exists()) {
        dir.createDirectory();
    }
    return dir;
}

bool AutomaticOrchestratorAudioProcessor::savePresetToFile(const juce::File& file, const juce::String& presetName) {
    auto pat = sequencerEngine.getPattern();
    if (presetName.isNotEmpty()) {
        pat.name = presetName.toStdString();
    }
    return pat.saveToFile(file.getFullPathName().toStdString());
}

bool AutomaticOrchestratorAudioProcessor::loadPresetFromFile(const juce::File& file) {
    if (!file.existsAsFile()) return false;
    auto pat = Sequencer::OrchestralPattern::loadFromFile(file.getFullPathName().toStdString());
    setStylePattern(pat);
    return true;
}

void AutomaticOrchestratorAudioProcessor::setScaleMode(Harmonic::ScaleMode mode) {
    currentScaleMode.store(mode);
}

void AutomaticOrchestratorAudioProcessor::setVoicingStyle(Orchestration::VoicingStyle style) {
    currentVoicingStyle.store(style);
}

void AutomaticOrchestratorAudioProcessor::setLibraryProfile(const Articulation::LibraryProfile& profile) {
    activeLibrary = profile;
}

void AutomaticOrchestratorAudioProcessor::setStylePattern(const Sequencer::OrchestralPattern& pattern) {
    sequencerEngine.setPattern(pattern);
}

const Sequencer::OrchestralPattern AutomaticOrchestratorAudioProcessor::getCurrentPattern() const {
    return sequencerEngine.getPattern();
}

std::string AutomaticOrchestratorAudioProcessor::getCurrentChordName() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return lastChordName;
}

Harmonic::HarmonicFrame AutomaticOrchestratorAudioProcessor::getCurrentHarmonicFrame() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return lastFrame;
}

Harmonic::OrchestralVoicing AutomaticOrchestratorAudioProcessor::getCurrentVoicing() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return lastVoicing;
}

std::string AutomaticOrchestratorAudioProcessor::exportMidiForDrag(int numBars,
                                                                 std::optional<Harmonic::InstrumentId> singleStem) {
    Harmonic::OrchestralVoicing voicingCopy;
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        voicingCopy = lastVoicing;
    }

    // Default Fallback: If no live MIDI chord has been played yet, generate a default C Major/Cmaj7 voicing!
    if (voicingCopy.voices.empty()) {
        Harmonic::HarmonicFrame defaultFrame;
        defaultFrame.rootPitchClass = 0; // C
        defaultFrame.bassMidiNote = 36;  // C2
        defaultFrame.quality = Harmonic::ChordQuality::Major7;
        defaultFrame.pitches = {36, 48, 52, 55, 59}; // C2, C3, E3, G3, B3
        defaultFrame.chordTones = {0, 4, 7, 11};
        defaultFrame.chordName = "Cmaj7";
        voicingCopy = voicingEngine.generateVoicing(defaultFrame, currentVoicingStyle.load());
    }

    std::string filename = singleStem.has_value()
        ? "/tmp/AutomaticOrchestrator_" + Harmonic::instrumentToString(*singleStem) + ".mid"
        : "/tmp/AutomaticOrchestrator_Master.mid";

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
    return new AutomaticOrchestratorAudioProcessor();
}
