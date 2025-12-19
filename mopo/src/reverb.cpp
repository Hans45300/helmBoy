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
#include "reverb_all_pass.h"
#include "reverb_comb.h"
#include "reverb_tuning.h"

namespace mopo {

  Reverb::Reverb() : ProcessorRouter(static_cast<int>(Inputs::NumInputs), 2), current_dry_(0.0f), current_wet_(0.0f) {
    static const Value gain(FIXED_GAIN);

    Bypass* audio_input = new Bypass();
    LinearSmoothBuffer* feedback_input = new LinearSmoothBuffer();
    cr::Clamp* damping_clamp = new cr::Clamp(0.0f, 1.0f);
    LinearSmoothBuffer* damping_input = new LinearSmoothBuffer();

  registerInput(audio_input->input(), static_cast<int>(Inputs::Audio));
  registerInput(feedback_input->input(), static_cast<int>(Inputs::Feedback));
  registerInput(damping_clamp->input(0), static_cast<int>(Inputs::Damping));
    damping_input->plug(damping_clamp);

    Multiply* gained_input = new Multiply();
    gained_input->plug(audio_input, 0);
    gained_input->plug(&gain, 1);

    addProcessor(audio_input);
    addProcessor(gained_input);
    addProcessor(feedback_input);
    addProcessor(damping_clamp);
    addProcessor(damping_input);

    VariableAdd* left_comb_total = new VariableAdd(NUM_COMB);
    for (int i = 0; i < NUM_COMB; ++i) {
      ReverbComb* comb = new ReverbComb(1 + mopo::MAX_SAMPLE_RATE * COMB_TUNINGS[i]);
      Value* time = new cr::Value(COMB_TUNINGS[i]);
      addIdleProcessor(time);
      cr::TimeToSamples* samples = new cr::TimeToSamples();
      samples->plug(time);

  comb->plug(gained_input, static_cast<int>(ReverbComb::Inputs::Audio));
  comb->plug(samples, static_cast<int>(ReverbComb::Inputs::SampleDelay));
  comb->plug(feedback_input, static_cast<int>(ReverbComb::Inputs::Feedback));
  comb->plug(damping_input, static_cast<int>(ReverbComb::Inputs::Damping));
      left_comb_total->plugNext(comb);
      addProcessor(samples);
      addProcessor(comb);
    }

    VariableAdd* right_comb_total = new VariableAdd(NUM_COMB);
    for (int i = 0; i < NUM_COMB; ++i) {
      mopo_float tuning = COMB_TUNINGS[i] + STEREO_SPREAD;
      ReverbComb* comb = new ReverbComb(1 + mopo::MAX_SAMPLE_RATE * tuning);
      Value* time = new cr::Value(tuning);
      addIdleProcessor(time);
      cr::TimeToSamples* samples = new cr::TimeToSamples();
      samples->plug(time);

  comb->plug(gained_input, static_cast<int>(ReverbComb::Inputs::Audio));
  comb->plug(samples, static_cast<int>(ReverbComb::Inputs::SampleDelay));
  comb->plug(feedback_input, static_cast<int>(ReverbComb::Inputs::Feedback));
  comb->plug(damping_input, static_cast<int>(ReverbComb::Inputs::Damping));
      right_comb_total->plugNext(comb);
      addProcessor(samples);
      addProcessor(comb);
    }

    addProcessor(left_comb_total);
    addProcessor(right_comb_total);

    reverb_wet_left_ = left_comb_total;
    for (int i = 0; i < NUM_ALL_PASS; ++i) {
      ReverbAllPass* all_pass = new ReverbAllPass(1 + mopo::MAX_SAMPLE_RATE * ALL_PASS_TUNINGS[i]);
      Value* time = new cr::Value(ALL_PASS_TUNINGS[i]);
      addIdleProcessor(time);
      cr::TimeToSamples* samples = new cr::TimeToSamples();
      samples->plug(time);

  all_pass->plug(reverb_wet_left_, static_cast<int>(ReverbAllPass::Inputs::Audio));
  all_pass->plug(samples, static_cast<int>(ReverbAllPass::Inputs::SampleDelay));
  all_pass->plug(&utils::value_half, static_cast<int>(ReverbAllPass::Inputs::Feedback));

      addProcessor(all_pass);
      addProcessor(samples);
      reverb_wet_left_ = all_pass;
    }

    reverb_wet_right_ = right_comb_total;
    for (int i = 0; i < NUM_ALL_PASS; ++i) {
      mopo_float tuning = ALL_PASS_TUNINGS[i] + STEREO_SPREAD;
      ReverbAllPass* all_pass = new ReverbAllPass(1 + mopo::MAX_SAMPLE_RATE * tuning);
      Value* time = new cr::Value(tuning);
      addIdleProcessor(time);
      cr::TimeToSamples* samples = new cr::TimeToSamples();
      samples->plug(time);

  all_pass->plug(reverb_wet_right_, static_cast<int>(ReverbAllPass::Inputs::Audio));
  all_pass->plug(samples, static_cast<int>(ReverbAllPass::Inputs::SampleDelay));
  all_pass->plug(&utils::value_half, static_cast<int>(ReverbAllPass::Inputs::Feedback));

      addProcessor(all_pass);
      addProcessor(samples);
      reverb_wet_right_ = all_pass;
    }
  }

