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


#include "portamento_slope.h"
#include "utils.h"
#include <cmath>
#include <iostream>

namespace mopo {

  PortamentoSlope::PortamentoSlope()
    : Processor(static_cast<int>(Inputs::NumInputs), 1) {
    last_value_ = 0.0f;
  }

  /**
   * @brief Passe la valeur cible directement en sortie (bypass).
   */
  void PortamentoSlope::processBypass(int start) {
    mopo_float* dest = output(0)->buffer;
    const mopo_float* src = input(static_cast<int>(Inputs::Target))->source->buffer;
    utils::copyBuffer(
      std::span<mopo_float>(dest + start, buffer_size_ - start),
      std::span<const mopo_float>(src, buffer_size_ - start)
    );
    last_value_ = dest[buffer_size_ - 1];
  }

  /**
   * @brief Traite les triggers d'entr�e.
   */
  void PortamentoSlope::processTriggers() {
    output()->clearTrigger();

    if (input(static_cast<int>(Inputs::TriggerJump))->source->triggered) {
      int offset = input(static_cast<int>(Inputs::TriggerJump))->source->trigger_offset;
      output()->trigger(input(static_cast<int>(Inputs::Target))->at(offset), offset);
    }
    else if (input(static_cast<int>(Inputs::TriggerStart))->source->triggered) {
      float value = input(static_cast<int>(Inputs::TriggerStart))->source->trigger_value;
      output()->trigger(value, input(static_cast<int>(Inputs::TriggerStart))->source->trigger_offset);
    }
  }

  /**
   * @brief Traite le portamento/slope sur le buffer.
   */
  void PortamentoSlope::process() {
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Target)));

    processTriggers();
    auto state = static_cast<State>(static_cast<int>(input(static_cast<int>(Inputs::PortamentoType))->at(0)));
    mopo_float run_seconds = input(static_cast<int>(Inputs::RunSeconds))->at(0);
    if (state == State::Off || utils::closeToZero(run_seconds)) {
      processBypass(0);
      return;
    }

    mopo_float increment = 0.4f / (sample_rate_ * input(static_cast<int>(Inputs::RunSeconds))->at(0));
    mopo_float decay = 0.07f / (sample_rate_ * input(static_cast<int>(Inputs::RunSeconds))->at(0));
    const mopo_float* targets = input(static_cast<int>(Inputs::Target))->source->buffer;

    int i = 0;
    int note_number = static_cast<int>(input(static_cast<int>(Inputs::NoteNumber))->source->trigger_value);

    if (state == State::Auto && note_number <= 1 && input(static_cast<int>(Inputs::TriggerJump))->source->triggered) {
      int trigger_offset = input(static_cast<int>(Inputs::TriggerJump))->source->trigger_offset;
      for (; i < trigger_offset; ++i)
        tick(i, targets[i], increment, decay);

      last_value_ = input(static_cast<int>(Inputs::Target))->at(trigger_offset);
    }
    else if (input(static_cast<int>(Inputs::TriggerStart))->source->triggered) {
      int trigger_offset = input(static_cast<int>(Inputs::TriggerStart))->source->trigger_offset;
      for (; i < trigger_offset; ++i)
        tick(i, targets[i], increment, decay);

      last_value_ = input(static_cast<int>(Inputs::TriggerStart))->source->trigger_value;
    }

    if (last_value_ == input(static_cast<int>(Inputs::Target))->at(0) &&
        last_value_ == input(static_cast<int>(Inputs::Target))->at(buffer_size_ - 1)) {
      processBypass(i);
    }
    else {
      for (; i < buffer_size_; ++i)
        tick(i, targets[i], increment, decay);
    }
  }

  /**
   * @brief Applique un tick d'interpolation vers la cible.
   */
  inline void PortamentoSlope::tick(int i, mopo_float target,
                                    mopo_float increment, mopo_float decay) {
    if (target <= last_value_)
      last_value_ = utils::clamp(last_value_ - increment, target, last_value_);
    else
      last_value_ = utils::clamp(last_value_ + increment, last_value_, target);

    mopo_float movement = target - last_value_;
    movement *= std::fabs(movement);
    last_value_ += movement * decay;
    output()->buffer[i] = last_value_;
  }

} // namespace mopo
