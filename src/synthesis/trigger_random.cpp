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

#include "trigger_random.h"

#include <cstdlib>

namespace mopo {

/**
 * @file trigger_random.cpp
 * @brief Implementation of TriggerRandom: sample-and-hold random generator triggered by input.
 */

  TriggerRandom::TriggerRandom() : Processor(1, 1, true), value_(0.0) { }

  void TriggerRandom::process() {
    if (input()->source->triggered)
      value_ = 2.0 * rand() / RAND_MAX - 1.0;

    output()->buffer[0] = value_;
  }
} // namespace mopo
