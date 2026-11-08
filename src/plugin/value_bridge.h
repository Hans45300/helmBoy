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
 * helmBoy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with helmBoy.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VALUE_BRIDGE_H
#define VALUE_BRIDGE_H

#include <JuceHeader.h>
#include <atomic>
#include <iomanip>
#include <locale>
#include <span>
#include <sstream>
#include <string_view>
#include "mopo.h"
#include "helmBoy_common.h"

class ValueBridge : public AudioProcessorParameterWithID {
  public:
    class Listener {
      public:
        virtual ~Listener() { }
        /** Synchronous callback; name is owned by the bridge and must not outlive it. */
        virtual void parameterChanged(const std::string& name, mopo::mopo_float value) = 0;
    };

    ValueBridge(std::string name, mopo::Value* value,
                const std::atomic<mopo::mopo_float>* snapshot = nullptr) :
        AudioProcessorParameterWithID(
            ParameterID(String("helmboy_") + String(name), 0), String(name)),
        name_(name), value_(value), snapshot_(snapshot), listener_(nullptr) {
      details_ = mopo::Parameters::getDetails(name);
      span_ = details_.max - details_.min;
      text_lookup_ = getTextLookup(details_);
    }

    float getValue() const override {
      return convertToPluginValue(snapshot_ != nullptr
          ? snapshot_->load(std::memory_order_acquire) : value_->value());
    }

    void setValue(float value) override {
      if (listener_ && !isNotifyingOnThisThread()) {
        NotificationScope scope(*this);
        mopo::mopo_float synth_value = convertToSynthValue(value);
        listener_->parameterChanged(details_.name, synth_value);
      }
    }

    void setListener(Listener* listener) {
      listener_ = listener;
    }

    float getDefaultValue() const override {
      return convertToPluginValue(details_.default_value);
    }

    String getName(int maximumStringLength) const override {
      return name_.substring(0, maximumStringLength);
    }

    String getLabel() const override {
      return String(details_.display_units);
    }

