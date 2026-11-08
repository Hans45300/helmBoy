/* Copyright 2025 Marc Scheffer
 *
 * helmBoy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *
 * This work is based on bepzi's Helm project, <https://github.com/bepzi/helm>,
 * itself based on Matt Tytel's Helm <https://tytel.org/helm/>
 *
 * helmBoy is distributedin the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with helmBoy.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file helmBoy_common.h
 * @brief Core types, constants and small utility classes used across helmBoy.
 */

#pragma once
#ifndef HELM_COMMON_H
#define HELM_COMMON_H

#include "mopo.h"
#include "value.h"
#include "operators.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <string>
#include <memory>

namespace mopo {

  struct ModulationConnection;


  enum class DisplaySkew {
    kLinear,
    kQuadratic,
    kSquareRoot,
    kExponential
  };

  struct ValueDetails {

    std::string name;
    mopo_float min;
    mopo_float max;
    int steps;
    mopo_float default_value;

    // post_offset used to offset quadratic and exponential scaling.
    mopo_float post_offset;

    mopo_float display_multiply;
  DisplaySkew display_skew;
    bool display_invert;
    std::string display_units;
    std::string display_name;
  bool deprecated = false;
  } typedef ValueDetails;

  /** Convert presentation values using metadata only; never access DSP state. */
  inline bool tryDisplayToInternal(const ValueDetails& details, double display_value,
                                   double& internal_value) {
    if (!std::isfinite(display_value) ||
        std::abs(details.display_multiply) < std::numeric_limits<double>::epsilon())
      return false;

    double value = display_value / details.display_multiply;
    if (details.display_invert) {
      if (std::abs(value) < std::numeric_limits<double>::epsilon())
        return false;
      value = 1.0 / value;
    }
    value -= details.post_offset;
    switch (details.display_skew) {
      case DisplaySkew::kQuadratic:
        if (value < 0.0)
          return false;
        value = std::sqrt(value);
        break;
      case DisplaySkew::kExponential:
        if (value <= 0.0)
          return false;
        value = std::log2(value);
        break;
      case DisplaySkew::kSquareRoot:
        if (value < 0.0)
          return false;
        value = value * value;
        break;
      default:
        break;
    }
    if (!std::isfinite(value))
      return false;
    internal_value = value;
    return true;
  }

  /** Apply the established display curve, offset, inversion and unit scale. */
  inline bool tryInternalToDisplay(const ValueDetails& details, double internal_value,
                                   double& display_value) {
    if (!std::isfinite(internal_value))
      return false;

    double value = internal_value;
    switch (details.display_skew) {
      case DisplaySkew::kQuadratic:
        value = std::pow(value, 2.0);
        break;
      case DisplaySkew::kExponential:
        value = std::pow(2.0, value);
        break;
      case DisplaySkew::kSquareRoot:
        if (value < 0.0)
          return false;
        value = std::sqrt(value);
        break;
      default:
        break;
    }
    value += details.post_offset;
    if (details.display_invert) {
      if (std::abs(value) < std::numeric_limits<double>::epsilon())
        return false;
      value = 1.0 / value;
    }
    value *= details.display_multiply;
    if (!std::isfinite(value))
      return false;
    display_value = value;
    return true;
  }

  namespace strings {

    const std::string off_on[] = {
      "off",
      "on"
    };

    const std::string off_auto_on[] = {
      "off",
      "auto",
      "on"
    };

    const std::string off_auto_on_slider[] = {
      "OFF",
      "AUT",
      "ON"
    };

    const std::string filter_style[] = {
      "12dB",
      "24dB",
      "Shelf",
      "Notch",
      "Comb"
    };

    const std::string filter_style_short[] = {
      "12",
      "24",
      "SH",
      "N",
      "C"
    };

    const std::string arp_patterns[] = {
      "up",
      "down",
      "up-down",
      "as played",
      "random"
    };

    const std::string freq_sync_styles[] = {
      "Seconds",
      "Tempo",
      "Tempo Dotted",
      "Tempo Triplets"
    };


    const std::string freq_retrigger_styles[] = {
      "Free",
      "Retrigger",
      "Sync to Playhead"
    };

    const std::string distortion_types_short[] = {
      "sft clp",
      "hrd clp",
      "lin fld",
      "sin fld"
    };

    const std::string distortion_types_long[] = {
      "Soft Clip",
      "Hard Clip",
      "Linear Fold",
      "Sine Fold"
    };

    const std::string filter_shelves[] = {
      "low shelf",
      "band shelf",
      "high shelf"
    };

    const std::string filter_types[] = {
      "low pass",
      "high pass",
      "band pass",
      "low shelf",
      "high shelf",
      "band shelf",
      "all pass"
    };

    const std::string oscillator_waveforms[] = {
      "sin",
      "triangle",
      "square",
      "saw down",
      "saw up",
      "3 step",
      "4 step",
      "8 step",
      "3 pyramid",
      "5 pyramid",
      "9 pyramid",
      "saw mobius",
    };

    const std::string lfo_waveforms[] = {
      "sin",
      "triangle",
      "square",
      "saw down",
      "saw up",
      "3 step",
      "4 step",
      "8 step",
      "3 pyramid",
      "5 pyramid",
      "9 pyramid",
      "sample and hold",
      "sample and glide",
      "smooth random",
    };

    const std::string synced_frequencies[] = {
      "32/1",
      "16/1",
      "8/1",
      "4/1",
      "2/1",
      "1/1",
      "1/2",
      "1/4",
      "1/8",
      "1/16",
      "1/32",
      "1/64",
    };

