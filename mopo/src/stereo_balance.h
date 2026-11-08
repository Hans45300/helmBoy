#pragma once
#ifndef STEREO_BALANCE_H
#define STEREO_BALANCE_H

#include "processor.h"

namespace mopo {

  /** Applies a stereo balance control without collapsing the stereo input. */
  class StereoBalance : public Processor {
    public:
      enum class Inputs : int {
        AudioLeft,
        AudioRight,
        Pan,
        NumInputs
      };

      enum class Outputs : int {
        Left,
        Right,
        NumOutputs
      };

      StereoBalance();
      [[nodiscard]] Processor* clone() const override { return new StereoBalance(*this); }
      void process() override;
  };

} // namespace mopo

#endif // STEREO_BALANCE_H