    /** Format a supplied normalized value without reading or changing DSP state.
        Positive length limits truncate the result; non-positive limits are unlimited.
        Units are returned separately by getLabel(). */
    String getText(float value, int maximumStringLength) const override {
      const float normalized = std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f)
                                                   : getDefaultValue();
      const double internal = convertToSynthValue(normalized);
      String text;
      const int index = static_cast<int>(std::round(internal));
      if (!text_lookup_.empty() && index >= 0 &&
          static_cast<std::size_t>(index) < text_lookup_.size()) {
        text = String(text_lookup_[static_cast<std::size_t>(index)]);
      }
      else {
        double display = 0.0;
        if (!mopo::tryInternalToDisplay(details_, internal, display))
          return {};
        std::ostringstream stream;
        stream.imbue(std::locale::classic());
        stream << std::setprecision(std::numeric_limits<float>::max_digits10) << display;
        text = String(stream.str());
      }
      return maximumStringLength > 0 ? text.substring(0, maximumStringLength) : text;
    }

    /** Parse a label or display value, with optional matching units and decimal comma.
        Invalid/ambiguous text or an invalid inverse returns the normalized default.
        Finite values are clamped and quantized using the existing parameter mapping. */
    float getValueForText(const String &text) const override {
      String number = text.trim();
      for (std::size_t index = 0; index < text_lookup_.size(); ++index) {
        if (static_cast<double>(index) >= details_.min &&
            static_cast<double>(index) <= details_.max &&
            number.equalsIgnoreCase(String(text_lookup_[index])))
          return convertToPluginValue(static_cast<mopo::mopo_float>(index));
      }

      const String units(details_.display_units);
      if (units.isNotEmpty() && number.endsWithIgnoreCase(units))
        number = number.dropLastCharacters(units.length()).trim();
      if (number.containsChar(',')) {
        if (number.containsChar('.') || number.indexOfChar(',') != number.lastIndexOfChar(','))
          return getDefaultValue();
        number = number.replaceCharacter(',', '.');
      }

      std::istringstream stream(number.toStdString());
      stream.imbue(std::locale::classic());
      double display = 0.0;
      stream >> std::noskipws >> display;
      double internal = 0.0;
      if (stream.fail() || !stream.eof() ||
          !mopo::tryDisplayToInternal(details_, display, internal))
        return getDefaultValue();
      internal = std::clamp(internal, static_cast<double>(details_.min),
                            static_cast<double>(details_.max));
      const float normalized = std::clamp(convertToPluginValue(internal), 0.0f, 1.0f);
      return std::clamp(convertToPluginValue(convertToSynthValue(normalized)), 0.0f, 1.0f);
    }

    bool isAutomatable() const override {
      return true;
    }

    // Converts internal value to value from 0.0 to 1.0.
    float convertToPluginValue(mopo::mopo_float synth_value) const {
      return (synth_value - details_.min) / span_;
    }

    // Converts from value from 0.0 to 1.0 to internal synth value.
    float convertToSynthValue(mopo::mopo_float plugin_value) const {
      mopo::mopo_float percent = plugin_value;
      if (details_.steps) {
        mopo::mopo_float max_step = details_.steps - 1;
        percent = floor(percent * max_step + 0.5) / max_step;
      }

      return percent * span_ + details_.min;
    }

    void setValueNotifyHost(float new_value) {
      if (!isNotifyingOnThisThread()) {
        NotificationScope scope(*this);
        setValueNotifyingHost(new_value);
      }
    }

  private:
    struct NotificationScope {
      explicit NotificationScope(const ValueBridge& parameter)
          : bridge(parameter), previous(active_notification_) {
        active_notification_ = this;
      }
      ~NotificationScope() { active_notification_ = previous; }
      const ValueBridge& bridge;
      const NotificationScope* previous;
    };

    bool isNotifyingOnThisThread() const {
      for (auto* scope = active_notification_; scope != nullptr; scope = scope->previous)
        if (&scope->bridge == this)
          return true;
      return false;
    }

    inline static thread_local const NotificationScope* active_notification_ = nullptr;

    static std::span<const std::string> getTextLookup(const mopo::ValueDetails& details) {
      const std::string_view name = details.name;
      if (details.steps <= 0)
        return {};
      if (name.ends_with("_waveform"))
        return name.find("_lfo_") != std::string_view::npos
            ? std::span<const std::string>(mopo::strings::lfo_waveforms)
            : std::span<const std::string>(mopo::strings::oscillator_waveforms);
      if (name.ends_with("_tempo"))
        return name.find("_lfo_") != std::string_view::npos
            ? std::span<const std::string>(mopo::strings::lfo_synced_frequencies)
            : std::span<const std::string>(mopo::strings::synced_frequencies);
      if (name.ends_with("_sync") && details.steps == 4)
        return mopo::strings::freq_sync_styles;
      if (name.ends_with("_retrigger"))
        return mopo::strings::freq_retrigger_styles;
      if (name == "arp_pattern")
        return mopo::strings::arp_patterns;
      if (name == "distortion_type")
        return mopo::strings::distortion_types_short;
      if (name == "portamento_type")
        return mopo::strings::off_auto_on;
      if (name == "filter_style")
        return mopo::strings::filter_style;
      if (name == "filter_shelf")
        return mopo::strings::filter_shelves;
      if (name == "filter_type")
        return mopo::strings::filter_types;
      if (details.steps == 2 && details.min == 0.0 && details.max == 1.0)
        return mopo::strings::off_on;
      return {};
    }

    String name_;
    mopo::ValueDetails details_;
    std::span<const std::string> text_lookup_;
    mopo::mopo_float span_;
    mopo::Value* value_;
    const std::atomic<mopo::mopo_float>* snapshot_;
    Listener* listener_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ValueBridge)
};

#endif // VALUE_BRIDGE_H
