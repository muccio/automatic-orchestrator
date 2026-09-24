cmake_minimum_required(VERSION 3.20)
project(HollywoodOrchestrator VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Architecture support (Apple Silicon & x86_64)
if(APPLE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    endif()
endif()

# Core Orchestrator Library (pure C++20, real-time safe)
add_library(orchestrator_core STATIC
    src/Common/HarmonicTypes.cpp
    src/Harmonic/GraceWindowBuffer.cpp
    src/Harmonic/ChordDetector.cpp
    src/Harmonic/ScaleQuantizer.cpp
    src/Orchestration/VoicingEngine.cpp
    src/Orchestration/HarmonicRandomizer.cpp
    src/Sequencer/PatternModel.cpp
    src/Sequencer/SequencerEngine.cpp
    src/Articulation/ArticulationMap.cpp
    src/MidiExport/StandardMidiWriter.cpp
)

target_include_directories(orchestrator_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Harmonic
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Orchestration
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Sequencer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Articulation
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MidiExport
)

# Test Suite Executable
add_executable(orchestrator_tests
    tests/TestRunner.cpp
    tests/TestHarmonicTypes.cpp
    tests/TestGraceWindow.cpp
    tests/TestChordDetector.cpp
    tests/TestScaleQuantizer.cpp
    tests/TestVoicingEngine.cpp
    tests/TestRandomizer.cpp
    tests/TestSequencerEngine.cpp
    tests/TestArticulationMap.cpp
    tests/TestMidiWriter.cpp
    tests/TestPluginProcessor.cpp
    tests/TestIntegrationE2E.cpp
)

target_include_directories(orchestrator_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(orchestrator_tests PRIVATE orchestrator_core)

# Optional JUCE Plugin Target
set(JUCE_DIR "/Users/mariosalvucci/Documents/Development/JUCE" CACHE PATH "Path to local JUCE installation")
if(EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(STATUS "Found local JUCE at: ${JUCE_DIR}")
    add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)

    juce_add_plugin(HollywoodOrchestrator
        PLUGIN_NAME "Hollywood Orchestrator"
        COMPANY_NAME "OrchestratorLab"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        FORMATS VST3 AU Standalone
        PRODUCT_NAME "Hollywood Orchestrator"
    )

    target_sources(HollywoodOrchestrator PRIVATE
        src/Plugin/PluginProcessor.cpp
        src/Plugin/PluginEditor.cpp
    )

    target_include_directories(HollywoodOrchestrator PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(HollywoodOrchestrator PRIVATE
        orchestrator_core
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
    )

    juce_generate_juce_header(HollywoodOrchestrator)
else()
    message(WARNING "JUCE not found at ${JUCE_DIR}. Plugin targets will be skipped, building core and test suite.")
endif()
