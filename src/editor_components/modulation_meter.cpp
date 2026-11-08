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

#include "modulation_meter.h"

#include "colors.h"
#include "mopo.h"
#include "synth_base.h"
#include "synth_gui_interface.h"
#include "text_look_and_feel.h"
#include "utils.h"
#include "wave_selector.h"

#define ANGLE 2.51327412f
#define SLIDER_MOD_COLOR 0xff69f0ae

ModulationMeter::ModulationMeter(SynthBase* synth,
         const mopo::Output* mono_total,
                                 const mopo::Output* poly_total,
                                 SynthSlider* slider) :
  synth_(synth), mono_total_(mono_total), poly_total_(poly_total),
        destination_(slider), current_value_(0.0), knob_percent_(0.0), mod_percent_(0.0),
        knob_stroke_(0.0f, PathStrokeType::beveled, PathStrokeType::butt),
        full_radius_(0.0), outer_radius_(0.0) {
  setInterceptsMouseClicks(false, false);
  telemetry_indices_[0] = synth_->getVisualTelemetryIndex(mono_total_);
  telemetry_indices_[1] = synth_->getVisualTelemetryIndex(poly_total_);
  telemetry_count_ = telemetry_indices_[1] >= 0 ? 2 : telemetry_indices_[0] >= 0 ? 1 : 0;
  updateValue();
  updateDrawing();
}

ModulationMeter::~ModulationMeter() {
}

Colour ModulationMeter::getMeterColour() const {
  auto meter_colour = Colour(SLIDER_MOD_COLOR);
  if (!destination_->isActive())
    meter_colour = meter_colour.withAlpha(0.4f);

  return meter_colour;
}

void ModulationMeter::paint(Graphics& g) {
  if (destination_->getSliderStyle() == Slider::RotaryHorizontalVerticalDrag) {
    if (&destination_->getLookAndFeel() == TextLookAndFeel::instance())
      drawTextSlider(g);
    else
      drawKnob(g);
  }
  else
    drawSlider(g);
}

void ModulationMeter::resized() {
  static const float stroke_percent = 0.12f;

  full_radius_ = std::min(getWidth() / 2.0f, getHeight() / 2.0f);
  float stroke_width = 2.0f * full_radius_ * stroke_percent;
  knob_stroke_ = PathStrokeType(stroke_width, PathStrokeType::beveled, PathStrokeType::butt);
  outer_radius_ = full_radius_ - stroke_width;

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent) {
    std::vector<mopo::ModulationConnection*> connections;
    connections = parent->getSynth()->getSourceConnections(getName().toStdString());
    setModulated(connections.size());
  }
}

void ModulationMeter::updateValue() {
  std::array<float, 2> values {};
  if (synth_ == nullptr || telemetry_count_ == 0 ||
      !synth_->copyVisualTelemetry(
          std::span<const int>(telemetry_indices_.data(), telemetry_count_),
          std::span<float>(values.data(), telemetry_count_)))
    return;
  current_value_ = values[0] + (telemetry_count_ > 1 ? values[1] : 0.0f);
}

bool ModulationMeter::copyModulatedValue(float& value) const {
  std::array<float, 2> values {};
  if (!modulated_ || synth_ == nullptr || telemetry_count_ == 0 ||
      !synth_->copyVisualTelemetry(
          std::span<const int>(telemetry_indices_.data(), telemetry_count_),
          std::span<float>(values.data(), telemetry_count_)))
    return false;
  value = values[0] + (telemetry_count_ > 1 ? values[1] : 0.0f);
  return std::isfinite(value);
}

void ModulationMeter::updateDrawing() {
  const double minimum = destination_->getMinimum();
  const double maximum = destination_->getMaximum();
  const double new_mod_percent = destination_->valueToProportionOfLength(
      jlimit(minimum, maximum, current_value_));
  const double new_knob_percent = destination_->valueToProportionOfLength(
      destination_->getValue());

  if (new_mod_percent != mod_percent_ || new_knob_percent != knob_percent_) {
    mod_percent_ = new_mod_percent;
    knob_percent_ = new_knob_percent;
    repaint();
  }
}

