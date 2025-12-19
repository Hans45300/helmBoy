cmake_minimum_required(VERSION 3.15)


project(MOPO
  VERSION 0.10.0
  DESCRIPTION "A modular and polyphonic audio synthesis library, by Matt Tytel."
  LANGUAGES CXX)

# Forcer l'utilisation du compilateur MSVC le plus récent (Visual Studio 2026)
if(MSVC)
  message(STATUS "Forçage de l'utilisation du compilateur MSVC le plus récent (Visual Studio 2026)")
  set(CMAKE_GENERATOR_TOOLSET "v143" CACHE STRING "Toolset for Visual Studio 2022/2025/2026" FORCE)
endif()

# Activer C++20 par défaut
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(mopo STATIC
  src/alias.cpp
  src/arpeggiator.cpp
  src/biquad_filter.cpp
  src/bit_crush.cpp
  src/bypass_router.cpp
  src/delay.cpp
  src/distortion.cpp
  src/envelope.cpp
  src/feedback.cpp
  src/formant_manager.cpp
  src/ladder_filter.cpp
  src/linear_slope.cpp
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
  src/reverb_all_pass.cpp
  src/reverb_comb.cpp
  src/reverb.cpp
  src/sample_decay_lookup.cpp
  src/simple_delay.cpp
  src/smooth_filter.cpp
  src/smooth_value.cpp
  src/state_variable_filter.cpp
  src/step_generator.cpp
  src/stutter.cpp
  src/trigger_operators.cpp
  src/value.cpp
  src/voice_handler.cpp)

target_include_directories(mopo PUBLIC src)



set_target_properties(mopo PROPERTIES CXX_EXTENSIONS OFF)

# Aggressive optimisations pour l'audio uniquement en Release/RelWithDebInfo
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  if(MSVC)
    # MSVC : vectorisation et calcul rapide, uniquement Release
    target_compile_options(mopo PRIVATE
      $<$<CONFIG:Release>:/O2 /Oi /Ot /Oy /GL /favor:INTEL64 /arch:AVX2>
    )
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    # Clang/GCC : vectorisation et optimisations math
    target_compile_options(mopo PRIVATE
      -O3 -ffast-math -ftree-vectorize -funroll-loops
      $<$<CONFIG:Release>:-mavx2>
      $<$<CONFIG:Release>:-mfma>
    )
  endif()

  # Définitions SIMD
  target_compile_definitions(mopo PRIVATE
    MOPO_USE_SIMD=1
    MOPO_ENABLE_VECTORIZATION=1
  )
endif()
