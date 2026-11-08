#pragma once

#ifndef JUCE_PROCESSOR_BASE_H
#define JUCE_PROCESSOR_BASE_H

#include "processor.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <array>
#include <vector>

namespace mopo {

template <typename JuceDSPType, std::size_t NumChannels>
class JuceProcessorBase : public Processor {
  public:
    explicit JuceProcessorBase(int num_inputs, int num_outputs, bool control_rate = false)
        : Processor(num_inputs, num_outputs, control_rate) {
      resizeScratchBuffers();
    }

    void setSampleRate(int sample_rate) override {
      Processor::setSampleRate(sample_rate);
      prepared_ = false;
    }

    void setBufferSize(int buffer_size) override {
      Processor::setBufferSize(buffer_size);
      resizeScratchBuffers();
      prepared_ = false;
    }

  protected:
    void ensurePrepared() {
      if (prepared_)
        return;

      juce::dsp::ProcessSpec spec {
        static_cast<double>(sample_rate_),
        static_cast<juce::uint32>(std::max(1, buffer_size_)),
        static_cast<juce::uint32>(NumChannels)
      };
      juce_processor_.prepare(spec);
      juce_processor_.reset();
      prepared_ = true;
    }

    void resetJuceProcessor() {
      ensurePrepared();
      juce_processor_.reset();
      clearScratch();
    }

    void clearScratch(std::size_t num_channels = NumChannels) {
      const auto sample_count = static_cast<std::size_t>(std::max(1, buffer_size_));
      for (std::size_t channel = 0; channel < num_channels; ++channel)
        std::fill_n(scratch_buffers_[channel].data(), sample_count, 0.0f);
    }

    void copyMonoInputToScratch(const mopo_float* source, std::size_t num_channels = 1) {
      ensurePrepared();
      const auto sample_count = static_cast<std::size_t>(buffer_size_);
      for (std::size_t channel = 0; channel < num_channels; ++channel)
        std::copy_n(source, sample_count, scratch_buffers_[channel].data());
    }

    juce::dsp::AudioBlock<float> scratchBlock(std::size_t num_channels = NumChannels) {
      ensurePrepared();
      return { scratch_channel_pointers_.data(), num_channels, static_cast<std::size_t>(buffer_size_) };
    }

    void copyScratchToMonoOutput(mopo_float* destination, std::size_t channel = 0) const {
      std::copy_n(scratch_buffers_[channel].data(), static_cast<std::size_t>(buffer_size_), destination);
    }

    void copyScratchToStereoOutputs(mopo_float* left, mopo_float* right) const {
      std::copy_n(scratch_buffers_[0].data(), static_cast<std::size_t>(buffer_size_), left);
      std::copy_n(scratch_buffers_[1].data(), static_cast<std::size_t>(buffer_size_), right);
    }

    JuceDSPType juce_processor_;

  private:
    void resizeScratchBuffers() {
      const auto sample_count = static_cast<std::size_t>(std::max(1, buffer_size_));
      for (std::size_t channel = 0; channel < NumChannels; ++channel) {
        scratch_buffers_[channel].resize(sample_count);
        scratch_channel_pointers_[channel] = scratch_buffers_[channel].data();
      }
    }

    bool prepared_ = false;
    std::array<std::vector<float>, NumChannels> scratch_buffers_ {};
    std::array<float*, NumChannels> scratch_channel_pointers_ {};
};

} // namespace mopo

#endif // JUCE_PROCESSOR_BASE_H
