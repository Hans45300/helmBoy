cmake_minimum_required(VERSION 3.15)

# Set a default build type if none was specified
set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

project(HELMBOY
  VERSION 1.1.2
  DESCRIPTION "A cross-platform polyphonic synthesizer, modernised by Marc Scheffer."
  LANGUAGES C CXX)

if(APPLE)
  enable_language(OBJCXX)
endif()

# Force C++20 for modern features (std::span, concepts, etc.)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable aggressive SIMD optimizations
if(MSVC)
  # MSVC optimizations for x64 architecture
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:AVX2 /fp:fast")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /arch:AVX2 /fp:fast")
  # Enable vectorization reports in debug builds
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /arch:AVX2")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # Clang optimizations - Apply AVX2 only if compiling for x86_64
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
    set(AVX_FLAGS "-mavx2 -mfma")
  else()
    set(AVX_FLAGS "") # ARM64 (Apple Silicon / iOS) uses NEON automatically
  endif()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${AVX_FLAGS} -ffast-math")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${AVX_FLAGS} -ffast-math")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  # GCC optimizations - Apply AVX2 only if compiling for x86_64
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(AVX_FLAGS "-mavx2 -mfma")
  else()
    set(AVX_FLAGS "")
  endif()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${AVX_FLAGS} -ffast-math -ftree-vectorize")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${AVX_FLAGS} -ffast-math -ftree-vectorize")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# It's "bad form" to set this globally, but it's convenient.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Debug diagnostics for safe DSP migration (disabled by default).
option(HELMBOY_DEBUG_AUDIO_LEVELS "Log periodic engine output peak levels" OFF)
option(HELMBOY_DEBUG_BUFFER_INTEGRITY "Check output buffers for NaN/Inf" OFF)
option(HELMBOY_DEBUG_DSP_PREP "Log invalid DSP preparation state" OFF)
option(HELMBOY_DEBUG_BIQUAD_ROUTING "Log Biquad JUCE-path versus fallback routing" OFF)
option(HELMBOY_DEBUG_SVF_TRANSITIONS "Log SVF style/shelf transitions and reset diagnostics" OFF)
option(HELMBOY_DEBUG_REVERB "Log Reverb wet-mix sanitization events" OFF)
option(HELMBOY_ENABLE_BIQUAD_JUCE_PATH "Enable optional JUCE-compatible path in BiquadFilter" ON)
option(HELMBOY_ENABLE_SVF_JUCE_PATH "Enable optional JUCE-compatible path in StateVariableFilter" ON)
option(HELMBOY_RUN_TESTS "Enable and run HelmBoy tests" OFF)

# VST3 SDK Handling
set(JUCE_VST3_SDK "${CMAKE_CURRENT_SOURCE_DIR}/externals/VST3_SDK")

# ==================================================

add_subdirectory(concurrentqueue)
add_subdirectory(mopo)
target_include_directories(mopo PRIVATE externals/xsimd/include)
add_subdirectory(externals/xsimd)
add_subdirectory(externals/JUCE)
target_link_libraries(mopo PRIVATE juce::juce_dsp)
target_link_libraries(mopo PUBLIC juce::juce_audio_basics)
add_subdirectory(externals/VST3_SDK)

juce_add_binary_data(HelmBoyData SOURCES
  fonts/DroidSansMono.ttf
  fonts/Roboto-Regular.ttf
  fonts/Roboto-Light.ttf
  fonts/Roboto-Thin.ttf
  resources/patch_save.svg
  resources/patch_save_as.svg
  resources/patch_export.svg
  resources/patch_browse.svg
  resources/patch_edit.svg
  resources/patch_redo.svg
  images/helmBoy_icon_16_1x.png
  images/helmBoy_icon_16_2x.png
  images/helmBoy_icon_32_1x.png
  images/helmBoy_icon_32_2x.png
  images/helmBoy_icon_128_1x.png
  images/helmBoy_icon_128_2x.png
  images/helmBoy_icon_256_1x.png
  images/helmBoy_icon_256_2x.png
  images/helmBoy_icon_512_1x.png
  images/helmBoy_icon_512_2x.png)

# ==================================================