  void Reverb::process() {
  MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Audio)));

    ProcessorRouter::process();
  const mopo_float* audio = input(static_cast<int>(Inputs::Audio))->source->buffer;
  const mopo_float* left_wet_audio = reverb_wet_left_->output()->buffer;
  const mopo_float* right_wet_audio = reverb_wet_right_->output()->buffer;
  mopo_float* dest_left = output(0)->buffer;
  mopo_float* dest_right = output(1)->buffer;

  mopo_float wet_in = utils::clamp(input(static_cast<int>(Inputs::Wet))->at(0), 0.0, 1.0);
    mopo_float next_wet = sqrt(wet_in);
    mopo_float next_dry = sqrt(1.0 - wet_in);
    mopo_float wet_inc = (next_wet - current_wet_) / buffer_size_;
    mopo_float dry_inc = (next_dry - current_dry_) / buffer_size_;

    #include <xsimd/xsimd.hpp>
    using batch = xsimd::batch<mopo_float>;
    constexpr std::size_t simd_size = batch::size;
    int simd_end = buffer_size_ - (buffer_size_ % simd_size);
    batch dry_start = batch(current_dry_);
    batch wet_start = batch(current_wet_);
    batch dry_inc_v = batch(dry_inc);
    batch wet_inc_v = batch(wet_inc);
    for (int i = 0; i < simd_end; i += simd_size) {
      alignas(alignof(batch)) mopo_float idx_arr[simd_size];
      for (std::size_t j = 0; j < simd_size; ++j) idx_arr[j] = static_cast<mopo_float>(i + static_cast<int>(j));
      batch idx = batch::load_aligned(idx_arr);
      batch dry = dry_start + idx * dry_inc_v;
      batch wet = wet_start + idx * wet_inc_v;
      batch a = batch::load_unaligned(&audio[i]);
      batch l = batch::load_unaligned(&left_wet_audio[i]);
      batch r = batch::load_unaligned(&right_wet_audio[i]);
      (dry * a + wet * l).store_unaligned(&dest_left[i]);
      (dry * a + wet * r).store_unaligned(&dest_right[i]);
    }
    for (int i = simd_end; i < buffer_size_; ++i) {
      mopo_float dry = current_dry_ + i * dry_inc;
      mopo_float wet = current_wet_ + i * wet_inc;
      dest_left[i] = dry * audio[i] + wet * left_wet_audio[i];
      dest_right[i] = dry * audio[i] + wet * right_wet_audio[i];
    }

    current_dry_ = next_dry;
    current_wet_ = next_wet;
  }
} // namespace mopo


























































