    const std::string lfo_synced_frequencies[] = {
      "64/1",
      "32/1",
      "16/1",
      "8/1",
      "4/1",
      "2/1",
      "1/1",
      "1/2",
      "1/4",
      "1/8",
      "1/16",
      "1/32",
      "1/64",
      "1/128",
    };
  } // namespace strings

  const constexpr mopo_float MAX_STEPS = 128;
  const constexpr int NUM_FORMANTS = 4;
  const constexpr int NUM_CHANNELS = 2;
  const constexpr int MEMORY_SAMPLE_RATE = 22000;
  const constexpr int MEMORY_RESOLUTION = 512;
  const constexpr mopo_float STUTTER_MAX_SAMPLES = 96000.0;
  const constexpr int DEFAULT_MODULATION_CONNECTIONS = 256;
  const constexpr int DEFAULT_WINDOW_WIDTH = 1320;
  const constexpr int DEFAULT_WINDOW_HEIGHT = 850;

  const std::string PATCH_EXTENSION = "helmBoy";

  typedef std::map<std::string, Value*> control_map;
  typedef std::pair<Value*, mopo_float> control_change;
  typedef std::pair<ModulationConnection*, mopo_float> modulation_change;
  typedef std::map<std::string, Processor*> input_map;
  typedef std::map<std::string, Output*> output_map;

  const mopo::cr::Value synced_freq_ratios[] = {
    cr::Value(1.0 / 128.0),
    cr::Value(1.0 / 64.0),
    cr::Value(1.0 / 32.0),
    cr::Value(1.0 / 16.0),
    cr::Value(1.0 / 8.0),
    cr::Value(1.0 / 4.0),
    cr::Value(1.0 / 2.0),
    cr::Value(1.0),
    cr::Value(2.0),
    cr::Value(4.0),
    cr::Value(8.0),
    cr::Value(16.0),
  };

  const mopo::cr::Value lfo_synced_freq_ratios[] = {
    cr::Value(1.0 / 256.0),
    cr::Value(1.0 / 128.0),
    cr::Value(1.0 / 64.0),
    cr::Value(1.0 / 32.0),
    cr::Value(1.0 / 16.0),
    cr::Value(1.0 / 8.0),
    cr::Value(1.0 / 4.0),
    cr::Value(1.0 / 2.0),
    cr::Value(1.0),
    cr::Value(2.0),
    cr::Value(4.0),
    cr::Value(8.0),
    cr::Value(16.0),
    cr::Value(32.0),
  };

  /** @brief Remap an old LFO division index into the extended tempo table. */
  [[nodiscard]] inline mopo_float migrateLegacyLfoTempoIndex(mopo_float index) {
    return std::clamp(index, mopo_float{0.0}, mopo_float{11.0}) + 1.0;
  }

  struct ModulationConnection {
    /** @brief Default construct an empty modulation connection. */
    ModulationConnection() : ModulationConnection("", "") { }

    /** @brief Construct connection between two parameter names. */
    ModulationConnection(std::string from, std::string to) :
        source(from), destination(to) {
    }

    /** @brief Destroy the modulation connection and free resources. */
    ~ModulationConnection() {
      amount.destroy();
      modulation_scale.destroy();
    }

    /**
     * @brief Reset this connection to point from->to.
     * @param from Source parameter name.
     * @param to Destination parameter name.
     */
    void resetConnection(const std::string& from, const std::string& to) {
      source = from;
      destination = to;
      modulation_scale.router(nullptr);
    }

    std::string source;
    std::string destination;
    cr::Value amount;
    cr::Multiply modulation_scale;
  };

  class ModulationConnectionBank {
    public:
      /**
       * @brief Pool allocator for temporary modulation connections.
       *
       * Recycles previously allocated ModulationConnection instances to
       * avoid frequent heap allocations during modulation routing changes.
       */
      ModulationConnectionBank();
      ~ModulationConnectionBank();
      ModulationConnection* get(const std::string& from, const std::string& to);
      void recycle(ModulationConnection* connection);

    private:
      void allocateMoreConnections();
      std::list<ModulationConnection*> available_connections_;
      std::vector<ModulationConnection*> all_connections_;
  };

  class ValueDetailsLookup {
    public:
      ValueDetailsLookup();
      const bool isParameter(const std::string& name) const {
        return details_lookup_.count(name);
      }

      const ValueDetails& getDetails(const std::string& name) const {
        auto details = details_lookup_.find(name);
        MOPO_ASSERT(details != details_lookup_.end());
        return details->second;
      }

      std::map<std::string, ValueDetails> getAllDetails() const {
        return details_lookup_;
      }

      static const ValueDetails parameter_list[];

    private:
      std::map<std::string, ValueDetails> details_lookup_;
  };

  class Parameters {
    public:
      static const ValueDetails& getDetails(const std::string& name) {
        return lookup_.getDetails(name);
      }

      static const bool isParameter(const std::string& name) {
        return lookup_.isParameter(name);
      }

      static ValueDetailsLookup lookup_;
  };

  /** @brief Return the symmetric modulation-depth limit for a destination range. */
  [[nodiscard]] inline mopo_float modulationAmountLimit(mopo_float minimum,
                                                        mopo_float maximum) {
    return maximum - minimum;
  }

  /** @brief Return the symmetric modulation-depth limit for a destination. */
  [[nodiscard]] inline mopo_float modulationAmountLimit(const ValueDetails& destination) {
    return modulationAmountLimit(destination.min, destination.max);
  }

  /** @brief Clamp a finite modulation amount to the destination's symmetric range. */
  [[nodiscard]] inline mopo_float clampModulationAmount(const ValueDetails& destination,
                                                        mopo_float amount) {
    const mopo_float limit = modulationAmountLimit(destination);
    return std::clamp(amount, -limit, limit);
  }
} // namespace mopo

#endif // HELM_COMMON_H
