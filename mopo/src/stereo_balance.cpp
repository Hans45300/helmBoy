#include "stereo_balance.h"

#include <algorithm>
#include <cmath>
#include <span>

namespace mopo {

  StereoBalance::StereoBalance()
      : Processor(static_cast<int>(Inputs::NumInputs),
                  static_cast<int>(Outputs::NumOutputs)) { }

  void StereoBalance::process() {
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::AudioLeft)));
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::AudioRight)));
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Pan)));

    const auto left = std::span<const mopo_float>(
        input(static_cast<int>(Inputs::AudioLeft))->source->buffer, buffer_size_);
    const auto right = std::span<const mopo_float>(
        input(static_cast<int>(Inputs::AudioRight))->source->buffer, buffer_size_);
    const auto pan = std::span<const mopo_float>(
        input(static_cast<int>(Inputs::Pan))->source->buffer, buffer_size_);
    auto out_left = std::span<mopo_float>(
        output(static_cast<int>(Outputs::Left))->buffer, buffer_size_);
    auto out_right = std::span<mopo_float>(
        output(static_cast<int>(Outputs::Right))->buffer, buffer_size_);

    for (int sample = 0; sample < buffer_size_; ++sample) {
      const mopo_float position = std::isfinite(pan[sample])
          ? std::clamp(pan[sample], static_cast<mopo_float>(-1.0), static_cast<mopo_float>(1.0))
          : 0.0;
      const mopo_float left_gain = position > 0.0 ? 1.0 - position : 1.0;
      const mopo_float right_gain = position < 0.0 ? 1.0 + position : 1.0;
      out_left[sample] = left[sample] * left_gain;
      out_right[sample] = right[sample] * right_gain;
    }
  }

} // namespace mopo