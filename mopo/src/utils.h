
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
/**
 * @file utils.h
 * @brief Utilitaires mathématiques et helpers pour mopo (interpolation, buffers, conversions).
 *
 * Contient fonctions rapides pour le traitement numérique (sin approximatif, conversion
 * fréquence/midi, utilitaires de buffer SIMD-friendly) et petits concepts utilitaires.
 */
#ifndef UTILS_H
#define UTILS_H


#include "common.h"
#include "value.h"
#include <xsimd/xsimd.hpp>
#include <cmath>
#include <span>
#include <cstdlib>
#include <concepts>

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#include <x86intrin.h>
#elif defined(__GNUC__) && defined(__ARM_NEON__)
#include <algorithm>
#endif

namespace mopo {

  namespace {
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

  namespace utils {

    /**
     * @brief Espace de noms contenant fonctions utilitaires numériques et manipulation de buffers.
     */

    const Value value_zero(0.0);
    const Value value_one(1.0);
    const Value value_two(2.0);
    const Value value_half(0.5);
    const Value value_fifth(0.2);
    const Value value_tenth(0.1);
    const Value value_pi(PI);
    const Value value_2pi(2.0 * PI);
    const Value value_neg_one(-1.0);

    // Concept : BufferLike (pour buffers de float ou mopo_float)
    template <typename T>
    concept BufferLike = requires(T buf, size_t i) {
      { buf.size() } -> std::convertible_to<size_t>;
      { buf[i] } -> std::convertible_to<mopo_float>;
    };

    // Concept : IntBufferLike (pour buffers d'entiers)
    template <typename T>
    concept IntBufferLike = requires(T buf, size_t i) {
      { buf.size() } -> std::convertible_to<size_t>;
      { buf[i] } -> std::convertible_to<int>;
    };

    // Concept : ProcessorLike (pour tout objet ayant une m�thode process() renvoyant void)
    template <typename T>
    concept ProcessorLike = requires(T proc) {
      { proc.process() } -> std::same_as<void>;
    };

    // Exemple d'utilisation : fonction utilitaire qui force le process d'une s�quence de processeurs
    template <std::ranges::input_range Range>
      requires ProcessorLike<std::ranges::range_value_t<Range>>
    void processAll(Range &&processors)
    {
      for (auto &proc : processors)
      {
        proc.process();
      }
    }

    // Variante pour les conteneurs de pointeurs vers ProcessorLike
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

#ifdef __SSE2__
    inline double min(double one, double two) {
      _mm_store_sd(&one, _mm_min_sd(_mm_set_sd(one),_mm_set_sd(two)));
      return one;
    }

    inline double max(double one, double two) {
      _mm_store_sd(&one, _mm_max_sd(_mm_set_sd(one),_mm_set_sd(two)));
      return one;
    }

    inline double clamp(double value, double min, double max) {
      _mm_store_sd(&value, _mm_min_sd(_mm_max_sd(_mm_set_sd(value),
                                                 _mm_set_sd(min)),
                                      _mm_set_sd(max)));
      return value;
    }

      consteval double min(double one, double two) {
        return (one < two) ? one : two;
      }

      consteval double max(double one, double two) {
        return (one > two) ? one : two;
      }

      consteval double clamp(double value, double min, double max) {
        return (value < min) ? min : (value > max) ? max : value;
      }

    inline float min(float one, float two) {
      _mm_store_ss(&one, _mm_min_ss(_mm_set_ss(one),_mm_set_ss(two)));
      return one;
    }

    inline float max(float one, float two) {
      _mm_store_ss(&one, _mm_max_ss(_mm_set_ss(one),_mm_set_ss(two)));
      return one;
    }

    inline float clamp(float value, float min, float max) {
      _mm_store_ss(&value, _mm_min_ss(_mm_max_ss(_mm_set_ss(value),
                                                 _mm_set_ss(min)),
                                      _mm_set_ss(max)));
      return value;
    }

