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

#include "step_generator.h"
#include "utils.h"

#include <cmath>

namespace mopo {

/**
 * @file step_generator.cpp
 * @brief Step/sequence generator implementation.
 *
 * Produces step values and step indices for sequencing tasks. Supports
 * sample-accurate reset and time correction.
 */

  StepGenerator::StepGenerator(int max_steps)
    : Processor(static_cast<int>(Inputs::NumInputs) + max_steps,
                static_cast<int>(Outputs::NumOutputs), true),
      max_steps_(static_cast<unsigned int>(max_steps)),
      offset_(0.0f),
      current_step_(0u)
  { }

  void StepGenerator::process() {
    using enum Inputs;
    using enum Outputs;
    static mopo_float integral;
    unsigned int num_steps = static_cast<unsigned int>(input(static_cast<int>(Inputs::NumSteps))->at(0));
    num_steps = static_cast<unsigned int>(utils::iclamp(static_cast<int>(num_steps), 1, static_cast<int>(max_steps_)));

    int i = 0;
    if (input(static_cast<int>(Inputs::Reset))->source->triggered) {
      offset_ = 0.0f;
      current_step_ = 0u;
      i = input(static_cast<int>(Inputs::Reset))->source->trigger_offset;
    }

    offset_ += samples_to_process_ * input(static_cast<int>(Inputs::Frequency))->at(0) / sample_rate_;
    offset_ = utils::mod(offset_, &integral);
    current_step_ += static_cast<unsigned int>(integral);
    current_step_ = (current_step_ + num_steps) % num_steps;

    output(static_cast<int>(Outputs::Value))->buffer[0] = input(static_cast<int>(Inputs::Steps) + static_cast<int>(current_step_))->source->buffer[0];
    output(static_cast<int>(Outputs::Step))->buffer[0] = static_cast<mopo_float>(current_step_);
  }

  void StepGenerator::correctToTime(mopo_float samples) {
    using enum Inputs;
    static mopo_float integral;

    unsigned int num_steps = static_cast<unsigned int>(input(static_cast<int>(Inputs::NumSteps))->at(0));
    num_steps = static_cast<unsigned int>(utils::iclamp(static_cast<int>(num_steps), 1, static_cast<int>(max_steps_)));

    offset_ = samples * input(static_cast<int>(Inputs::Frequency))->at(0) / sample_rate_;
    offset_ = utils::mod(offset_, &integral);
    current_step_ = static_cast<unsigned int>(integral);
    current_step_ = (current_step_ + num_steps) % num_steps;
  }
} // namespace mopo