void ModulationMeter::drawTextSlider(Graphics& g) {
  g.setColour(getMeterColour());

  const float diff_percent = mod_percent_ - knob_percent_;
  const float content_x = 5.0f;
  const float content_width = jmax(0.0f, getWidth() - 2.0f * content_x);
  if (content_width <= 0.0f)
    return;

  if (diff_percent < 0.0f) {
    const float mod_position = -getHeight() * diff_percent;
    g.fillRect(content_x, 0.0f, content_width, mod_position);
  }
  else if (diff_percent > 0.0f) {
    const float mod_position = getHeight() * (1.0f - diff_percent);
    g.fillRect(content_x, mod_position, content_width, getHeight() - mod_position);
  }
}

void ModulationMeter::drawSlider(Graphics& g) {
  g.setColour(getMeterColour());

  if (destination_->getSliderStyle() == Slider::LinearBar) {
    if (const auto* selector = dynamic_cast<const WaveSelector*>(destination_)) {
      float effective_value = static_cast<float>(destination_->getValue());
      if (modulated_ && !copyModulatedValue(effective_value))
        return;

      const int waveform_index = selector->getWaveformIndex(effective_value);
      const auto selector_bounds = selector->getCellBounds(waveform_index);
      const auto top_left = getLocalPoint(selector, selector_bounds.getTopLeft());
      const auto bottom_right = getLocalPoint(selector, selector_bounds.getBottomRight());
      const Rectangle<float> meter_bounds(
          top_left.x, top_left.y,
          bottom_right.x - top_left.x, bottom_right.y - top_left.y);
      g.setColour(Colour(Colors::Color_aaffffff));
      g.drawRect(meter_bounds, 1.0f);
      return;
    }

    float knob_position = getWidth() * knob_percent_;
    float mod_position = getWidth() * mod_percent_;

    if (destination_->getInterval() == 1.0 && destination_->getMinimum() == 0.0) {
      int index = mod_percent_ * destination_->getMaximum() + 0.5;
      float width = getWidth() / (destination_->getMaximum() + 1.0);

      g.setColour(Colour(Colors::Color_aaffffff));
      g.drawRect(index * width, 0.0f, width, float(getHeight()), 1.0f);
    }
    else
      fillHorizontalRect(g, knob_position, mod_position, getHeight() / 2.0f);
  }
  else {
    float mod_position = getHeight() * (1.0f - mod_percent_);
    float knob_position = getHeight() * (1.0f - knob_percent_);

    fillVerticalRect(g, mod_position, knob_position, getWidth() / 2.0f);
  }
}

void ModulationMeter::drawKnob(Graphics& g) {
  float current_mod_angle = mopo::utils::interpolate(-ANGLE, ANGLE, mod_percent_);
  float current_knob_angle = mopo::utils::interpolate(-ANGLE, ANGLE, knob_percent_);

  if (current_mod_angle != current_knob_angle) {
    Path mod_section;
    mod_section.addCentredArc(full_radius_, full_radius_, outer_radius_, outer_radius_,
                              0.0f, current_mod_angle, current_knob_angle, true);
    g.setColour(getMeterColour());
    g.strokePath(mod_section, knob_stroke_);
  }
}

void ModulationMeter::fillHorizontalRect(Graphics& g, float x1, float x2, float height) {
  float x = std::min(x1, x2);
  float width = fabsf(x1 - x2);
  float padding = (getHeight() - height) / 2.0f;
  g.fillRect(x, padding, width, height);
}

void ModulationMeter::fillVerticalRect(Graphics& g, float y1, float y2, float width) {
  float y = std::min(y1, y2);
  float height = fabsf(y1 - y2);
  float padding = (getWidth() - width) / 2.0f;
  g.fillRect(padding, y, width, height);
}