    inline void enableDenormalFlushing(bool enable) {
      if (enable) {
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
      }
      else {
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_OFF);
      }
    }

#else
    inline mopo_float min(mopo_float one, mopo_float two) {
      return fmin(one, two);
    }

    inline mopo_float max(mopo_float one, mopo_float two) {
      return fmax(one, two);
    }

    inline mopo_float clamp(mopo_float value, mopo_float min, mopo_float max) {
      return fmin(max, fmax(value, min));
    }

    inline void enableDenormalFlushing(bool enable) {
    }

#endif

    inline int imax(int one, int two) {
      return (one > two) ? one : two;
    }
    consteval int imax_consteval(int one, int two) {
      return (one > two) ? one : two;
    }

    inline int imin(int one, int two) {
      return (one > two) ? two : one;
    }
    consteval int imin_consteval(int one, int two) {
      return (one > two) ? two : one;
    }

    inline double interpolate(double from, double to, double t) {
      return t * (to - from) + from;
    }

    inline float interpolate(float from, float to, float t) {
      return fmaf(t, to - from, from);
    }

    inline mopo_float mod(double value, double* integral) {
      return modf(value, integral);
    }

    inline float mod(float value, float* integral) {
      return modff(value, integral);
    }

    inline mopo_float iclamp(int value, int min, int max) {
      return value > max ? max : (value < min ? min : value);
    }
    consteval int iclamp_consteval(int value, int min, int max) {
      return value > max ? max : (value < min ? min : value);
    }

    inline bool closeToZero(mopo_float value) {
      return value <= EPSILON && value >= -EPSILON;
    }

    inline bool closeToZerof(float value) {
      return value <= EPSILON && value >= -EPSILON;
    }

    inline mopo_float gainToDb(mopo_float gain) {
      return DB_GAIN_CONVERSION_MULT * log10(gain);
    }

    inline mopo_float dbToGain(mopo_float decibels) {
      return pow(10.0, decibels / DB_GAIN_CONVERSION_MULT);
    }

    inline mopo_float centsToRatio(mopo_float cents) {
      return pow(2.0, cents / CENTS_PER_OCTAVE);
    }

    inline mopo_float midiCentsToFrequency(mopo_float cents) {
      return MIDI_0_FREQUENCY * centsToRatio(cents);
    }

    inline mopo_float midiNoteToFrequency(mopo_float note) {
      return midiCentsToFrequency(note * CENTS_PER_NOTE);
    }

    inline mopo_float frequencyToMidiNote(mopo_float frequency) {
      return NOTES_PER_OCTAVE * log(frequency / MIDI_0_FREQUENCY) / log(2.0);
    }

    inline mopo_float frequencyToMidiCents(mopo_float frequency) {
      return CENTS_PER_NOTE * frequencyToMidiNote(frequency);
    }

    inline mopo_float magnitudeToQ(mopo_float magnitude) {
      return pow(2.0, interpolate(MIN_Q_POW, MAX_Q_POW, magnitude));
    }

    inline mopo_float qToMagnitude(mopo_float q) {
      return (pow(0.5, q) - MIN_Q_POW) / (MAX_Q_POW - MIN_Q_POW);
    }

    inline int nextPowerOfTwo(mopo_float value) {
      return round(pow(2.0, ceil(log(value) / log(2.0))));
    }

    inline mopo_float quickerTanh(mopo_float value) {
      mopo_float square = value * value;
      return value / (1.0 + square / (3.0 + square / 5.0));
    }

    inline mopo_float quickTanh(mopo_float value) {
      mopo_float abs_value = fabs(value);
      mopo_float square = value * value;

      mopo_float num = value * (2.45550750702956 + 2.45550750702956 * abs_value +
                                square * (0.893229853513558 + 0.821226666969744 * abs_value));
      mopo_float den = 2.44506634652299 + (2.44506634652299 + square) *
                       fabs(value + 0.814642734961073 * value * abs_value);
      return num / den;
    }

