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

#include "reverb.h"

#include "operators.h"

#include <cmath>
#include <cstdio>

namespace {
  inline bool isFiniteReverbValue(mopo::mopo_float value) {
    return std::isfinite(static_cast<double>(value));
  }
}

namespace mopo {

  namespace {
    class ReverbAudioInput : public Processor {
      public:
        ReverbAudioInput() : Processor(2, 2) { }
        Processor* clone() const override { return new ReverbAudioInput(*this); }
        void process() override {
          const Output* left = input(0)->source;
          const Output* right = input(1)->source;
          if (right == &null_source_)
            right = left;
          MOPO_ASSERT(left->buffer_size >= buffer_size_ && right->buffer_size >= buffer_size_);
          for (int sample = 0; sample < buffer_size_; ++sample) {
            output(0)->buffer[sample] = left->buffer[sample];
            output(1)->buffer[sample] = right->buffer[sample];
          }
        }
    };
  }

  void Reverb::processJucePath(const mopo_float* audio,
                               const mopo_float* audio_right,
                               mopo_float* dest_left,
                               mopo_float* dest_right,
                               mopo_float wet_in) {
    // Safety checks: ensure buffers and size are valid
    if (!audio || !audio_right || !dest_left || !dest_right || buffer_size_ <= 0 || sample_rate_ <= 0) {
      // Keep deterministic output even when preconditions are not met.
      if (audio && audio_right && dest_left && dest_right && buffer_size_ > 0) {
        for (int i = 0; i < buffer_size_; ++i) {
          dest_left[i] = audio[i];
          dest_right[i] = audio_right[i];
        }
      }
      return;
    }

    // Lazy initialization: create juce::Reverb on first call.
    if (!juce_reverb_) {
      try {
        juce_reverb_ = std::make_unique<juce::Reverb>();
      } catch (...) {
        // Allocation failed: fail safe with dry pass-through.
        for (int i = 0; i < buffer_size_; ++i) {
          dest_left[i] = audio[i];
          dest_right[i] = audio_right[i];
        }
        return;
      }
    }

    // (Re)initialize juce::Reverb when the sample rate changes.
    if (juce_last_sample_rate_ != sample_rate_ && sample_rate_ > 0.0) {
      juce_reverb_->setSampleRate(static_cast<double>(sample_rate_));
      juce_reverb_->reset();
      juce_last_sample_rate_ = sample_rate_;
    }

    // Resize float conversion buffers if the block size has changed.
    if (static_cast<int>(juce_left_buf_.size()) != buffer_size_) {
      try {
        juce_left_buf_.assign(buffer_size_, 0.0f);
        juce_right_buf_.assign(buffer_size_, 0.0f);
      } catch (...) {
        // Buffer resize failed: fail safe with dry pass-through.
        for (int i = 0; i < buffer_size_; ++i) {
          dest_left[i] = audio[i];
          dest_right[i] = audio_right[i];
        }
        return;
      }
    }

    // Read StereoWidth [0, 1]; stub default in engine is 1.0 (full stereo).
    const mopo_float sw_raw = input(static_cast<int>(Inputs::StereoWidth))->at(0);
    const float stereo_width = isFiniteReverbValue(sw_raw)
        ? static_cast<float>(utils::clamp(sw_raw, 0.0, 1.0))
        : 1.0f;

    // Read FreezeMode [0, 1]. 0 = normal, >=0.5 = infinite decay.
    const mopo_float fm_raw = input(static_cast<int>(Inputs::FreezeMode))->at(0);
    const float freeze_mode = isFiniteReverbValue(fm_raw)
        ? static_cast<float>(utils::clamp(fm_raw, 0.0, 1.0))
        : 0.0f;

    // Map feedback [-1, 1] -> roomSize [0, 1].
    const mopo_float fb_raw = input(static_cast<int>(Inputs::Feedback))->at(0);
    const mopo_float fb_safe = isFiniteReverbValue(fb_raw)
        ? utils::clamp(fb_raw, -1.0, 1.0) : 0.7;
    const float room_size = static_cast<float>((fb_safe + 1.0) * 0.5);

    // Map damping [0, 1].
    const mopo_float damp_raw = input(static_cast<int>(Inputs::Damping))->at(0);
    const float damping = isFiniteReverbValue(damp_raw)
        ? static_cast<float>(utils::clamp(damp_raw, 0.0, 1.0))
        : 0.5f;

    // Map wet_in to juce::Reverb parameters, compensating for internal scale factors
    // (juce applies x3 to wetLevel and x2 to dryLevel, so we pre-divide to match
    // the established equal-power wet/dry mix).
    const float wet_sqrt = static_cast<float>(std::sqrt(wet_in));
    const float dry_sqrt = static_cast<float>(std::sqrt(1.0 - wet_in));

    juce::Reverb::Parameters params;
    params.roomSize   = room_size;
    params.damping    = damping;
    params.wetLevel   = wet_sqrt / 3.0f;
    params.dryLevel   = dry_sqrt / 2.0f;
    params.width      = stereo_width;
    params.freezeMode = freeze_mode;
    juce_reverb_->setParameters(params);

    // Convert both input channels without collapsing the stereo image.
    for (int i = 0; i < buffer_size_; ++i) {
      juce_left_buf_[i]  = static_cast<float>(audio[i]);
      juce_right_buf_[i] = static_cast<float>(audio_right[i]);
    }

    // Safety check: ensure juce_reverb_ is valid before using it
    if (!juce_reverb_) {
      // Fallback: copy audio directly if reverb isn't initialized
      for (int i = 0; i < buffer_size_; ++i) {
        dest_left[i] = audio[i];
        dest_right[i] = audio_right[i];
      }
      return;
    }

    juce_reverb_->processStereo(juce_left_buf_.data(), juce_right_buf_.data(), buffer_size_);

    // float->double to output buffers.
    for (int i = 0; i < buffer_size_; ++i) {
      dest_left[i]  = static_cast<mopo_float>(juce_left_buf_[i]);
      dest_right[i] = static_cast<mopo_float>(juce_right_buf_[i]);
    }
  }

