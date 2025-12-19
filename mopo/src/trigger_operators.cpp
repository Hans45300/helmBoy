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

#include "trigger_operators.h"

namespace mopo {

  TriggerCombiner::TriggerCombiner() : Processor(2, 1) { }

  void TriggerCombiner::process() {
    output()->clearTrigger();

    if (input(0)->source->triggered) {
      output()->trigger(input(0)->source->trigger_value,
                        input(0)->source->trigger_offset);
    }
    else if (input(1)->source->triggered) {
      output()->trigger(input(1)->source->trigger_value,
                        input(1)->source->trigger_offset);
    }
  }

  TriggerWait::TriggerWait() : Processor(static_cast<int>(Inputs::NumInputs), 1), waiting_(false), trigger_value_(0.0f) { }

  void TriggerWait::waitTrigger(mopo_float trigger_value) {
    waiting_ = true;
    trigger_value_ = trigger_value;
  }

  void TriggerWait::sendTrigger(int trigger_offset) {
    if (waiting_)
      output()->trigger(trigger_value_, trigger_offset);
    waiting_ = false;
  }

  void TriggerWait::process() {
    using enum Inputs;
    output()->clearTrigger();

    if (input(static_cast<int>(Inputs::Wait))->source->triggered &&
        input(static_cast<int>(Inputs::Trigger))->source->triggered) {

      if (input(static_cast<int>(Inputs::Wait))->source->trigger_offset <=
          input(static_cast<int>(Inputs::Trigger))->source->trigger_offset) {
        waitTrigger(input(static_cast<int>(Inputs::Wait))->source->trigger_value);
        sendTrigger(input(static_cast<int>(Inputs::Trigger))->source->trigger_offset);
      }
      else {
        sendTrigger(input(static_cast<int>(Inputs::Trigger))->source->trigger_offset);
        waitTrigger(input(static_cast<int>(Inputs::Wait))->source->trigger_value);
      }
    }
    else if (input(static_cast<int>(Inputs::Wait))->source->triggered)
      waitTrigger(input(static_cast<int>(Inputs::Wait))->source->trigger_value);
    else if (input(static_cast<int>(Inputs::Trigger))->source->triggered)
      sendTrigger(input(static_cast<int>(Inputs::Trigger))->source->trigger_offset);
  }

  TriggerFilter::TriggerFilter(mopo_float trigger_filter)
    : Processor(static_cast<int>(Inputs::NumInputs), 1), trigger_filter_(trigger_filter) {}

  void TriggerFilter::process() {
    using enum Inputs;
    output()->clearTrigger();

    if (input(static_cast<int>(Inputs::Trigger))->source->triggered) {
      mopo_float trigger_value = input(static_cast<int>(Inputs::Trigger))->source->trigger_value;
      if (trigger_value == trigger_filter_) {
        output()->trigger(trigger_value,
                          input(static_cast<int>(Inputs::Trigger))->source->trigger_offset);
      }
    }
  }

  void TriggerEquals::process() {
    using enum Inputs;
    output()->clearTrigger();

    if (input(static_cast<int>(Inputs::Trigger))->source->triggered && input(static_cast<int>(Inputs::Condition))->at(0) == value_) {
      mopo_float trigger_value = input(static_cast<int>(Inputs::Trigger))->source->trigger_value;
      output()->trigger(trigger_value,
                        input(static_cast<int>(Inputs::Trigger))->source->trigger_offset);
    }
  }

  void TriggerNonZero::process() {
    using enum Inputs;
    output()->clearTrigger();

    if (input(static_cast<int>(Inputs::Trigger))->source->triggered && input(static_cast<int>(Inputs::Condition))->at(0)) {
      mopo_float trigger_value = input(static_cast<int>(Inputs::Trigger))->source->trigger_value;
      output()->trigger(trigger_value,
                        input(static_cast<int>(Inputs::Trigger))->source->trigger_offset);
    }
  }

  LegatoFilter::LegatoFilter() : Processor(static_cast<int>(Inputs::NumInputs), static_cast<int>(Outputs::NumOutputs)),
                                 last_value_(kVoiceOff) { }

  void LegatoFilter::process() {
    using enum Inputs;
    using enum Outputs;
    output(static_cast<int>(Outputs::Retrigger))->clearTrigger();
    output(static_cast<int>(Outputs::Remain))->clearTrigger();
    if (!input(static_cast<int>(Inputs::Trigger))->source->triggered)
      return;

    if (static_cast<int>(input(static_cast<int>(Inputs::Trigger))->source->trigger_value) == static_cast<int>(kVoiceOn) &&
        static_cast<int>(last_value_) == static_cast<int>(kVoiceOn) && input(static_cast<int>(Inputs::Legato))->at(0)) {
      output(static_cast<int>(Outputs::Remain))->trigger(
          input(static_cast<int>(Inputs::Trigger))->source->trigger_value,
          input(static_cast<int>(Inputs::Trigger))->source->trigger_offset);
    }
    else {
      output(static_cast<int>(Outputs::Retrigger))->trigger(
          input(static_cast<int>(Inputs::Trigger))->source->trigger_value,
          input(static_cast<int>(Inputs::Trigger))->source->trigger_offset);
    }
    last_value_ = input(static_cast<int>(Inputs::Trigger))->source->trigger_value;
  }

  PortamentoFilter::PortamentoFilter() : Processor(static_cast<int>(Inputs::NumInputs), 1),
                                         released_(true) { }

  void PortamentoFilter::updateReleased() {
    using enum Inputs;
    if (!input(static_cast<int>(Inputs::VoiceTrigger))->source->triggered)
      return;

    int voice_trigger = input(static_cast<int>(Inputs::VoiceTrigger))->source->trigger_value;
    if (voice_trigger == kVoiceOff)
      released_ = true;
  }

  void PortamentoFilter::updateTrigger() {
    using enum Inputs;
    using enum State;
    output()->clearTrigger();
    if (!input(static_cast<int>(Inputs::FrequencyTrigger))->source->triggered)
      return;

    int state = static_cast<int>(input(static_cast<int>(Inputs::Portamento))->at(0));
    if (state == static_cast<int>(State::PortamentoOff) || (state == static_cast<int>(State::PortamentoAuto) && released_)) {
      output()->trigger(input(static_cast<int>(Inputs::FrequencyTrigger))->source->trigger_value,
                        input(static_cast<int>(Inputs::FrequencyTrigger))->source->trigger_offset);
      released_ = false;
    }
  }

  void PortamentoFilter::process() {
    updateTrigger();
    updateReleased();
  }
} // namespace mopo

























































































