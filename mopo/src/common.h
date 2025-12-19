/* Copyright 2025 Marc Scheffer
 *
 * mopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This work is based on bepzi's Helm project, <https://github.com/bepzi/helm>,
 * itself based on Matt Tytel's Helm <https://tytel.org/helm/>
 *
 * mopo is distributed in the hope that it will be useful,

 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mopo.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef COMMON_H
#define COMMON_H

/**
 * @file common.h
 * @brief Core definitions and constants for the mopo synthesis library
 * @ingroup mopo_modules
 *
 * This header contains fundamental definitions, constants, and utilities
 * used throughout the mopo synthesis engine. It defines the basic data types,
 * debugging macros, compiler-specific optimizations, and system constants.
 *
 * @section data_types Core Data Types
 *
 * - **mopo_float**: Primary floating-point type (double precision)
 * - **VoiceEvent**: Enumeration for voice state changes
 * - **Buffer constants**: Maximum sizes and default values
 * - **MIDI constants**: Channel and controller definitions
 *
 * @section optimization Compiler Optimizations
 *
 * The header provides compiler-specific optimizations:
 * - **VECTORIZE_LOOP**: Enables SIMD vectorization hints
 * - **MOPO_ASSERT**: Debug-only assertion macro
 * - **UNUSED**: Suppresses unused variable warnings
 *
 * @section constants System Constants
 *
 * Key system parameters:
 * - Audio buffer sizes and sample rates
 * - MIDI specifications and polyphony limits
 * - Mathematical constants and timing values
 * - Voice management parameters
 */

// Utilities.
#define UNUSED(x) (void)(x)

// SIMD detection and optimization hints
#if defined(_MSC_VER) && defined(_M_X64)
  #include <intrin.h>
  #define MOPO_SIMD_AVAILABLE 1
#elif defined(__GNUC__) || defined(__clang__)
  #if defined(__AVX2__)
    #include <immintrin.h>
    #define MOPO_SIMD_AVAILABLE 1
  #elif defined(__SSE4_1__)
    #include <smmintrin.h>
    #define MOPO_SIMD_AVAILABLE 1
  #endif
#endif

#ifndef MOPO_SIMD_AVAILABLE
  #define MOPO_SIMD_AVAILABLE 0
#endif

// Debugging.
#if DEBUG
#include <cassert>
#define MOPO_ASSERT(x) assert(x)
#else
#define MOPO_ASSERT(x) ((void)0)
#endif // DEBUG

// Vectorization hints - enhanced for better SIMD generation
#ifdef __clang__
#define VECTORIZE_LOOP _Pragma("clang loop vectorize(enable) interleave(enable) unroll(enable)")
#elif _MSC_VER
#define VECTORIZE_LOOP __pragma(loop(hint_parallel(8))) __pragma(loop(ivdep))
#else
#define VECTORIZE_LOOP _Pragma("GCC ivdep")
#endif

// Additional optimization hints for critical audio loops
#define AUDIO_RATE_LOOP VECTORIZE_LOOP
#define CONTROL_RATE_LOOP

// Memory alignment for SIMD-friendly data structures
#if MOPO_SIMD_AVAILABLE
  #define MOPO_ALIGN_SIMD alignas(32)  // AVX2 alignment
  #define MOPO_ALIGN_SSE alignas(16)   // SSE alignment
#else
  #define MOPO_ALIGN_SIMD
  #define MOPO_ALIGN_SSE
#endif

/**
 * @namespace mopo
 * @brief Core synthesis module namespace
 *
 * The mopo namespace contains all low-level synthesis modules, data types,
 * and utilities used by the HelmBoy synthesizer. This includes processors,
 * filters, oscillators, modulators, and supporting infrastructure.
 */
namespace mopo {

  typedef double mopo_float;

  const constexpr mopo_float PI = 3.1415926535897932384626433832795;
  const constexpr int MAX_BUFFER_SIZE = 256;
  const constexpr int DEFAULT_BUFFER_SIZE = 256;
  const constexpr int DEFAULT_SAMPLE_RATE = 44100;
  const constexpr int MAX_SAMPLE_RATE = 192000;
  const constexpr int MIDI_SIZE = 128;
  const constexpr int MAX_POLYPHONY = 33;

  const constexpr int PPQ = 960; // Pulses per quarter note.
  const constexpr mopo_float VOICE_KILL_TIME = 0.02;
  const constexpr int NUM_MIDI_CHANNELS = 16;

  // Common types of events across different Processors.
  enum VoiceEvent {
    kVoiceOff,     // Stop. (e.g. release in an envelope)
    kVoiceOn,      // Start. (e.g. start attack in an envelope)
    kVoiceReset,   // Reset. Immediately reset to initial state and play.
    kVoiceKill,   // Kill. Silence voice as fast as possible.
  };
} // namespace mopo

#endif // COMMON_H