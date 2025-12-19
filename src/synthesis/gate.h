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
#ifndef GATE_H
#define GATE_H

#include "value.h"

namespace mopo {

  class Gate : public Processor {
    public:
      enum Inputs {
        kChoice,
        kNumInputs
      };

      Gate();

      virtual void destroy() override;
      virtual Processor* clone() const override { return new Gate(*this); }
      void process() override;

    private:
      void setSource(int source);

      mopo_float* original_buffer_;
  };
/**
 * @file gate.h
 * @brief Gate processor header: selects one of several inputs as an output.
 */
} // namespace mopo

#endif // GATE_H
