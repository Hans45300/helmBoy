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
#ifndef VALUE_SWITCH_H
#define VALUE_SWITCH_H

#include "value.h"

namespace mopo {

/**
 * @file value_switch.h
 * @brief Header for ValueSwitch: routes one input buffer to output and enables processors.
 */

  class ValueSwitch : public cr::Value {
    public:
      enum Outputs {
  kValue = Value::Inputs::Set,
        kSwitch,
        kNumOutputs
      };

      ValueSwitch(mopo_float value = 0.0);
      virtual void destroy() override;

      virtual Processor* clone() const override { return new ValueSwitch(*this); }
      virtual void process() override { }
      virtual void set(mopo_float value) override;

      void addProcessor(Processor* processor) { processors_.push_back(processor); }

    private:
      void setSource(int source);

      mopo_float* original_buffer_;
      std::vector<Processor*> processors_;
  };
} // namespace mopo

#endif // VALUE_SWITCH_H
