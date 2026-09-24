#include "TestHarness.h"
#include "Articulation/ArticulationMap.h"

TEST_CASE(ArticulationMap, CinematicStudioSeriesCC58) {
    auto profile = Articulation::createCSSProfile();
    ASSERT_EQ(profile.libraryName, "Cinematic Studio Series");

    const auto& vln = profile.getInstrumentProfile(Harmonic::InstrumentId::Violins1);
    auto spiccato = vln.getTrigger(Harmonic::ArticulationType::Spiccato);

    ASSERT_EQ(spiccato.method, Articulation::TriggerMethod::ContinuousController);
    ASSERT_EQ(spiccato.param1, 58); // CC 58
    ASSERT_EQ(spiccato.param2, 20); // Value 20 = Spiccato in CSS

    auto sustain = vln.getTrigger(Harmonic::ArticulationType::Sustain);
    ASSERT_EQ(sustain.param1, 58);
    ASSERT_EQ(sustain.param2, 0); // Value 0 = Sustain in CSS
}

TEST_CASE(ArticulationMap, SpitfireUACC_CC32) {
    auto profile = Articulation::createSpitfireUACCProfile();
    ASSERT_EQ(profile.libraryName, "Spitfire Audio (UACC)");

    const auto& cello = profile.getInstrumentProfile(Harmonic::InstrumentId::Cellos);
    auto spiccato = cello.getTrigger(Harmonic::ArticulationType::Spiccato);

    ASSERT_EQ(spiccato.method, Articulation::TriggerMethod::ContinuousController);
    ASSERT_EQ(spiccato.param1, 32); // CC 32 (UACC)
    ASSERT_EQ(spiccato.param2, 42); // Value 42 = Spiccato
}

TEST_CASE(ArticulationMap, EastWestOpusKeyswitches) {
    auto profile = Articulation::createEastWestOpusProfile();
    ASSERT_EQ(profile.libraryName, "EastWest Hollywood Opus");

    const auto& horns = profile.getInstrumentProfile(Harmonic::InstrumentId::FrenchHorns);
    auto staccato = horns.getTrigger(Harmonic::ArticulationType::Staccato);

    ASSERT_EQ(staccato.method, Articulation::TriggerMethod::Keyswitch);
    ASSERT_EQ(staccato.param1, 26); // D0 keyswitch
}
