/* Copyright 2025 Marc Scheffer - GNU GPL v3 */
#pragma once
#ifndef UTILS_H
#define UTILS_H

#include "common.h"
#include "value.h"
#include <xsimd/xsimd.hpp>
#include <cmath>
#include <span>
#include <cstdlib>
#include <concepts>
#include <type_traits>
#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#include <x86intrin.h>
#elif defined(__GNUC__) && defined(__ARM_NEON__)
#include <algorithm>
#endif

namespace mopo
{
  namespace
  {
    const mopo_float EPSILON = 1e-16;
    const mopo_float DB_GAIN_CONVERSION_MULT = 20.0;
    const mopo_float MIDI_0_FREQUENCY = 8.1757989156;
    const int NOTES_PER_OCTAVE = 12;
    const int CENTS_PER_NOTE = 100;
    const int CENTS_PER_OCTAVE = NOTES_PER_OCTAVE * CENTS_PER_NOTE;
    const int MAX_CENTS = MIDI_SIZE * CENTS_PER_NOTE;
    const mopo_float MAX_Q_POW = 4.0;
    const mopo_float MIN_Q_POW = -1.0;
  }

  namespace utils
  {
    /** @brief Règle de conversion d'une valeur waveform en index discret. */
    enum class WaveformIndexRounding {
      Nearest,
      Truncate
    };

    const Value value_zero(0.0);
    const Value value_one(1.0);
    const Value value_two(2.0);
    const Value value_half(0.5);
    const Value value_fifth(0.2);
    const Value value_tenth(0.1);
    const Value value_pi(PI);
    const Value value_2pi(2.0 * PI);
    const Value value_neg_one(-1.0);

    template <typename T>
    concept BufferLike = requires(T buf, size_t i) { { buf.size() } -> std::convertible_to<size_t>; { buf[i] } -> std::convertible_to<mopo_float>; };
    template <typename T>
    concept IntBufferLike = requires(T buf, size_t i) { { buf.size() } -> std::convertible_to<size_t>; { buf[i] } -> std::convertible_to<int>; };
    template <typename T>
    concept ProcessorLike = requires(T& proc) { { proc.process() } -> std::same_as<void>; };

    template <std::ranges::input_range Range>
      requires ProcessorLike<std::ranges::range_value_t<Range>>
    void processAll(Range &&processors)
    {
      for (auto &proc : processors)
        proc.process();
    }

    template <std::ranges::input_range Range>
      requires ProcessorLike<std::remove_pointer_t<std::ranges::range_value_t<Range>>>
    void processAllPtr(Range &&processors)
    {
      for (auto *proc : processors)
      {
        if (proc)
          proc->process();
      }
    }

    // --- FONCTIONS ARITHMÉTIQUES GÉNÉRIQUES OPTIMISÉES SIMD ---
    template <typename T1, typename T2>
    constexpr T1 min(T1 one, T2 two) { return (one < static_cast<T1>(two)) ? one : static_cast<T1>(two); }

    template <typename T1, typename T2>
    constexpr T1 max(T1 one, T2 two) { return (one > static_cast<T1>(two)) ? one : static_cast<T1>(two); }

    template <typename T1, typename T2, typename T3>
    constexpr T1 clamp(T1 value, T2 min_val, T3 max_val)
    {
      T1 mn = static_cast<T1>(min_val);
      T1 mx = static_cast<T1>(max_val);
      if (value < mn)
        return mn;
      return (value > mx) ? mx : value;
    }

    // Alias pour la compatibilité avec vos anciens appels entiers (réduits en surcharges transparentes)
    template <typename T1, typename T2>
    constexpr T1 imax(T1 one, T2 two) { return max(one, two); }
    template <typename T1, typename T2>
    constexpr T1 imin(T1 one, T2 two) { return min(one, two); }
    template <typename T1, typename T2, typename T3>
    constexpr T1 iclamp(T1 v, T2 mn, T3 mx) { return clamp(v, mn, mx); }

    inline int roundAndClampWaveformIndex(mopo_float value, int minimum, int maximum) {
      return iclamp(static_cast<int>(value + 0.5), minimum, maximum);
    }

    inline int truncateAndClampWaveformIndex(mopo_float value, int minimum, int maximum) {
      return iclamp(static_cast<int>(value), minimum, maximum);
    }

    inline int waveformIndex(mopo_float value, int minimum, int maximum,
                             WaveformIndexRounding rounding) {
      return rounding == WaveformIndexRounding::Nearest
          ? roundAndClampWaveformIndex(value, minimum, maximum)
          : truncateAndClampWaveformIndex(value, minimum, maximum);
    }

#ifdef __SSE2__
    inline void enableDenormalFlushing(bool enable)
    {
      if (enable)
      {
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
      }
      else
      {
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_OFF);
      }
    }
#else
    inline void enableDenormalFlushing(bool enable) {}
#endif