  Reverb::Reverb() : ProcessorRouter(static_cast<int>(Inputs::NumInputs), 2) {
    ReverbAudioInput* audio_input = new ReverbAudioInput();
    LinearSmoothBuffer* feedback_input = new LinearSmoothBuffer();
    cr::Clamp* damping_clamp = new cr::Clamp(0.0f, 1.0f);
    LinearSmoothBuffer* damping_input = new LinearSmoothBuffer();
    LinearSmoothBuffer* stereo_width_input = new LinearSmoothBuffer();
    LinearSmoothBuffer* wet_input = new LinearSmoothBuffer();
    cr::Bypass* freeze_mode_input = new cr::Bypass();

  registerInput(audio_input->input(), static_cast<int>(Inputs::Audio));
  registerInput(audio_input->input(1), static_cast<int>(Inputs::AudioRight));
  registerInput(feedback_input->input(), static_cast<int>(Inputs::Feedback));
  registerInput(damping_clamp->input(0), static_cast<int>(Inputs::Damping));
  registerInput(stereo_width_input->input(), static_cast<int>(Inputs::StereoWidth));
  registerInput(wet_input->input(), static_cast<int>(Inputs::Wet));
  registerInput(freeze_mode_input->input(), static_cast<int>(Inputs::FreezeMode));
    damping_input->plug(damping_clamp);

    addProcessor(audio_input);
    addProcessor(feedback_input);
    addProcessor(damping_clamp);
    addProcessor(damping_input);
    addProcessor(stereo_width_input);
    addProcessor(wet_input);
    addProcessor(freeze_mode_input);
  }

  Reverb::Reverb(const Reverb& other)
      : ProcessorRouter(other)
        // juce_reverb_ left as nullptr for lazy initialization
  {
    // The unique_ptr juce_reverb_ is left empty; it will be created lazily
    // on the first call to processJucePath(). We don't copy it because
    // juce::Reverb has a deleted copy constructor.
  }

  void Reverb::resetState() {
    ProcessorRouter::resetState();
    if (juce_reverb_ != nullptr)
      juce_reverb_->reset();
  }

  void Reverb::process() {
  MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Audio)));

  const mopo_float* audio = input(static_cast<int>(Inputs::Audio))->source->buffer;
  const Output* right_source = input(static_cast<int>(Inputs::AudioRight))->source;
  if (right_source == &null_source_)
    right_source = input(static_cast<int>(Inputs::Audio))->source;
  MOPO_ASSERT(right_source->buffer_size >= buffer_size_);
  const mopo_float* audio_right = right_source->buffer;
  Output* out_left = output(0);
  Output* out_right = output(1);

  // Safety check: ensure outputs are valid and have buffers
  if (!out_left || !out_left->buffer || !out_right || !out_right->buffer) {
    // Fallback: silently return if outputs aren't ready
    return;
  }

  mopo_float* dest_left = out_left->buffer;
  mopo_float* dest_right = out_right->buffer;

  mopo_float wet_in = input(static_cast<int>(Inputs::Wet))->at(0);
#if defined(HELMBOY_DEBUG_REVERB)
    const mopo_float wet_in_raw = wet_in;
#endif
    if (!isFiniteReverbValue(wet_in)) {
#if defined(HELMBOY_DEBUG_REVERB)
      std::fprintf(stderr, "[HelmBoy][Reverb] non-finite wet_in=%g ? forced 0\n",
                   static_cast<double>(wet_in_raw));
#endif
      wet_in = 0.0;
    }
    wet_in = utils::clamp(wet_in, 0.0, 1.0);
#if defined(HELMBOY_DEBUG_REVERB)
    if (std::isfinite(static_cast<double>(wet_in_raw)) && wet_in_raw != wet_in)
      std::fprintf(stderr, "[HelmBoy][Reverb] wet_in clamped raw=%g ? %g\n",
                   static_cast<double>(wet_in_raw), static_cast<double>(wet_in));
#endif

    processJucePath(audio, audio_right, dest_left, dest_right, wet_in);
  }
} // namespace mopo
