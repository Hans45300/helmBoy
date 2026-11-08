cmake_minimum_required(VERSION 3.15)


project(MOPO
  VERSION 0.10.0
  DESCRIPTION "A modular and polyphonic audio synthesis library, by Matt Tytel."
  LANGUAGES CXX)

# Laisser CMake choisir automatiquement le toolset compatible avec le g�n�rateur.
# Forcer v143 casse la configuration sur Visual Studio 2026 (toolset v180).

# Activer C++20 par d�faut
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(mopo STATIC
  src/alias.cpp
  src/arpeggiator.cpp
  src/biquad_filter.cpp
  src/bit_crush.cpp
  src/bypass_router.cpp
  src/chorus.cpp
  src/delay.cpp
  src/distortion.cpp
  src/envelope.cpp
  src/feedback.cpp
  src/formant_manager.cpp
  src/ladder_filter.cpp
  src/linear_slope.cpp
  src/limiter.cpp
  src/magnitude_lookup.cpp
  src/memory.cpp
  src/midi_lookup.cpp
  src/mono_panner.cpp
  src/operators.cpp
  src/oscillator.cpp
  src/portamento_slope.cpp
  src/processor.cpp
  src/processor_router.cpp
  src/resonance_lookup.cpp
  src/reverb.cpp
  src/sample_decay_lookup.cpp
  src/simple_delay.cpp
  src/smooth_filter.cpp
  src/smooth_value.cpp
  src/stereo_balance.cpp
  src/state_variable_filter.cpp
  src/step_generator.cpp
  src/stutter.cpp
  src/trigger_operators.cpp
  src/value.cpp
  src/voice_handler.cpp)

target_include_directories(mopo PUBLIC src)

set_source_files_properties(src/chorus.cpp PROPERTIES COMPILE_OPTIONS
  "$<$<CXX_COMPILER_ID:MSVC>:/fp:precise>;$<$<CXX_COMPILER_ID:GNU,Clang>:-fno-finite-math-only>")


set_target_properties(mopo PROPERTIES CXX_EXTENSIONS OFF)

if(HELMBOY_DEBUG_DSP_PREP)
  target_compile_definitions(mopo PRIVATE HELMBOY_DEBUG_DSP_PREP=1)
endif()

if(HELMBOY_ENABLE_BIQUAD_JUCE_PATH)
  target_compile_definitions(mopo PRIVATE HELMBOY_ENABLE_BIQUAD_JUCE_PATH=1)
endif()

if(HELMBOY_ENABLE_SVF_JUCE_PATH)
  target_compile_definitions(mopo PRIVATE HELMBOY_ENABLE_SVF_JUCE_PATH=1)
endif()

if(HELMBOY_DEBUG_BIQUAD_ROUTING)
  target_compile_definitions(mopo PRIVATE HELMBOY_DEBUG_BIQUAD_ROUTING=1)
endif()

if(HELMBOY_DEBUG_SVF_TRANSITIONS)
  target_compile_definitions(mopo PRIVATE HELMBOY_DEBUG_SVF_TRANSITIONS=1)
endif()

if(HELMBOY_DEBUG_REVERB)
  target_compile_definitions(mopo PRIVATE HELMBOY_DEBUG_REVERB=1)
endif()

# Aggressive optimisations pour l'audio uniquement en Release/RelWithDebInfo
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  if(MSVC)
    # MSVC : vectorisation et calcul rapide, uniquement Release
    target_compile_options(mopo PRIVATE
      $<$<CONFIG:Release>:/O2 /Oi /Ot /Oy /GL /favor:INTEL64 /arch:AVX2>
    )
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    # Clang/GCC : vectorisation et optimisations math
    set(MOPO_ARCH_FLAGS "")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$" OR
       CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
      list(APPEND MOPO_ARCH_FLAGS
        "$<$<CONFIG:Release>:-mavx2>"
        "$<$<CONFIG:Release>:-mfma>")
    endif()
    target_compile_options(mopo PRIVATE
      -O3 -ffast-math -ftree-vectorize -funroll-loops
      ${MOPO_ARCH_FLAGS}
    )
  endif()

  # D�finitions SIMD
  target_compile_definitions(mopo PRIVATE
    MOPO_USE_SIMD=1
    MOPO_ENABLE_VECTORIZATION=1
  )
endif()