    // --- LOGIQUE SIGNAL ET DSP ---
    inline double interpolate(double from, double to, double t) { return t * (to - from) + from; }
    inline float interpolate(float from, float to, float t) { return fmaf(t, to - from, from); }
    inline mopo_float remapPhase(mopo_float phase, mopo_float stretch)
    {
      if (phase < stretch)
        return (phase / stretch) * 0.5;
      return 0.5 + ((phase - stretch) / (1.0 - stretch)) * 0.5;
    }
    inline unsigned int phaseToFixedPoint(mopo_float degrees)
    {
      if (!std::isfinite(degrees))
        return 0;
      const mopo_float wrapped_degrees = std::fmod(degrees, 360.0);
      const auto phase = static_cast<std::int64_t>(wrapped_degrees * (4294967296.0 / 360.0));
      return static_cast<unsigned int>(phase);
    }
    inline mopo_float mod(double value, double *integral) { return modf(value, integral); }
    inline float mod(float value, float *integral) { return modff(value, integral); }
    inline bool closeToZero(mopo_float value) { return value <= EPSILON && value >= -EPSILON; }
    inline bool closeToZerof(float value) { return value <= EPSILON && value >= -EPSILON; }
    inline mopo_float gainToDb(mopo_float gain) { return DB_GAIN_CONVERSION_MULT * log10(gain); }
    inline mopo_float dbToGain(mopo_float decibels) { return pow(10.0, decibels / DB_GAIN_CONVERSION_MULT); }
    inline mopo_float centsToRatio(mopo_float cents) { return pow(2.0, cents / CENTS_PER_OCTAVE); }
    inline mopo_float midiCentsToFrequency(mopo_float cents) { return MIDI_0_FREQUENCY * centsToRatio(cents); }
    inline mopo_float midiNoteToFrequency(mopo_float note) { return midiCentsToFrequency(note * CENTS_PER_NOTE); }
    inline mopo_float frequencyToMidiNote(mopo_float frequency) { return NOTES_PER_OCTAVE * log(frequency / MIDI_0_FREQUENCY) / log(2.0); }
    inline mopo_float frequencyToMidiCents(mopo_float frequency) { return CENTS_PER_NOTE * frequencyToMidiNote(frequency); }
    inline mopo_float magnitudeToQ(mopo_float magnitude) { return pow(2.0, interpolate(MIN_Q_POW, MAX_Q_POW, magnitude)); }
    inline mopo_float qToMagnitude(mopo_float q) { return (pow(0.5, q) - MIN_Q_POW) / (MAX_Q_POW - MIN_Q_POW); }
    inline int nextPowerOfTwo(mopo_float value) { return round(pow(2.0, ceil(log(value) / log(2.0)))); }
    inline mopo_float quickerTanh(mopo_float value)
    {
      mopo_float square = value * value;
      return value / (1.0 + square / (3.0 + square / 5.0));
    }

    inline mopo_float quickTanh(mopo_float value)
    {
      mopo_float abs_value = fabs(value), square = value * value;
      mopo_float num = value * (2.45550750702956 + 2.45550750702956 * abs_value + square * (0.893229853513558 + 0.821226666969744 * abs_value));
      mopo_float den = 2.44506634652299 + (2.44506634652299 + square) * fabs(value + 0.814642734961073 * value * abs_value);
      return num / den;
    }

    inline mopo_float quickerSin(mopo_float phase) { return phase * (8.0 - 16.0 * fabs(phase)); }
    inline mopo_float quickSin(mopo_float phase)
    {
      mopo_float approx = quickerSin(phase);
      return approx * (0.776 + 0.224 * fabs(approx));
    }
    inline mopo_float quickerSin1(mopo_float phase)
    {
      phase = 0.5 - phase;
      return phase * (8.0 - 16.0 * fabs(phase));
    }
    inline mopo_float quickSin1(mopo_float phase)
    {
      mopo_float approx = quickerSin1(phase);
      return approx * (0.776 + 0.224 * fabs(approx));
    }

    inline bool isSilent(std::span<const mopo_float> buffer)
    {
      for (auto v : buffer)
      {
        if (!closeToZero(v))
          return false;
      }
      return true;
    }
    inline bool isSilentf(std::span<const float> buffer)
    {
      for (auto v : buffer)
      {
        if (!closeToZerof(v))
          return false;
      }
      return true;
    }
    inline mopo_float rms(std::span<const mopo_float> buffer)
    {
      mopo_float square_total = 0.0;
      for (auto v : buffer)
        square_total += v * v;
      return sqrt(square_total / buffer.size());
    }
    inline mopo_float peak(std::span<const mopo_float> buffer, int skip)
    {
      mopo_float peak_val = 0.0;
      for (size_t i = 0; i < buffer.size(); i += skip)
        peak_val = fmax(peak_val, fabs(buffer[i]));
      return peak_val;
    }

    template <IntBufferLike Buffer>
    inline void zeroBuffer(Buffer &&buffer)
    {
      for (auto &v : buffer)
        v = 0;
    }

    template <BufferLike Dest, BufferLike Src>
    inline void copyBuffer(Dest &&dest, Src &&source)
    {
      using batch = xsimd::batch<mopo_float>;
      constexpr std::size_t simd_size = batch::size;
      int size = std::min(dest.size(), source.size()), simd_end = size - (size % simd_size), i = 0;
      for (; i < simd_end; i += simd_size)
      {
        batch data = batch::load_unaligned(&source[i]);
        data.store_unaligned(&dest[i]);
      }
      for (; i < size; ++i)
        dest[i] = source[i];
    }

    inline void copyBufferf(std::span<float> dest, std::span<const float> source)
    {
      int size = std::min(dest.size(), source.size());
      VECTORIZE_LOOP for (int i = 0; i < size; ++i) dest[i] = source[i];
    }
  } // namespace utils
} // namespace mopo

#endif // UTILS_H
