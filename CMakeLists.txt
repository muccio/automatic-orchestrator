cmake_minimum_required(VERSION 3.20)
project(AutomaticOrchestrator VERSION 2.0.0 LANGUAGES CXX C)

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
    src/Converter/MidiPresetConverter.cpp
)

target_include_directories(orchestrator_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Harmonic
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Orchestration
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Sequencer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Articulation
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MidiExport
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Converter
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
    tests/TestMidiPresetConverter.cpp
)

target_include_directories(orchestrator_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(orchestrator_tests PRIVATE orchestrator_core)

# CLI Converter Executable
add_executable(midi_preset_cli
    src/Converter/ConverterCliMain.cpp
)
target_include_directories(midi_preset_cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(midi_preset_cli PRIVATE orchestrator_core)

# Optional JUCE Plugin Target
set(JUCE_DIR "/Users/mariosalvucci/Documents/Development/JUCE" CACHE PATH "Path to local JUCE installation")
if(EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(STATUS "Found local JUCE at: ${JUCE_DIR}")
    add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)

    juce_add_plugin(AutomaticOrchestrator
        PLUGIN_NAME "Automatic Orchestrator"
        COMPANY_NAME "OrchestratorLab"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        FORMATS VST3 AU Standalone
        PRODUCT_NAME "Automatic Orchestrator"
    )

    target_sources(AutomaticOrchestrator PRIVATE
        src/Plugin/PluginProcessor.cpp
        src/Plugin/PluginEditor.cpp
    )

    target_include_directories(AutomaticOrchestrator PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_compile_definitions(AutomaticOrchestrator PRIVATE
        JUCE_VST3_CAN_REPLACE_VST2=0
    )

    target_link_libraries(AutomaticOrchestrator PRIVATE
        orchestrator_core
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
    )

    juce_generate_juce_header(AutomaticOrchestrator)

    # Standalone GUI Wizard App: Automatic Orchestrator - MIDI Converter
    juce_add_gui_app(AutomaticOrchestratorConverter
        PRODUCT_NAME "Automatic Orchestrator - MIDI Converter"
        COMPANY_NAME "OrchestratorLab"
        VERSION 1.0.0
    )

    target_sources(AutomaticOrchestratorConverter PRIVATE
        src/Converter/ConverterApp.cpp
    )

    target_include_directories(AutomaticOrchestratorConverter PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(AutomaticOrchestratorConverter PRIVATE
        orchestrator_core
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
    )

    juce_generate_juce_header(AutomaticOrchestratorConverter)
else()
    message(WARNING "JUCE not found at ${JUCE_DIR}. Plugin targets will be skipped, building core and test suite.")
endif()

