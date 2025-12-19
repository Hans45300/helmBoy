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

#include "envelope.h"

#include "sample_decay_lookup.h"

#include <cmath>

#define ATTACK_DONE 0.999

namespace mopo {

  /**
   * @file envelope.cpp
   * @brief Implementation of the ADSR-style envelope used by the synth.
   *
   * The Envelope processor provides a simple multi-stage envelope with
   * Attack -> Decay -> Sustain -> Release semantics plus a short 'Kill'
   * mode used to rapidly silence a voice. The processor runs at control
   * rate (see Processor constructor) and interpolates its output across the
   * current audio block to produce smooth, sample-accurate envelope values.
   */

  /**
   * @brief Construct a new Envelope processor.
   *
   * Initializes the processor in the Releasing state with a zeroed output
   * value. The processor is set to control rate (buffer_size == 1) in the
   * base Processor class via the constructor call.
   */
  Envelope::Envelope() :
    Processor(static_cast<int>(Inputs::NumInputs), static_cast<int>(Outputs::NumOutputs), true),
    state_(State::Releasing), current_value_(0.0) { }

  /**
   * @brief Handle incoming trigger events.
   * @param event Encoded event (kVoiceOn/kVoiceOff/kVoiceKill/kVoiceReset).
   *
   * The envelope reacts to the following events:
   * - `kVoiceOn` / `kVoiceReset`: start the Attack stage and reset the
   *   envelope output to 0. Also triggers the `Finished` output to notify
   *   downstream processors of the reset.
   * - `kVoiceOff`: begin the Release stage.
   * - `kVoiceKill`: immediate transition to Killing stage which quickly
   *   reduces the envelope to zero over `VOICE_KILL_TIME` seconds.
   */
  void Envelope::trigger(mopo_float event) {
    if (static_cast<int>(event) == static_cast<int>(kVoiceOn) || static_cast<int>(event) == static_cast<int>(kVoiceReset)) [[likely]] {
      state_ = State::Attacking;
      current_value_ = 0.0;

      output(static_cast<int>(Outputs::Finished))->trigger(kVoiceReset);
    }
    else if (static_cast<int>(event) == static_cast<int>(kVoiceOff)) [[likely]]
      state_ = State::Releasing;
    else if (static_cast<int>(event) == static_cast<int>(kVoiceKill)) [[unlikely]] {
      state_ = State::Killing;
    }
  }

  /**
   * @brief Main control-rate processing for the envelope.
   *
   * The processor computes the envelope output for the current block. It
   * supports gradual interpolation by computing how many samples remain in
   * the current attack phase and using exponentials for decay/release
   * stages via `SampleDecayLookup` which provides a precomputed per-sample
   * decay multiplier for a given time constant.
   */
  void Envelope::process() {
    // Clear the finished flag from the previous block.
    output(static_cast<int>(Outputs::Finished))->clearTrigger();

    // If a trigger arrived on the input, handle it first.
    if (input(static_cast<int>(Inputs::Trigger))->source->triggered)
      trigger(input(static_cast<int>(Inputs::Trigger))->source->trigger_value);

    // Expose the current state as a control output for debugging/visualization.
    output(static_cast<int>(Outputs::Phase))->buffer[0] = static_cast<mopo_float>(state_);

    int samples = 0;

    if (state_ == State::Attacking) {
      // Attack is handled by linear incrementation until an ATTACK_DONE threshold
      // is reached (ATTACK_DONE slightly less than 1.0 to avoid float edge cases).
      mopo_float attack = utils::max(input(static_cast<int>(Inputs::Attack))->at(0), 0.000000001);
      mopo_float attack_increment = 1.0 / (sample_rate_ * attack);
      samples = (ATTACK_DONE - current_value_) / attack_increment;

      if (samples < samples_to_process_) {
        // The attack will finish within the current block: clamp to 1.0 and
        // transition to the Decay stage.
        state_ = State::Decaying;
        current_value_ = 1.0;
        output(static_cast<int>(Outputs::Value))->buffer[0] = current_value_;
      }
      else {
        // Still attacking after this block: output current value and advance
        // the envelope by samples_to_process_ samples worth of attack.
        output(static_cast<int>(Outputs::Value))->buffer[0] = current_value_;
        current_value_ += samples_to_process_ * attack_increment;
      }
    }
    else if (state_ == State::Decaying) {
      // Decay is exponential towards the sustain level. The SampleDecayLookup
      // provides an efficient per-sample multiplier for a given decay time.
      mopo_float decay_samples = sample_rate_ * input(static_cast<int>(Inputs::Decay))->at(0);
      mopo_float sustain = input(static_cast<int>(Inputs::Sustain))->at(0);

      mopo_float decay_decay_ = SampleDecayLookup::sampleDecayLookup(decay_samples);
      mopo_float leftover_samples = samples_to_process_ - samples;
      mopo_float delta = current_value_ - sustain;
      mopo_float end_delta = delta * pow(decay_decay_, leftover_samples);

      current_value_ = sustain + end_delta;
      output(static_cast<int>(Outputs::Value))->buffer[0] = current_value_;
    }
    else if (state_ == State::Releasing) {
      // Release stage behaves similarly to decay, exponentially approaching
      // zero with a time constant specified by the Release input.
      mopo_float release_samples = sample_rate_ * input(static_cast<int>(Inputs::Release))->at(0);

      mopo_float release_decay = SampleDecayLookup::sampleDecayLookup(release_samples);
      mopo_float leftover_samples = samples_to_process_ - samples;
      current_value_ = current_value_ * pow(release_decay, leftover_samples);
      output(static_cast<int>(Outputs::Value))->buffer[0] = current_value_;
    }
    else if (state_ == State::Killing) {
      // Kill mode is a hard ramp-down over VOICE_KILL_TIME seconds to ensure
      // the voice is silenced quickly without producing hard clicks.
      mopo_float decrement = samples_to_process_ / (VOICE_KILL_TIME * sample_rate_);
      current_value_ = utils::max(0.0, current_value_ - decrement);
      output(static_cast<int>(Outputs::Value))->buffer[0] = current_value_;
    }
  }
} // namespace mopo
