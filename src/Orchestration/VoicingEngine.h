#pragma once

#include "Common/HarmonicTypes.h"
#include "OrchestralTessituras.h"
#include <map>

namespace Orchestration {

enum class VoicingStyle {
    AcousticPyramid,
    Drop2,
    Drop4,
    CloseVoicing,
    OpenVoicing
};

class VoicingEngine {
public:
    VoicingEngine() = default;

    Tessitura getTessitura(Harmonic::InstrumentId id) const {
        return getInstrumentTessitura(id);
    }

    Harmonic::OrchestralVoicing generateVoicing(const Harmonic::HarmonicFrame& frame,
                                                VoicingStyle style = VoicingStyle::AcousticPyramid);

    void resetHistory();

private:
    std::map<Harmonic::InstrumentId, int> previousPitches;

    int findBestPitchForInstrument(Harmonic::InstrumentId id,
                                  const std::vector<int>& allowedPitchClasses,
                                  int preferredPitchClass,
                                  int targetRegisterMin,
                                  int targetRegisterMax);
};

} // namespace Orchestration