# ==============================================================================
# CONFIGURATION DES DÉPENDANCES SYSTÈME LINUX
# ==============================================================================
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LINUX_DEPS REQUIRED alsa freetype2 libcurl x11 xext xinerama xrandr xcursor)
    pkg_check_modules(JACK_DEP jack)
endif()

# Désactivation temporaire de l'option LTO pour les builds Debug et RelWithDebInfo, car elle peut causer des problèmes de compilation avec certaines versions de GCC/Clang.
# if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto=auto")
    # set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto=auto")
# endif()

option(BUILD_VST3 "Build VST3 plugin" ON)
option(BUILD_AU "Build AU plugin" ON)
option(BUILD_AUv3 "Build AUv3 plugin" ON)
option(BUILD_LV2 "Build LV2 plugin" ON)
option(BUILD_AAX "Build AAX plugin" OFF)

set(PLUGIN_FORMATS "")
if(BUILD_AAX)
  list(APPEND PLUGIN_FORMATS AAX)
endif()
if(BUILD_VST3)
  list(APPEND PLUGIN_FORMATS VST3)
endif()
if(BUILD_AU)
  list(APPEND PLUGIN_FORMATS AU)
endif()
if(BUILD_AUv3)
  list(APPEND PLUGIN_FORMATS AUv3)
endif()
if(BUILD_LV2)
  list(APPEND PLUGIN_FORMATS LV2)
endif()

juce_add_plugin(HelmBoyPlugin
  ICON_BIG images/helmBoy_icon_512_2x.png
  ICON_SMALL images/helmBoy_icon_128_2x.png

  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE

  PRODUCT_NAME "helmBoy"
  COMPANY_NAME "Marc Scheffer"
  BUNDLE_ID "com.MarcScheffer.helmBoy"
  PLUGIN_MANUFACTURER_CODE "Mars"
  FORMATS ${PLUGIN_FORMATS}

  LV2URI "https://github.com/Hans45/helmBoy"
  LV2_WANTS_LATENCY FALSE)

# The standalone GUI target defines this as 1 itself. Don't propagate the
# plugin shared-code target's conflicting value to its consumers.
get_target_property(HELMBOY_PLUGIN_INTERFACE_DEFINITIONS
  HelmBoyPlugin INTERFACE_COMPILE_DEFINITIONS)
if(HELMBOY_PLUGIN_INTERFACE_DEFINITIONS)
  list(FILTER HELMBOY_PLUGIN_INTERFACE_DEFINITIONS
    EXCLUDE REGEX "^JUCE_STANDALONE_APPLICATION=")
  set_property(TARGET HelmBoyPlugin PROPERTY INTERFACE_COMPILE_DEFINITIONS
    "${HELMBOY_PLUGIN_INTERFACE_DEFINITIONS}")
endif()

juce_generate_juce_header(HelmBoyPlugin)

