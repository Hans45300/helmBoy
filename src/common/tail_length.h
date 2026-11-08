#pragma once

#include "common.h"
#include "sample_decay_lookup.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

namespace helmboy {
  /** Configuration-based tail estimate to -100 dB relative amplitude, not silence
      detection. Serial stages are added; unknown/non-decaying cases are infinite.
      JUCE Reverb bounds correspond to the currently bundled Freeverb implementation. */
  class TailLength {
    public:
      enum Control : std::size_t {
        Release, ChorusOn, ChorusMix, DelayOn, DelayMix, DelayFeedback,
        ReverbOn, ReverbMix, ReverbFeedback, ReverbFreeze, Count
      };
      inline static constexpr std::array<std::string_view, Count> names {
        "amp_release", "chorus_on", "chorus_mix", "delay_on", "delay_dry_wet",
        "delay_feedback", "reverb_on", "reverb_dry_wet", "reverb_feedback",
        "reverb_freeze_mode"
      };
      struct Inputs {
        std::array<double, Count> values {};
        std::uint32_t modulated = 0;
        double sample_rate = 44100.0;
      };

      static constexpr double kThreshold = 0.00001;
      static constexpr double infinity() { return std::numeric_limits<double>::infinity(); }

      static std::uint32_t modulationBit(std::string_view destination) {
        for (std::size_t index = 0; index < names.size(); ++index)
          if (names[index] == destination)
            return std::uint32_t { 1 } << index;
        return 0;
      }

      static double estimate(const Inputs& inputs) {
        if (!std::isfinite(inputs.sample_rate) || inputs.sample_rate <= 0.0)
          return infinity();
        for (double value : inputs.values)
          if (!std::isfinite(value))
            return infinity();
        const auto modulated = [&inputs](Control control) {
          return (inputs.modulated & (std::uint32_t { 1 } << control)) != 0;
        };
        const auto audible = [&inputs, &modulated](Control on, Control mix) {
          return (inputs.values[on] > 0.0 || modulated(on)) &&
                 (inputs.values[mix] > 0.0 || modulated(mix));
        };
        if (modulated(Release))
          return infinity();
        const double release = inputs.values[Release] * inputs.values[Release];
        const double multiplier = mopo::SampleDecayLookup::sampleDecayLookup(
            inputs.sample_rate * release);
        double seconds = decaySeconds(1.0 / inputs.sample_rate, multiplier);
        if (audible(ChorusOn, ChorusMix))
          seconds += decaySeconds(0.111, 0.95) + 0.02;
        if (audible(DelayOn, DelayMix)) {
          if (modulated(DelayFeedback))
            return infinity();
            const double filter_settling = -std::log(kThreshold) * 2.0 * 1.306563 /
                           (2.0 * mopo::PI * 20.0) + 0.01;
            seconds += decaySeconds((mopo::MAX_DELAY_SAMPLES - 1.0) / inputs.sample_rate +
                          filter_settling,
                        std::abs(inputs.values[DelayFeedback]));
        }
        if (audible(ReverbOn, ReverbMix)) {
          if (inputs.values[ReverbFreeze] >= 0.5 || modulated(ReverbFreeze))
            return infinity();
          const double feedback = modulated(ReverbFeedback) ? 1.0
              : std::clamp(inputs.values[ReverbFeedback], -1.0, 1.0);
          const double room_size = (feedback + 1.0) * 0.5;
          const double comb_feedback = 0.7 + 0.28 * room_size;
          seconds += decaySeconds(1640.0 / 44100.0 + 1.0 / inputs.sample_rate, comb_feedback);
          seconds += decaySeconds(1655.0 / 44100.0 + 4.0 / inputs.sample_rate, 0.5) + 0.01;
        }
        return seconds;
      }

      /** VST3 recognises infinity. Other wrappers use a documented finite reporting
          ceiling; no raw infinity is passed to unverified sample-count conversions. */
      static double forWrapper(double seconds, double sample_rate, bool supports_infinity) {
        const double rate = std::isfinite(sample_rate) && sample_rate > 0.0 ? sample_rate : 44100.0;
        const double limit = static_cast<double>(0x3fffffffu) / rate;
        if (!std::isfinite(seconds) || seconds > limit)
          return supports_infinity ? infinity() : limit;
        return std::max(0.0, seconds);
      }

      /** Retain finite previous configurations while they decay. Leaving an infinite
          configuration restarts the full current finite decay estimate. */
      static double retain(double previous, double current, double elapsed) {
        if (!std::isfinite(current))
          return infinity();
        if (!std::isfinite(previous))
          return current;
        return std::max(current, std::max(0.0, previous - std::max(0.0, elapsed)));
      }

    private:
      static double decaySeconds(double period, double feedback) {
        if (!std::isfinite(period) || !std::isfinite(feedback) || feedback >= 1.0)
          return infinity();
        if (feedback <= 0.0)
          return period;
        return period * (1.0 + std::ceil(std::log(kThreshold) / std::log(feedback)));
      }
  };
}