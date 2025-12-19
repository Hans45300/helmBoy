/* Copyright 2025 Marc Scheffer
 *
 * helmBoy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *
 * This work is based on bepzi's Helm project, <https://github.com/bepzi/helm>,
 * itself based on Matt Tytel's Helm <https://tytel.org/helm/>
 *
 * helmBoy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with helmBoy.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef SAFE_AUDIO_BUFFER_H
#define SAFE_AUDIO_BUFFER_H

#include <span>
#include <vector>
#include <cassert>
#include <algorithm>
#include <execution>
#include <ranges>
#include <concepts>
// Concept : type flottant (float/double)
template<typename T>
concept FloatingPoint = std::floating_point<T>;

// Concept : AudioBufferLike (doit avoir SampleType, getArrayOfWritePointers, getNumChannels, getNumSamples)
template<typename T>
concept AudioBufferLike = requires(T a) {
    typename T::SampleType;
    { a.getArrayOfWritePointers() } -> std::same_as<typename T::SampleType* const*>;
    { a.getNumChannels() } -> std::convertible_to<int>;
    { a.getNumSamples() } -> std::convertible_to<int>;
};

/**
 * @file safe_audio_buffer.h
 * @brief Modern C++20 safe audio buffer management with std::span and SIMD optimizations.
 */

namespace helmboy {

/**
 * @brief SafeAudioBuffer utilities and wrappers namespace.
 *
 * Provides a lightweight, safe wrapper around raw audio buffers and
 * helper functions to interoperate with JUCE audio types.
 */

/**
 * @brief Safe wrapper for audio buffers using std::span
 *
 * SafeAudioBuffer provides a modern C++20 interface for audio buffer operations
 * with automatic bounds checking and memory safety. It wraps audio data in
 * std::span objects that provide safe access without performance overhead.
 *
 * @tparam SampleType The audio sample type (float, double)
 */
template<FloatingPoint SampleType = float>
class SafeAudioBuffer {
public:
    using SpanType = std::span<SampleType>;
    using ConstSpanType = std::span<const SampleType>;

    /**
     * @brief Construct empty buffer
     */
    SafeAudioBuffer() = default;

    /**
     * @brief Construct from raw buffer data
     * @param data Pointer to audio data
     * @param num_channels Number of audio channels
     * @param num_samples Number of samples per channel
     */
    SafeAudioBuffer(SampleType* const* data, int num_channels, int num_samples)
        : num_channels_(num_channels), num_samples_(num_samples) {

        assert(data != nullptr);
        assert(num_channels > 0);
        assert(num_samples > 0);

        for (int channel = 0; channel < num_channels; ++channel) {
            assert(data[channel] != nullptr);
            channels_.emplace_back(data[channel], num_samples);
        }
    }    /**
     * @brief Construct from JUCE AudioBuffer
     * @param audio_buffer Reference to JUCE AudioBuffer
     */
    template<AudioBufferLike AudioBuffer>
    explicit SafeAudioBuffer(AudioBuffer& audio_buffer)
        : SafeAudioBuffer(audio_buffer.getArrayOfWritePointers(),
                         audio_buffer.getNumChannels(),
                         audio_buffer.getNumSamples()) {}

    /**
     * @brief Get safe span for specific channel
     * @param channel Channel index
        * @return SpanType std::span for the channel data
     */
    [[nodiscard]] SpanType getChannelSpan(int channel) {
        assert(channel >= 0 && channel < num_channels_);
        return channels_[channel];
    }

    /**
     * @brief Get const safe span for specific channel
     * @param channel Channel index
        * @return ConstSpanType const std::span for the channel data
     */
    [[nodiscard]] ConstSpanType getChannelSpan(int channel) const {
        assert(channel >= 0 && channel < num_channels_);
        return channels_[channel];
    }

    /**
     * @brief Get all channels as spans
     * @return const std::vector<SpanType>& Vector of spans for all channels
     */
    [[nodiscard]] const std::vector<SpanType>& getAllChannels() const {
        return channels_;
    }