target_sources(HelmBoyPlugin PRIVATE
  src/synthesis/helmBoy_engine.h
  src/plugin/helmBoy_plugin.h
  src/common/border_bounds_constrainer.cpp
  src/common/file_list_box_model.cpp
  src/common/helmBoy_common.cpp
  src/common/load_save.cpp
  src/common/midi_manager.cpp
  src/common/midi_2_manager.cpp
  src/common/midi_2_phase4.cpp
  src/common/patch_bank_archive.cpp
  src/common/startup.cpp
  src/common/synth_base.cpp
  src/common/synth_gui_interface.cpp
  src/editor_components/bpm_slider.cpp
  src/editor_components/button_modulation_slider.cpp
  src/editor_components/filter_response.cpp
  src/editor_components/filter_selector.cpp
  src/editor_components/global_tool_tip.cpp
  src/editor_components/HelmBoyGraphics.cpp
  src/editor_components/graphical_envelope.cpp
  src/editor_components/graphical_step_sequencer.cpp
  src/editor_components/midi_keyboard.cpp
  src/editor_components/modulation_button.cpp
  src/editor_components/modulation_highlight.cpp
  src/editor_components/modulation_meter.cpp
  src/editor_components/modulation_slider.cpp
  src/editor_components/oscilloscope.cpp
  src/editor_components/patch_selector.cpp
  src/editor_components/retrigger_selector.cpp
  src/editor_components/synth_button.cpp
  src/editor_components/synth_slider.cpp
  src/editor_components/tempo_selector.cpp
  src/editor_components/text_selector.cpp
  src/editor_components/text_slider.cpp
  src/editor_components/wave_selector.cpp
  src/editor_components/wave_viewer.cpp
  src/editor_components/xy_pad.cpp
  src/editor_sections/about_section.cpp
  src/editor_sections/arp_section.cpp
  src/editor_sections/bpm_section.cpp
  src/editor_sections/chorus_section.cpp
  src/editor_sections/delay_section.cpp
  src/editor_sections/delete_section.cpp
  src/editor_sections/distortion_section.cpp
  src/editor_sections/dynamic_section.cpp
  src/editor_sections/envelope_section.cpp
  src/editor_sections/extra_mod_section.cpp
  src/editor_sections/feedback_section.cpp
  src/editor_sections/filter_section.cpp
  src/editor_sections/formant_section.cpp
  src/editor_sections/full_interface.cpp
  src/editor_sections/lfo_section.cpp
  src/editor_sections/limiter_section.cpp
  src/editor_sections/mixer_section.cpp
  src/editor_sections/modulation_manager.cpp
  src/editor_sections/noise_section.cpp
  src/editor_sections/oscillator_section.cpp
  src/editor_sections/patch_browser.cpp
  src/editor_sections/reverb_section.cpp
  src/editor_sections/save_section.cpp
  src/editor_sections/step_sequencer_section.cpp
  src/editor_sections/stutter_section.cpp
  src/editor_sections/sub_section.cpp
  src/editor_sections/synthesis_interface.cpp
  src/editor_sections/synth_section.cpp
  src/editor_sections/voice_section.cpp
  src/editor_sections/volume_section.cpp
  src/look_and_feel/browser_look_and_feel.cpp
  src/look_and_feel/colors.cpp
  src/look_and_feel/default_look_and_feel.cpp
  src/look_and_feel/fonts.cpp
  src/look_and_feel/modulation_look_and_feel.cpp
  src/look_and_feel/text_look_and_feel.cpp
  src/plugin/helmBoy_editor.cpp
  src/plugin/helmBoy_plugin.cpp
  src/synthesis/dc_filter.cpp
  src/synthesis/detune_lookup.cpp
  src/synthesis/fixed_point_oscillator.cpp
  src/synthesis/fixed_point_wave.cpp
  src/synthesis/gate.cpp
  src/synthesis/helmBoy_engine.cpp
  src/synthesis/JuceExponentialSmootherWrapper.cpp
  src/synthesis/helmBoy_lfo.cpp
  src/synthesis/helmBoy_module.cpp
  src/synthesis/helmBoy_oscillators.cpp
  src/synthesis/helmBoy_voice_handler.cpp
  src/synthesis/noise_oscillator.cpp
  src/synthesis/peak_meter.cpp
  src/synthesis/resonance_cancel.cpp
  src/synthesis/switch_modulation_processor.cpp
  src/synthesis/trigger_random.cpp
  src/synthesis/value_switch.cpp)

target_include_directories(HelmBoyPlugin PUBLIC
  src/common
  src/editor_components
  src/editor_sections
  src/look_and_feel
  src/plugin
  src/synthesis)

target_include_directories(HelmBoyPlugin PRIVATE
  externals/xsimd/include)

  # Spécifie le chemin du VST3 SDK
