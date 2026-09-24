#include "ArticulationMap.h"

namespace Articulation {

std::ostream& operator<<(std::ostream& os, TriggerMethod m) {
    switch (m) {
        case TriggerMethod::Keyswitch: return os << "Keyswitch";
        case TriggerMethod::ContinuousController: return os << "CC";
        case TriggerMethod::ProgramChange: return os << "ProgramChange";
        case TriggerMethod::MidiChannel: return os << "MidiChannel";
        default: return os << "TriggerMethod";
    }
}

static void applyCSSTriggers(InstrumentArticulationProfile& p) {
    using namespace Harmonic;
    // Cinematic Studio Series CC58 protocol
    p.setTrigger(ArticulationType::Sustain,   {TriggerMethod::ContinuousController, 58, 0});
    p.setTrigger(ArticulationType::Spiccato,  {TriggerMethod::ContinuousController, 58, 20});
    p.setTrigger(ArticulationType::Staccato,  {TriggerMethod::ContinuousController, 58, 40});
    p.setTrigger(ArticulationType::Pizzicato, {TriggerMethod::ContinuousController, 58, 60});
    p.setTrigger(ArticulationType::Tremolo,   {TriggerMethod::ContinuousController, 58, 80});
    p.setTrigger(ArticulationType::Marcato,   {TriggerMethod::ContinuousController, 58, 100});
    p.setTrigger(ArticulationType::Runs,      {TriggerMethod::ContinuousController, 58, 20});
}

static void applySpitfireUACCTriggers(InstrumentArticulationProfile& p) {
    using namespace Harmonic;
    // Spitfire Universal Articulation Controller (CC32)
    p.setTrigger(ArticulationType::Sustain,   {TriggerMethod::ContinuousController, 32, 1});
    p.setTrigger(ArticulationType::Marcato,   {TriggerMethod::ContinuousController, 32, 9});
    p.setTrigger(ArticulationType::Tremolo,   {TriggerMethod::ContinuousController, 32, 11});
    p.setTrigger(ArticulationType::Staccato,  {TriggerMethod::ContinuousController, 32, 40});
    p.setTrigger(ArticulationType::Spiccato,  {TriggerMethod::ContinuousController, 32, 42});
    p.setTrigger(ArticulationType::Pizzicato, {TriggerMethod::ContinuousController, 32, 56});
    p.setTrigger(ArticulationType::Runs,      {TriggerMethod::ContinuousController, 32, 42});
}

static void applyEWOpusTriggers(InstrumentArticulationProfile& p) {
    using namespace Harmonic;
    // EastWest Hollywood Opus Keyswitch protocol
    p.setTrigger(ArticulationType::Sustain,   {TriggerMethod::Keyswitch, 24, 127}); // C0
    p.setTrigger(ArticulationType::Staccato,  {TriggerMethod::Keyswitch, 26, 127}); // D0
    p.setTrigger(ArticulationType::Spiccato,  {TriggerMethod::Keyswitch, 28, 127}); // E0
    p.setTrigger(ArticulationType::Marcato,   {TriggerMethod::Keyswitch, 29, 127}); // F0
    p.setTrigger(ArticulationType::Tremolo,   {TriggerMethod::Keyswitch, 31, 127}); // G0
    p.setTrigger(ArticulationType::Pizzicato, {TriggerMethod::Keyswitch, 33, 127}); // A0
    p.setTrigger(ArticulationType::Runs,      {TriggerMethod::Keyswitch, 28, 127});
}

static void applyCSBTriggers(InstrumentArticulationProfile& p) {
    using namespace Harmonic;
    // Cinematic Studio Brass CC58 protocol
    p.setTrigger(ArticulationType::Sustain,   {TriggerMethod::ContinuousController, 58, 0});
    p.setTrigger(ArticulationType::Spiccato,  {TriggerMethod::ContinuousController, 58, 20}); // Staccatissimo
    p.setTrigger(ArticulationType::Staccato,  {TriggerMethod::ContinuousController, 58, 40});
    p.setTrigger(ArticulationType::Marcato,   {TriggerMethod::ContinuousController, 58, 100});
    p.setTrigger(ArticulationType::Runs,      {TriggerMethod::ContinuousController, 58, 20});
}

static void applyCSWTriggers(InstrumentArticulationProfile& p) {
    using namespace Harmonic;
    // Cinematic Studio Woodwinds CC58 protocol
    p.setTrigger(ArticulationType::Sustain,   {TriggerMethod::ContinuousController, 58, 0});
    p.setTrigger(ArticulationType::Spiccato,  {TriggerMethod::ContinuousController, 58, 20});
    p.setTrigger(ArticulationType::Staccato,  {TriggerMethod::ContinuousController, 58, 40});
    p.setTrigger(ArticulationType::Marcato,   {TriggerMethod::ContinuousController, 58, 100});
    p.setTrigger(ArticulationType::Runs,      {TriggerMethod::ContinuousController, 58, 20});
}

LibraryProfile createCSSProfile() {
    LibraryProfile lp;
    lp.libraryName = "Cinematic Studio Strings";
    InstrumentArticulationProfile prof;
    applyCSSTriggers(prof);

    // Apply to all orchestral instruments
    for (int i = 0; i <= static_cast<int>(Harmonic::InstrumentId::OrchestralPerc); ++i) {
        lp.setInstrumentProfile(static_cast<Harmonic::InstrumentId>(i), prof);
    }
    return lp;
}

LibraryProfile createCSBProfile() {
    LibraryProfile lp;
    lp.libraryName = "Cinematic Studio Brass";
    InstrumentArticulationProfile prof;
    applyCSBTriggers(prof);

    for (int i = 0; i <= static_cast<int>(Harmonic::InstrumentId::OrchestralPerc); ++i) {
        lp.setInstrumentProfile(static_cast<Harmonic::InstrumentId>(i), prof);
    }
    return lp;
}

LibraryProfile createCSWProfile() {
    LibraryProfile lp;
    lp.libraryName = "Cinematic Studio Woodwinds";
    InstrumentArticulationProfile prof;
    applyCSWTriggers(prof);

    for (int i = 0; i <= static_cast<int>(Harmonic::InstrumentId::OrchestralPerc); ++i) {
        lp.setInstrumentProfile(static_cast<Harmonic::InstrumentId>(i), prof);
    }
    return lp;
}

LibraryProfile createSpitfireUACCProfile() {
    LibraryProfile lp;
    lp.libraryName = "Spitfire Audio (UACC)";
    InstrumentArticulationProfile prof;
    applySpitfireUACCTriggers(prof);

    for (int i = 0; i <= static_cast<int>(Harmonic::InstrumentId::OrchestralPerc); ++i) {
        lp.setInstrumentProfile(static_cast<Harmonic::InstrumentId>(i), prof);
    }
    return lp;
}

LibraryProfile createEastWestOpusProfile() {
    LibraryProfile lp;
    lp.libraryName = "EastWest Hollywood Opus";
    InstrumentArticulationProfile prof;
    applyEWOpusTriggers(prof);

    for (int i = 0; i <= static_cast<int>(Harmonic::InstrumentId::OrchestralPerc); ++i) {
        lp.setInstrumentProfile(static_cast<Harmonic::InstrumentId>(i), prof);
    }
    return lp;
}

LibraryProfile createVSLProfile() {
    LibraryProfile lp;
    lp.libraryName = "VSL Synchron";
    InstrumentArticulationProfile prof;
    using namespace Harmonic;
    // VSL default keyswitches starting at C1
    prof.setTrigger(ArticulationType::Sustain,   {TriggerMethod::Keyswitch, 24, 127});
    prof.setTrigger(ArticulationType::Staccato,  {TriggerMethod::Keyswitch, 25, 127});
    prof.setTrigger(ArticulationType::Spiccato,  {TriggerMethod::Keyswitch, 26, 127});
    prof.setTrigger(ArticulationType::Marcato,   {TriggerMethod::Keyswitch, 27, 127});
    prof.setTrigger(ArticulationType::Tremolo,   {TriggerMethod::Keyswitch, 28, 127});
    prof.setTrigger(ArticulationType::Pizzicato, {TriggerMethod::Keyswitch, 29, 127});

    for (int i = 0; i <= static_cast<int>(Harmonic::InstrumentId::OrchestralPerc); ++i) {
        lp.setInstrumentProfile(static_cast<Harmonic::InstrumentId>(i), prof);
    }
    return lp;
}

LibraryProfile createKontaktProfile() {
    LibraryProfile lp;
    lp.libraryName = "Kontakt Factory Library";
    InstrumentArticulationProfile prof;
    applyEWOpusTriggers(prof); // Default keyswitches
    for (int i = 0; i <= static_cast<int>(Harmonic::InstrumentId::OrchestralPerc); ++i) {
        lp.setInstrumentProfile(static_cast<Harmonic::InstrumentId>(i), prof);
    }
    return lp;
}

} // namespace Articulation