    /**
     * @brief Add samples from another SafeAudioBuffer into this buffer.
     * @param other Source buffer to add from.
     * @param start_sample Starting sample index in both buffers.
     * @param num_samples_to_process Number of samples to process (defaults to the remaining sample count).
     */
    void addFrom(const SafeAudioBuffer& other, int start_sample = 0, int num_samples_to_process = -1) {
        if (num_samples_to_process == -1) {
            num_samples_to_process = std::min(num_samples_ - start_sample,
                                            other.num_samples_ - start_sample);
        }

        assert(start_sample >= 0);
        assert(start_sample + num_samples_to_process <= num_samples_);
        assert(start_sample + num_samples_to_process <= other.num_samples_);
        assert(num_channels_ == other.num_channels_);

        for (int channel = 0; channel < num_channels_; ++channel) {
            auto dest = channels_[channel].subspan(start_sample, num_samples_to_process);
            auto src = other.channels_[channel].subspan(start_sample, num_samples_to_process);

            // Use SIMD-friendly transform with vectorization hints
            if constexpr (std::is_same_v<SampleType, float>) {
                // Optimized path for float (most common case)
                #ifdef _MSC_VER
                __pragma(loop(ivdep))
                #endif
                for (size_t i = 0; i < dest.size(); ++i) {
                    dest[i] += src[i];
                }
            } else {
                // Generic fallback
                std::ranges::transform(src, dest, dest.begin(), [](SampleType b, SampleType a) { return a + b; });
            }
        }
    }

    /**
     * @brief Copy samples from another SafeAudioBuffer into this buffer.
     * @param other Source buffer to copy from.
     * @param start_sample Starting sample index in both buffers.
     * @param num_samples_to_process Number of samples to process.
     */
    void copyFrom(const SafeAudioBuffer& other, int start_sample = 0, int num_samples_to_process = -1) {
        if (num_samples_to_process == -1) {
            num_samples_to_process = std::min(num_samples_ - start_sample,
                                            other.num_samples_ - start_sample);
        }

        assert(start_sample >= 0);
        assert(start_sample + num_samples_to_process <= num_samples_);
        assert(start_sample + num_samples_to_process <= other.num_samples_);
        assert(num_channels_ == other.num_channels_);

        for (int channel = 0; channel < num_channels_; ++channel) {
            auto dest = channels_[channel].subspan(start_sample, num_samples_to_process);
            auto src = other.channels_[channel].subspan(start_sample, num_samples_to_process);

            // Use optimized copy for contiguous memory
            std::ranges::copy(src, dest.begin());
        }
    }

    /**
     * @brief Clear (zero) a region of the buffer.
     * @param start_sample Starting sample index.
     * @param num_samples_to_clear Number of samples to clear.
     */
    void clear(int start_sample = 0, int num_samples_to_clear = -1) {
        if (num_samples_to_clear == -1) {
            num_samples_to_clear = num_samples_ - start_sample;
        }

        assert(start_sample >= 0);
        assert(start_sample + num_samples_to_clear <= num_samples_);

        for (int channel = 0; channel < num_channels_; ++channel) {
            auto span = channels_[channel].subspan(start_sample, num_samples_to_clear);

            // Use vectorized fill for better SIMD utilization
            std::ranges::fill(span, SampleType{0});
        }
    }

    /**
     * @brief Apply a gain multiplier to a region of the buffer.
     * @param gain Gain value to apply.
     * @param start_sample Starting sample index.
     * @param num_samples_to_process Number of samples to process.
     */
    void applyGain(SampleType gain, int start_sample = 0, int num_samples_to_process = -1) {
        if (num_samples_to_process == -1) {
            num_samples_to_process = num_samples_ - start_sample;
        }

        assert(start_sample >= 0);
        assert(start_sample + num_samples_to_process <= num_samples_);

        for (int channel = 0; channel < num_channels_; ++channel) {
            auto span = channels_[channel].subspan(start_sample, num_samples_to_process);

            // Optimized gain application with vectorization hints
            if constexpr (std::is_same_v<SampleType, float>) {
                // Optimized path for float with compiler hints
                #ifdef _MSC_VER
                __pragma(loop(ivdep))
                #endif
                for (size_t i = 0; i < span.size(); ++i) {
                    span[i] *= gain;
                }
            } else {
                // Generic path with parallel execution
                std::ranges::transform(span, span.begin(), [gain](SampleType sample) { return sample * gain; });
            }
        }
    }

    // Getters
    [[nodiscard]] int getNumChannels() const { return num_channels_; }
    [[nodiscard]] int getNumSamples() const { return num_samples_; }
    [[nodiscard]] bool isEmpty() const { return num_channels_ == 0 || num_samples_ == 0; }

private:
    std::vector<SpanType> channels_;
    int num_channels_ = 0;
    int num_samples_ = 0;
};

/**
 * @brief Convenience function to create SafeAudioBuffer from JUCE AudioBuffer
 * @tparam AudioBuffer JUCE AudioBuffer type
 * @param audio_buffer Reference to JUCE AudioBuffer
 * @return SafeAudioBuffer wrapping the JUCE buffer
 */
template<AudioBufferLike AudioBuffer>
[[nodiscard]] auto makeSafeBuffer(AudioBuffer& audio_buffer) {
    using SampleType = typename AudioBuffer::SampleType;
    return SafeAudioBuffer<SampleType>(audio_buffer);
}

} // namespace helmboy

#endif // SAFE_AUDIO_BUFFER_H