set(VST3_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/externals/VST3_SDK")
set(JUCE_DEFINES
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_SDK_PATH="${VST3_SDK_PATH}")


# Ajustements spécifiques à Linux pour le Plugin (Audio et Affichage)
if(LINUX OR CMAKE_SYSTEM_NAME MATCHES "Linux")
  list(APPEND JUCE_DEFINES
    JUCE_USE_XRANDR=1
    JUCE_ALSA=1
    JUCE_JACK=1
  )
else()
  list(APPEND JUCE_DEFINES
    JUCE_USE_XRANDR=0
    JUCE_ALSA=0
    JUCE_JACK=0
  )
endif()

target_compile_definitions(HelmBoyPlugin PUBLIC
  # HelmBoy tries to detect at runtime whether it's actually installed,
  # or whether it's just being queried for its factory presets by
  # the LV2 .ttl generator. If it's the latter, Helm needs to know
  # where its factory presets are on the developer's disk. Not ideal
  # for a number of reasons, but it'll have to do for now.
  "helmBoy_LV2GEN_FACTORY_PRESET_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/patches/Factory Presets\""
  ${JUCE_DEFINES}
  $<$<BOOL:${HELMBOY_DEBUG_AUDIO_LEVELS}>:HELMBOY_DEBUG_AUDIO_LEVELS=1>
  $<$<BOOL:${HELMBOY_DEBUG_BUFFER_INTEGRITY}>:HELMBOY_DEBUG_BUFFER_INTEGRITY=1>
  $<$<BOOL:${HELMBOY_DEBUG_DSP_PREP}>:HELMBOY_DEBUG_DSP_PREP=1>
)

target_link_libraries(HelmBoyPlugin
  PUBLIC
    HelmBoyData
    concurrentqueue
    mopo

    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra

    juce::juce_recommended_config_flags
    # Désactivation temporaire de l'option LTO pour les builds Debug et RelWithDebInfo, car elle peut causer des problèmes de compilation avec certaines versions de GCC/Clang.
    # juce::juce_recommended_lto_flags
    # juce::juce_recommended_warning_flags
    )

if(BUILD_LV2 AND (UNIX AND NOT APPLE) AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "Forcing RTTI generation for HelmBoyEditor (LV2 + Linux + GCC + LTO)")
    target_sources(HelmBoyPlugin PRIVATE src/plugin/force_editor_rtti.cpp)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(HelmBoyPlugin PUBLIC
        ${LINUX_DEPS_LIBRARIES}
        $<$<BOOL:${JACK_DEP_FOUND}>:${JACK_DEP_LIBRARIES}>
    )
endif()

# ==================================================

juce_add_gui_app(HelmBoyStandalone
  ICON_BIG images/helmBoy_icon_512_2x.png
  ICON_SMALL images/helmBoy_icon_128_2x.png

  PRODUCT_NAME "helmBoy"
  COMPANY_NAME "Marc Scheffer"
  BUNDLE_ID "com.MarcScheffer.HelmBoyStandalone")

juce_generate_juce_header(HelmBoyStandalone)

target_sources(HelmBoyStandalone PRIVATE
  src/standalone/helmBoy_editor.cpp
  src/standalone/main.cpp)

target_include_directories(HelmBoyStandalone PRIVATE
  externals/xsimd/include
  src/common
  src/editor_components
  src/editor_sections
  src/look_and_feel
  src/standalone
  src/synthesis
  mopo/src
  concurrentqueue)




# Define a user-configurable path for the ASIO SDK
set(HELMBOY_ASIO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/externals/ASIOSDK" CACHE PATH "Path to the Steinberg ASIO SDK")

# Conditionally enable ASIO support
if(EXISTS "${HELMBOY_ASIO_SDK_PATH}/common/asiosys.h")
    message(STATUS "ASIO SDK found at ${HELMBOY_ASIO_SDK_PATH}. Enabling ASIO support.")
    target_include_directories(HelmBoyStandalone PRIVATE
        "${HELMBOY_ASIO_SDK_PATH}/common"
        "${HELMBOY_ASIO_SDK_PATH}/host")
    target_compile_definitions(HelmBoyStandalone PRIVATE
        JUCE_ASIO=1
        JUCE_ASIO_SDK="${HELMBOY_ASIO_SDK_PATH}")
else()
    message(STATUS "ASIO SDK not found. Building without ASIO support. (Set HELMBOY_ASIO_SDK_PATH to enable)")
    target_compile_definitions(HelmBoyStandalone PRIVATE JUCE_ASIO=0)
endif()

# Définitions de base communes pour le Standalone
set(STANDALONE_DEFS
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  $<$<BOOL:${HELMBOY_DEBUG_AUDIO_LEVELS}>:HELMBOY_DEBUG_AUDIO_LEVELS=1>
  $<$<BOOL:${HELMBOY_DEBUG_BUFFER_INTEGRITY}>:HELMBOY_DEBUG_BUFFER_INTEGRITY=1>
  $<$<BOOL:${HELMBOY_DEBUG_DSP_PREP}>:HELMBOY_DEBUG_DSP_PREP=1>
)

# Configuration des serveurs audio selon l'OS cible
if(MSVC OR WIN32)
  list(APPEND STANDALONE_DEFS JUCE_WASAPI=1 JUCE_DIRECTSOUND=1 JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0)
elseif(UNIX AND NOT APPLE)
  list(APPEND STANDALONE_DEFS JUCE_WASAPI=0 JUCE_DIRECTSOUND=0 JUCE_ALSA=1 JUCE_JACK=1 JUCE_USE_XRANDR=1 JUCE_JACK_CLIENT_NAME="HelmBoy")
else() # macOS
  list(APPEND STANDALONE_DEFS JUCE_WASAPI=0 JUCE_DIRECTSOUND=0 JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0)
endif()

target_compile_definitions(HelmBoyStandalone PRIVATE ${STANDALONE_DEFS})
target_link_libraries(HelmBoyStandalone PRIVATE HelmBoyPlugin)
if(UNIX AND NOT APPLE)
    target_link_libraries(HelmBoyStandalone PRIVATE
        ${LINUX_DEPS_LIBRARIES}
        $<$<BOOL:${JACK_DEP_FOUND}>:${JACK_DEP_LIBRARIES}>
        GL
    )
endif()
target_link_libraries(HelmBoyStandalone PRIVATE HelmBoyData)

option(HELMBOY_BUILD_MIDI_TESTS "Build focused MIDI 2.0 decoder tests" OFF)
if(HELMBOY_BUILD_MIDI_TESTS AND HELMBOY_RUN_TESTS)
  enable_testing()
  juce_add_console_app(HelmBoyMidiTests
    PRODUCT_NAME "HelmBoy MIDI Tests")
  juce_generate_juce_header(HelmBoyMidiTests)
  target_sources(HelmBoyMidiTests PRIVATE
    tests/midi_2_manager_tests.cpp
    src/common/midi_2_manager.cpp)
  target_include_directories(HelmBoyMidiTests PRIVATE
    src/common
    mopo/src)
  target_compile_definitions(HelmBoyMidiTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
  target_link_libraries(HelmBoyMidiTests PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_recommended_config_flags
  )
  add_test(NAME Midi2ManagerTests COMMAND HelmBoyMidiTests)
endif()

option(HELMBOY_BUILD_EFFECT_TESTS "Build focused stereo effects and noise tests" OFF)
if(HELMBOY_BUILD_EFFECT_TESTS AND HELMBOY_RUN_TESTS)
  enable_testing()
  juce_add_console_app(HelmBoyEffectTests
    PRODUCT_NAME "HelmBoy Effect Tests")
  target_sources(HelmBoyEffectTests PRIVATE
    tests/stereo_effects_tests.cpp
    src/synthesis/noise_oscillator.cpp)
  target_include_directories(HelmBoyEffectTests PRIVATE
    externals/xsimd/include
    src/synthesis)
  target_compile_options(HelmBoyEffectTests PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/fp:precise>
    $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-finite-math-only>)
  target_compile_definitions(HelmBoyEffectTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
  target_link_libraries(HelmBoyEffectTests PRIVATE
    mopo
    juce::juce_dsp
    juce::juce_recommended_config_flags)
  add_test(NAME StereoEffectsTests COMMAND HelmBoyEffectTests)
endif()

option(HELMBOY_BUILD_SVF_TESTS "Build focused state-variable filter tests" OFF)
if(HELMBOY_BUILD_SVF_TESTS AND HELMBOY_RUN_TESTS)
  enable_testing()
  juce_add_console_app(HelmBoySVFTests
    PRODUCT_NAME "HelmBoy SVF Tests")
  target_sources(HelmBoySVFTests PRIVATE
    tests/state_variable_filter_tests.cpp)
  target_include_directories(HelmBoySVFTests PRIVATE
    externals/xsimd/include
    mopo/src)
  target_compile_definitions(HelmBoySVFTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
  target_link_libraries(HelmBoySVFTests PRIVATE
    mopo
    juce::juce_recommended_config_flags)
  add_test(NAME StateVariableFilterTests COMMAND HelmBoySVFTests)
endif()

option(HELMBOY_BUILD_PARAMETER_TEXT_TESTS "Build focused host parameter text tests" OFF)
if(HELMBOY_BUILD_PARAMETER_TEXT_TESTS AND HELMBOY_RUN_TESTS)
  enable_testing()
  juce_add_console_app(HelmBoyParameterTextTests
    PRODUCT_NAME "HelmBoy Parameter Text Tests")
  juce_generate_juce_header(HelmBoyParameterTextTests)
  target_sources(HelmBoyParameterTextTests PRIVATE
    tests/parameter_text_tests.cpp
    src/common/helmBoy_common.cpp)
  target_include_directories(HelmBoyParameterTextTests PRIVATE
    externals/xsimd/include
    src/common
    src/plugin)
  target_compile_definitions(HelmBoyParameterTextTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
  target_link_libraries(HelmBoyParameterTextTests PRIVATE
    mopo
    juce::juce_audio_processors
    juce::juce_recommended_config_flags)
  add_test(NAME ParameterTextTests COMMAND HelmBoyParameterTextTests)
endif()

option(HELMBOY_BUILD_PARAMETER_NOTIFICATION_TESTS "Build focused automation notification tests" OFF)
if(HELMBOY_BUILD_PARAMETER_NOTIFICATION_TESTS AND HELMBOY_RUN_TESTS)
  enable_testing()
  find_package(Threads REQUIRED)
  juce_add_console_app(HelmBoyParameterNotificationTests
    PRODUCT_NAME "HelmBoy Parameter Notification Tests")
  juce_generate_juce_header(HelmBoyParameterNotificationTests)
  target_sources(HelmBoyParameterNotificationTests PRIVATE
    tests/parameter_notification_tests.cpp
    src/common/helmBoy_common.cpp)
  target_include_directories(HelmBoyParameterNotificationTests PRIVATE
    externals/xsimd/include
    src/common
    src/plugin)
  target_compile_definitions(HelmBoyParameterNotificationTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
  target_link_libraries(HelmBoyParameterNotificationTests PRIVATE
    mopo
    concurrentqueue
    Threads::Threads
    juce::juce_audio_processors
    juce::juce_recommended_config_flags)
  add_test(NAME ParameterNotificationTests COMMAND HelmBoyParameterNotificationTests)
endif()

option(HELMBOY_BUILD_TAIL_TESTS "Build focused host tail reporting tests" OFF)
if(HELMBOY_BUILD_TAIL_TESTS AND HELMBOY_RUN_TESTS)
  enable_testing()
  juce_add_console_app(HelmBoyTailTests PRODUCT_NAME "HelmBoy Tail Tests")
  target_sources(HelmBoyTailTests PRIVATE tests/tail_length_tests.cpp)
  target_include_directories(HelmBoyTailTests PRIVATE
    externals/xsimd/include
    src/common)
  target_link_libraries(HelmBoyTailTests PRIVATE
    mopo
    juce::juce_recommended_config_flags)
  add_test(NAME TailLengthTests COMMAND HelmBoyTailTests)
endif()

option(HELMBOY_BUILD_HOST_RESET_TESTS "Build focused plugin host reset tests" OFF)
if(HELMBOY_BUILD_HOST_RESET_TESTS AND HELMBOY_RUN_TESTS)
  enable_testing()
  juce_add_console_app(HelmBoyHostResetTests
    PRODUCT_NAME "HelmBoy Host Reset Tests")
  juce_generate_juce_header(HelmBoyHostResetTests)
  target_sources(HelmBoyHostResetTests PRIVATE tests/host_reset_tests.cpp)
  target_include_directories(HelmBoyHostResetTests PRIVATE externals/xsimd/include)
  target_link_libraries(HelmBoyHostResetTests PRIVATE
    HelmBoyPlugin
    juce::juce_recommended_config_flags)
  add_test(NAME HostResetTests COMMAND HelmBoyHostResetTests)
endif()

option(HELMBOY_BUILD_UNDO_HISTORY_TESTS "Build focused Undo/Redo history tests" OFF)
if(HELMBOY_BUILD_UNDO_HISTORY_TESTS AND HELMBOY_RUN_TESTS)
  enable_testing()
  juce_add_console_app(HelmBoyUndoHistoryTests
    PRODUCT_NAME "HelmBoy Undo History Tests")
  target_sources(HelmBoyUndoHistoryTests PRIVATE
    tests/undo_history_tests.cpp)
  target_include_directories(HelmBoyUndoHistoryTests PRIVATE
    src/common
    mopo/src)
  target_compile_definitions(HelmBoyUndoHistoryTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
  target_link_libraries(HelmBoyUndoHistoryTests PRIVATE
    juce::juce_recommended_config_flags)
  add_test(NAME UndoHistoryTests COMMAND HelmBoyUndoHistoryTests)
endif()

option(HELMBOY_BUILD_MODULATION_TESTS "Build focused bipolar modulation tests" OFF)
if(HELMBOY_BUILD_MODULATION_TESTS AND HELMBOY_RUN_TESTS)
  enable_testing()
  juce_add_console_app(HelmBoyModulationTests
    PRODUCT_NAME "HelmBoy Modulation Tests")
  target_sources(HelmBoyModulationTests PRIVATE
    tests/modulation_tests.cpp
    src/common/helmBoy_common.cpp
    src/synthesis/switch_modulation_processor.cpp)
  target_include_directories(HelmBoyModulationTests PRIVATE
    src/common
    src/synthesis
    mopo/src)
  target_compile_definitions(HelmBoyModulationTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
  target_link_libraries(HelmBoyModulationTests PRIVATE
    mopo
    juce::juce_recommended_config_flags)
  add_test(NAME ModulationTests COMMAND HelmBoyModulationTests)
endif()

option(HELMBOY_BUILD_LFO_TEMPO_TESTS "Build focused LFO tempo mapping tests" OFF)
if(HELMBOY_BUILD_LFO_TEMPO_TESTS AND HELMBOY_RUN_TESTS)
  enable_testing()
  juce_add_console_app(HelmBoyLfoTempoTests
    PRODUCT_NAME "HelmBoy LFO Tempo Tests")
  target_sources(HelmBoyLfoTempoTests PRIVATE
    tests/lfo_tempo_tests.cpp
    src/common/helmBoy_common.cpp)
  target_include_directories(HelmBoyLfoTempoTests PRIVATE
    src/common
    mopo/src)
  target_compile_definitions(HelmBoyLfoTempoTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
  target_link_libraries(HelmBoyLfoTempoTests PRIVATE
    mopo
    juce::juce_recommended_config_flags)
  add_test(NAME LfoTempoTests COMMAND HelmBoyLfoTempoTests)
endif()

option(HELMBOY_BUILD_LFO_WAVEFORM_TESTS "Build focused LFO phase and waveform tests" OFF)
if(HELMBOY_BUILD_LFO_WAVEFORM_TESTS AND HELMBOY_RUN_TESTS)
  enable_testing()
  juce_add_console_app(HelmBoyLfoWaveformTests
    PRODUCT_NAME "HelmBoy LFO Waveform Tests")
  target_sources(HelmBoyLfoWaveformTests PRIVATE
    tests/lfo_waveform_tests.cpp
    src/common/helmBoy_common.cpp
    src/synthesis/helmBoy_lfo.cpp)
  target_include_directories(HelmBoyLfoWaveformTests PRIVATE
    src/common
    src/synthesis
    mopo/src)
  target_compile_definitions(HelmBoyLfoWaveformTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
  target_link_libraries(HelmBoyLfoWaveformTests PRIVATE
    mopo
    juce::juce_recommended_config_flags)
  add_test(NAME LfoWaveformTests COMMAND HelmBoyLfoWaveformTests)
endif()

option(HELMBOY_BUILD_WAVEFORM_SELECTION_TESTS "Build focused waveform selection UI math tests" OFF)
if(HELMBOY_BUILD_WAVEFORM_SELECTION_TESTS AND HELMBOY_RUN_TESTS)
  enable_testing()
  juce_add_console_app(HelmBoyWaveformSelectionTests
    PRODUCT_NAME "HelmBoy Waveform Selection Tests")
  target_sources(HelmBoyWaveformSelectionTests PRIVATE
    tests/waveform_selection_tests.cpp
    src/common/helmBoy_common.cpp
    src/synthesis/fixed_point_wave.cpp)
  target_include_directories(HelmBoyWaveformSelectionTests PRIVATE
    src/editor_components
    src/common
    src/synthesis
    externals/xsimd/include
    mopo/src)
  target_compile_definitions(HelmBoyWaveformSelectionTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
  target_link_libraries(HelmBoyWaveformSelectionTests PRIVATE
    mopo
    juce::juce_recommended_config_flags)
  add_test(NAME WaveformSelectionTests COMMAND HelmBoyWaveformSelectionTests)
endif()

option(HELMBOY_BUILD_PATCH_BANK_TESTS "Build focused patch bank import/export tests" OFF)
if(HELMBOY_BUILD_PATCH_BANK_TESTS AND HELMBOY_RUN_TESTS)
  enable_testing()
  juce_add_console_app(HelmBoyPatchBankTests
    PRODUCT_NAME "HelmBoy Patch Bank Tests")
  juce_generate_juce_header(HelmBoyPatchBankTests)
  target_sources(HelmBoyPatchBankTests PRIVATE
    tests/patch_bank_archive_tests.cpp
    src/common/helmBoy_common.cpp
    src/common/patch_bank_archive.cpp)
  target_include_directories(HelmBoyPatchBankTests PRIVATE
    src/common
    mopo/src)
  target_compile_definitions(HelmBoyPatchBankTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
  target_link_libraries(HelmBoyPatchBankTests PRIVATE
    mopo
    juce::juce_core
    juce::juce_recommended_config_flags)
  add_test(NAME PatchBankArchiveTests COMMAND HelmBoyPatchBankTests)
endif()

# ==============================================================================
# CONFIGURATION DE LA SIGNATURE DE CODE APPLE (macOS & iOS)
# Pour un test et une compiation locale, vous pouvez utiliser un compte développeur gratuit, mais pour la distribution sur l'App Store, un compte payant est nécessaire.
# Syntaxe à utiliser pour compiler en vue d'une utilisation locale :
# cmake -B build -G Xcode -DAPPLE_CODE_SIGN_IDENTITY="Apple Development" -DAPPLE_DEVELOPMENT_TEAM="Sign to Run Locally"
# Une fois le projet compilé dans Xcode, il est nécessaire d'exécuter la version Standalone au moins une fois
# pour enregistrer l'AUv3 dans le système (PluginKit) afin que vos DAW (Logic, GarageBand) puissent le détecter.
# ==============================================================================
if(APPLE)
  # 1. Définissez vos identifiants Apple Developer (Remplacez par vos informations)
  set(APPLE_DEVELOPMENT_TEAM "VOTRE_TEAM_ID_ICI" CACHE STRING "Apple Development Team ID")
  set(APPLE_CODE_SIGN_IDENTITY "Apple Development" CACHE STRING "Code sign identity")

  # 2. Liste des cibles créées par JUCE pour HelmBoyPlugin à signer obligatoirement
  # JUCE génère automatiquement des sous-cibles basées sur le nom principal
  set(HELMBOY_XCODE_TARGETS
    HelmBoyPlugin           # Cible globale
    HelmBoyStandalone # L'application parente (nécessaire pour l'AUv3)
    HelmBoyPlugin_AU        # Le format AUv2 traditionnel
    HelmBoyPlugin_AUv3      # L'extension AUv3 (.appex)
  )

  # 3. Application des attributs de signature à toutes les cibles Xcode
  foreach(target IN LISTS HELMBOY_XCODE_TARGETS)
    if(TARGET ${target})
      set_target_properties(${target} PROPERTIES
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${APPLE_DEVELOPMENT_TEAM}"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${APPLE_CODE_SIGN_IDENTITY}"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
      )
    endif()
  endforeach()

  # 4. Activation du Sandbox et du Hardened Runtime (Essentiel pour la validation AUv3)
  if(TARGET HelmBoyPlugin_AUv3)
    set_target_properties(HelmBoyPlugin_AUv3 PROPERTIES
      XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
      # Obligatoire pour l'App Store. A passer à NO pour le développement local et l'accès aux fichiers.
      XCODE_ATTRIBUTE_com.apple.security.app-sandbox "YES"
    )
  endif()
endif()

if(MSVC)
  get_property(all_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)

  foreach(target IN LISTS all_targets)
    get_target_property(target_type ${target} TYPE)

    if(target_type STREQUAL "EXECUTABLE"
       OR target_type STREQUAL "SHARED_LIBRARY"
       OR target_type STREQUAL "MODULE_LIBRARY")
      target_link_options(${target} PRIVATE
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:RelWithDebInfo>:/LTCG>
        $<$<CONFIG:MinSizeRel>:/LTCG>)
    endif()
  endforeach()
endif()