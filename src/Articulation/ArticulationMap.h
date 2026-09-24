#pragma once

#include "Common/HarmonicTypes.h"
#include <string>
#include <map>
#include <iostream>

namespace Articulation {

enum class TriggerMethod {
    Keyswitch,             // Param1 = Note number, Param2 = Velocity
    ContinuousController,  // Param1 = CC Number, Param2 = Value
    ProgramChange,         // Param1 = Program Number, Param2 = 0
    MidiChannel            // Param1 = Channel (1-16)
};

std::ostream& operator<<(std::ostream& os, TriggerMethod m);

struct ArticulationTrigger {
    TriggerMethod method = TriggerMethod::Keyswitch;
    int param1 = 24; // Note pitch or CC number or Program
    int param2 = 127; // Velocity or CC value
};

class InstrumentArticulationProfile {
public:
    void setTrigger(Harmonic::ArticulationType art, const ArticulationTrigger& trigger) {
        triggers[art] = trigger;
    }

    ArticulationTrigger getTrigger(Harmonic::ArticulationType art) const {
        auto it = triggers.find(art);
        if (it != triggers.end()) {
            return it->second;
        }
        // Fallback default
        return {TriggerMethod::Keyswitch, 24, 127};
    }

    const std::map<Harmonic::ArticulationType, ArticulationTrigger>& getAllTriggers() const {
        return triggers;
    }

private:
    std::map<Harmonic::ArticulationType, ArticulationTrigger> triggers;
};

struct LibraryProfile {
    std::string libraryName;
    std::map<Harmonic::InstrumentId, InstrumentArticulationProfile> instruments;

    const InstrumentArticulationProfile& getInstrumentProfile(Harmonic::InstrumentId id) const {
        static InstrumentArticulationProfile empty;
        auto it = instruments.find(id);
        if (it != instruments.end()) {
            return it->second;
        }
        return empty;
    }

    void setInstrumentProfile(Harmonic::InstrumentId id, const InstrumentArticulationProfile& prof) {
        instruments[id] = prof;
    }
};

// Factory functions for built-in orchestral libraries
LibraryProfile createCSSProfile();
LibraryProfile createSpitfireUACCProfile();
LibraryProfile createEastWestOpusProfile();
LibraryProfile createVSLProfile();
LibraryProfile createKontaktProfile();

} // namespace Articulation