    // Version of quick sin where phase is is [-0.5, 0.5]
    inline mopo_float quickerSin(mopo_float phase) {
      return phase * (8.0 - 16.0 * fabs(phase));
    }

    inline mopo_float quickSin(mopo_float phase) {
      mopo_float approx = quickerSin(phase);
      return approx * (0.776 + 0.224 * fabs(approx));
    }

    // Version of quick sin where phase is is [0, 1]
    inline mopo_float quickerSin1(mopo_float phase) {
      phase = 0.5 - phase;
      return phase * (8.0 - 16.0 * fabs(phase));
    }

    inline mopo_float quickSin1(mopo_float phase) {
      mopo_float approx = quickerSin1(phase);
      return approx * (0.776 + 0.224 * fabs(approx));
    }

    /**
     * @brief Vérifie si un buffer est silencieux (tous les échantillons proches de zéro).
     * @param buffer Vue sur le buffer d'entrée.
     * @return `true` si silencieux, `false` sinon.
     */
    inline bool isSilent(std::span<const mopo_float> buffer) {
      for (auto v : buffer) {
        if (!closeToZero(v))
          return false;
      }
      return true;
    }

    /**
     * @brief Variante pour buffers `float`.
     */
    inline bool isSilentf(std::span<const float> buffer) {
      for (auto v : buffer) {
        if (!closeToZerof(v))
          return false;
      }
      return true;
    }

    /**
     * @brief Calcule la RMS d'un buffer.
     * @param buffer Buffer d'entrée.
     * @return Valeur RMS.
     */
    inline mopo_float rms(std::span<const mopo_float> buffer) {
      mopo_float square_total = 0.0;
      for (auto v : buffer)
        square_total += v * v;
      return sqrt(square_total / buffer.size());
    }

    /**
     * @brief Recherche le pic (valeur absolue maximale) dans un buffer en sautant des échantillons.
     * @param buffer Buffer d'entrée.
     * @param skip Pas d'échantillonnage pour la recherche (1 = tous les échantillons).
     * @return Valeur de crête.
     */
    inline mopo_float peak(std::span<const mopo_float> buffer, int skip) {
      mopo_float peak_val = 0.0;
      for (size_t i = 0; i < buffer.size(); i += skip)
        peak_val = fmax(peak_val, fabs(buffer[i]));
      return peak_val;
    }



    template<IntBufferLike Buffer>
    inline void zeroBuffer(Buffer&& buffer) {
      for (auto& v : buffer)
        v = 0;
    }

    /**
     * @brief Copie un buffer source vers une destination (optimisé SIMD si disponible).
     * @tparam Dest Type du buffer destination (doit respecter `BufferLike`).
     * @tparam Src Type du buffer source.
     * @param dest Destination.
     * @param source Source.
     */
    template<BufferLike Dest, BufferLike Src>
  inline void copyBuffer(Dest&& dest, Src&& source) {
      using batch = xsimd::batch<mopo_float>;
      constexpr std::size_t simd_size = batch::size;
      int size = std::min(dest.size(), source.size());
      int simd_end = size - (size % simd_size);
      int i = 0;
      for (; i < simd_end; i += simd_size) {
        batch data = batch::load_unaligned(&source[i]);
        data.store_unaligned(&dest[i]);
      }
      for (; i < size; ++i) {
        dest[i] = source[i];
      }
    }

    /**
     * @brief Copie un buffer `float` vers un `float` (non-template, vectorisé si possible).
     */
    inline void copyBufferf(std::span<float> dest, std::span<const float> source) {
      int size = std::min(dest.size(), source.size());
      VECTORIZE_LOOP
      for (int i = 0; i < size; ++i)
        dest[i] = source[i];
    }
  } // namespace utils
} // namespace mopo

#endif // UTILS_H
