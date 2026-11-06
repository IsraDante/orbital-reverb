cmake_minimum_required(VERSION 3.22)

# Mac: compatible con macOS 12 o superior
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Version minima de macOS")
# Windows: el plugin no necesita instalar nada extra (runtime estatico)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(OrbitalReverb VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE se descarga automaticamente (no hace falta instalarlo)
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        9.0.5
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

set(ORBITAL_FORMATS VST3)
if(APPLE)
    list(APPEND ORBITAL_FORMATS AU)   # Logic / GarageBand en Mac
endif()

juce_add_plugin(OrbitalReverb
    COMPANY_NAME             "Dante Ramos"
    PLUGIN_MANUFACTURER_CODE Dram
    PLUGIN_CODE              Orbr
    PRODUCT_NAME             "Orbital Reverb"
    FORMATS                  ${ORBITAL_FORMATS}
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES          Fx Reverb
    AU_MAIN_TYPE             kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD  FALSE)

target_sources(OrbitalReverb PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(OrbitalReverb PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_XINPUT=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(OrbitalReverb
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# Pruebas automaticas (opcional, solo para desarrollo)
option(ORBITAL_BUILD_TESTS "Compilar las pruebas" OFF)
if(ORBITAL_BUILD_TESTS)
    juce_add_console_app(OrbitalTests PRODUCT_NAME "Orbital Tests")
    target_sources(OrbitalTests PRIVATE
        Tests/OrbitalTests.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
    target_compile_definitions(OrbitalTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_USE_XINPUT=0)
    target_link_libraries(OrbitalTests PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
