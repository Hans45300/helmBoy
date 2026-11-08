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

#include "graphical_envelope.h"

#include "colors.h"
#include "envelope.h"
#include "full_interface.h"
#include "synth_base.h"
#include "synth_gui_interface.h"

namespace {
  const float ATTACK_RANGE_PERCENT = 0.33f;
  const float DECAY_RANGE_PERCENT = 0.33f;
  const float DELAY_RANGE_PERCENT = 0.12f;
  const float HOLD_RANGE_PERCENT = 0.12f;
  const float HOVER_DISTANCE = 20.0f;
  const int GRID_CELL_WIDTH = 8;
  const float MARKER_WIDTH = 1.0f;
} // namespace


GraphicalEnvelope::GraphicalEnvelope() {
  delay_hover_ = false;
  attack_hover_ = false;
  hold_hover_ = false;
  decay_hover_ = false;
  sustain_hover_ = false;
  release_hover_ = false;
  mouse_down_ = false;

  delay_slider_ = nullptr;
  attack_slider_ = nullptr;
  hold_slider_ = nullptr;
  decay_slider_ = nullptr;
  sustain_slider_ = nullptr;
  release_slider_ = nullptr;
  startTimerHz(30);
}

GraphicalEnvelope::~GraphicalEnvelope() {
  stopTimer();
  for (auto* slider : { delay_slider_, attack_slider_, hold_slider_, decay_slider_,
                        sustain_slider_, release_slider_ }) {
    if (slider != nullptr)
      slider->removeSliderListener(this);
  }
}

void GraphicalEnvelope::paint(Graphics& g) {
  const float ratio = getHeight() / 100.0f;
  const float line_width = 1.0f * getHeight() / 150.0f;
  const PathStrokeType stroke(line_width, PathStrokeType::beveled, PathStrokeType::rounded);
  static const DropShadow shadow(Colour(Colors::Color_bb000000), 5, Point<int>(0, 0));

  g.fillAll(Colour(Colors::Color_ff424242));

  g.setColour(Colour(Colors::Color_ff4a4a4a));
  for (int x = 0; x < getWidth(); x += GRID_CELL_WIDTH)
    g.drawLine(x, 0, x, getHeight());
  for (int y = 0; y < getHeight(); y += GRID_CELL_WIDTH)
    g.drawLine(0, y, getWidth(), y);

  shadow.drawForPath(g, envelope_line_);
  g.setColour(Colors::graph_fill);
  g.fillPath(envelope_line_);

  if (effective_shape_differs_) {
    g.setColour(Colors::modulation.withAlpha(0.5f));
    g.fillPath(effective_envelope_line_);
  }

  g.setColour(Colour(Colors::Color_ff505050));
  if (delay_slider_ != nullptr)
    g.drawLine(getDelayX(), 0.0f, getDelayX(), getHeight());
  g.drawLine(getAttackX(), 0.0f, getAttackX(), getHeight());
  if (hold_slider_ != nullptr)
    g.drawLine(getHoldX(), 0.0f, getHoldX(), getHeight());
  g.drawLine(getDecayX(), getSustainY(), getDecayX(), getHeight());

  g.setColour(Colors::modulation);
  g.strokePath(envelope_line_, stroke);
  if (effective_shape_differs_)
    g.strokePath(effective_envelope_line_, stroke);

  const float phase = telemetry_values_[0];
  const float amplitude = telemetry_values_[1];
    const auto geometry = geometry_snapshot_.load(std::memory_order_acquire);
    if (geometry != nullptr && synth_ != nullptr && telemetry_indices_[0] >= 0 && telemetry_indices_[1] >= 0 &&
      (amplitude > 0.0f || static_cast<int>(phase) ==
          static_cast<int>(mopo::Envelope::State::Delaying))) {
    const Point<float> marker = valuesToPosition(
      *geometry, phase, amplitude, telemetry_values_[2]);
    if (marker.x >= -1.0f && marker.x <= 1.0f) {
      const float x = (marker.x + 1.0f) * getWidth() * 0.5f;
      g.setColour(Colour(Colors::Color_77ffffff));
      g.fillRect(x - MARKER_WIDTH * ratio * 0.5f, 0.0f,
                 MARKER_WIDTH * ratio, static_cast<float>(getHeight()));
    }
  }

  float hover_line_x = -20;
  if (delay_hover_)
    hover_line_x = getDelayX();
  else if (attack_hover_)
    hover_line_x = getAttackX();
  else if (hold_hover_)
    hover_line_x = getHoldX();
  else if (decay_hover_)
    hover_line_x = getDecayX();
  else if (release_hover_)
    hover_line_x = getReleaseX();

  g.setColour(Colour(Colors::Color_bbffffff));
  g.fillRect(hover_line_x - 0.5f, 0.0f, 1.0f, 1.0f * getHeight());

  if (sustain_hover_) {
    if (mouse_down_) {
      g.setColour(Colour(Colors::Color_11ffffff));
      const float grab_radius = 20.0f * ratio;
      g.fillEllipse(getDecayX() - grab_radius, getSustainY() - grab_radius,
                    grab_radius * 2.0f, grab_radius * 2.0f);
    }

    g.setColour(Colour(Colors::Color_bbffffff));
    const float hover_radius = 7.0f * ratio;
    g.drawEllipse(getDecayX() - hover_radius, getSustainY() - hover_radius,
                  hover_radius * 2.0f, hover_radius * 2.0f, 1.0f);
  }
  else if (mouse_down_) {
    g.setColour(Colour(Colors::Color_11ffffff));
    g.fillRect(hover_line_x - 10.0f, 0.0f, 20.0f, 1.0f * getHeight());
  }

}

void GraphicalEnvelope::resized() {
  resetEnvelopeLine();
}

void GraphicalEnvelope::mouseMove(const MouseEvent& e) {
  float x = e.getPosition().x;
  const float y = e.getPosition().y;
  const float delay_delta = std::abs(x - getDelayX());
  const float attack_delta = std::abs(x - getAttackX());
  const float hold_delta = std::abs(x - getHoldX());
  const float decay_delta = std::abs(x - getDecayX());
  const float release_delta = std::abs(x - getReleaseX());
  const float sustain_delta = std::abs(y - getSustainY());

  const bool delay_is_hover = delay_slider_ != nullptr && delay_delta < HOVER_DISTANCE &&
                              y > getHeight() - HOVER_DISTANCE;
  const bool hold_is_hover = hold_slider_ != nullptr && hold_delta < HOVER_DISTANCE &&
                             y < HOVER_DISTANCE;
  const bool a_hover = !delay_is_hover && !hold_is_hover &&
                       attack_delta < decay_delta && attack_delta < HOVER_DISTANCE;
  const bool d_hover = !delay_is_hover && !hold_is_hover && !a_hover &&
                       decay_delta < release_delta && decay_delta < HOVER_DISTANCE;
  const bool r_hover = !delay_is_hover && !hold_is_hover && !a_hover && !d_hover &&
                       release_delta < HOVER_DISTANCE;
  const bool s_hover = !delay_is_hover && !hold_is_hover && !a_hover && !r_hover &&
                       x > getDecayX() - HOVER_DISTANCE &&
                       x < getDecayX() + HOVER_DISTANCE && sustain_delta < HOVER_DISTANCE;

  if (delay_is_hover != delay_hover_ || a_hover != attack_hover_ ||
      hold_is_hover != hold_hover_ || d_hover != decay_hover_ ||
      s_hover != sustain_hover_ || r_hover != release_hover_) {
    delay_hover_ = delay_is_hover;
    attack_hover_ = a_hover;
    hold_hover_ = hold_is_hover;
    decay_hover_ = d_hover;
    sustain_hover_ = s_hover;
    release_hover_ = r_hover;
    repaint();
  }
}

void GraphicalEnvelope::mouseExit(const MouseEvent& e) {
  delay_hover_ = false;
  attack_hover_ = false;
  hold_hover_ = false;
  decay_hover_ = false;
  sustain_hover_ = false;
  release_hover_ = false;
  repaint();
}

void GraphicalEnvelope::mouseDown(const MouseEvent& e) {
  const std::array<SynthSlider*, 6> sliders {
    delay_slider_, attack_slider_, hold_slider_, decay_slider_,
    sustain_slider_, release_slider_
  };
  for (auto* slider : sliders)
    if (slider != nullptr)
      slider->beginParameterEditHistoryGesture();

  mouse_down_ = true;
  repaint();
}

void GraphicalEnvelope::mouseDrag(const MouseEvent& e) {
  if (delay_hover_)
    setDelayX(e.getPosition().x);
  else if (attack_hover_)
    setAttackX(e.getPosition().x);
  else if (hold_hover_)
    setHoldX(e.getPosition().x);
  else if (decay_hover_)
    setDecayX(e.getPosition().x);
  else if (release_hover_)
    setReleaseX(e.getPosition().x);

  if (sustain_hover_)
    setSustainY(e.getPosition().y);

  if (delay_hover_ || attack_hover_ || hold_hover_ || decay_hover_ ||
      sustain_hover_ || release_hover_) {
    resetEnvelopeLine();
    repaint();
  }
}

void GraphicalEnvelope::mouseUp(const MouseEvent& e) {
  const std::array<SynthSlider*, 6> sliders {
    delay_slider_, attack_slider_, hold_slider_, decay_slider_,
    sustain_slider_, release_slider_
  };
  for (auto* slider : sliders)
    if (slider != nullptr)
      slider->endParameterEditHistoryGesture();

  mouse_down_ = false;
  repaint();
}

void GraphicalEnvelope::guiChanged(SynthSlider* slider) {
  effective_controls_valid_ = false;
  effective_shape_differs_ = false;
  resetEnvelopeLine();
  repaint();
}

void GraphicalEnvelope::timerCallback() {
  if (!isShowing())
    return;

  if (synth_ == nullptr)
    resolveRealtimeTelemetry();
  FullInterface* interface = findParentComponentOfClass<FullInterface>();
  if (interface == nullptr)
    return;

  const std::array<SynthSlider*, 6> sliders {
      delay_slider_, attack_slider_, hold_slider_, decay_slider_,
      sustain_slider_, release_slider_};
  const std::array<const char*, 6> stage_names {
      "delay", "attack", "hold", "decay", "sustain", "release"};
  const std::array<float, 6> base_values = getSliderValues();
  std::array<float, 6> next_values = base_values;
  bool has_modulation = false;

  if (interface->isAnimating()) {
    std::string prefix = getName().toStdString();
    constexpr char suffix[] = "_envelope";
    if (prefix.size() > sizeof(suffix) - 1 &&
        prefix.compare(prefix.size() - (sizeof(suffix) - 1),
                       sizeof(suffix) - 1, suffix) == 0)
      prefix.resize(prefix.size() - (sizeof(suffix) - 1));

    for (size_t index = 0; index < sliders.size(); ++index) {
      SynthSlider* slider = sliders[index];
      if (slider == nullptr)
        continue;
      const std::string destination = prefix + "_" + stage_names[index];
      if (!interface->isDestinationModulated(destination))
        continue;
      has_modulation = true;
      float value = effective_controls_valid_ ? effective_controls_[index] : base_values[index];
      if (interface->copyModulatedValue(destination, value) && std::isfinite(value))
        next_values[index] = jlimit(static_cast<float>(slider->getMinimum()),
                                    static_cast<float>(slider->getMaximum()), value);
      else
        next_values[index] = value;
    }
  }

  bool values_changed = !effective_controls_valid_;
  bool shape_differs = false;
  for (size_t index = 0; index < sliders.size(); ++index) {
    SynthSlider* slider = sliders[index];
    if (slider == nullptr)
      continue;
    const float base = static_cast<float>(
        slider->valueToProportionOfLength(base_values[index]));
    const float effective = static_cast<float>(
        slider->valueToProportionOfLength(next_values[index]));
    const float previous = static_cast<float>(slider->valueToProportionOfLength(
        effective_controls_valid_ ? effective_controls_[index] : base_values[index]));
    values_changed = values_changed || std::abs(effective - previous) >= 0.0005f;
    shape_differs = shape_differs || std::abs(effective - base) >= 0.001f;
  }

  shape_differs = shape_differs && has_modulation && interface->isAnimating();
  if (values_changed || shape_differs != effective_shape_differs_) {
    effective_controls_ = next_values;
    effective_controls_valid_ = true;
    effective_shape_differs_ = shape_differs;
    resetEnvelopeLine();
    repaint();
  }

  if (!interface->isAnimating() || synth_ == nullptr ||
      telemetry_indices_[0] < 0 || telemetry_indices_[1] < 0 ||
      telemetry_indices_[2] < 0)
    return;

  std::array<float, 3> next_telemetry {};
  if (!synth_->copyVisualTelemetry(telemetry_indices_, next_telemetry) ||
      !std::all_of(next_telemetry.begin(), next_telemetry.end(),
                   [](float value) { return std::isfinite(value); }))
    return;

  bool telemetry_changed = false;
  for (size_t index = 0; index < next_telemetry.size(); ++index)
    telemetry_changed = telemetry_changed ||
        std::abs(next_telemetry[index] - telemetry_values_[index]) >= 0.0001f;
  if (telemetry_changed) {
    telemetry_values_ = next_telemetry;
    repaint();
  }
}

void GraphicalEnvelope::showRealtimeFeedback(bool show_feedback) {
  if (!show_feedback) {
    synth_ = nullptr;
    telemetry_indices_.fill(-1);
    telemetry_values_.fill(0.0f);
    repaint();
    return;
  }

  resolveRealtimeTelemetry();
  startTimerHz(30);
}

void GraphicalEnvelope::resolveRealtimeTelemetry() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;
  synth_ = parent->getSynth();
  if (synth_ == nullptr)
    return;

  const std::string source_name = getName().toStdString();
  telemetry_indices_[0] = synth_->getVisualTelemetryIndex(
      synth_->getModSource(source_name + "_phase"));
  telemetry_indices_[1] = synth_->getVisualTelemetryIndex(
      synth_->getModSource(source_name + "_amp"));
  telemetry_indices_[2] = synth_->getVisualTelemetryIndex(
      synth_->getModSource(source_name + "_progress"));
}

float GraphicalEnvelope::getDelayX() {
  if (!delay_slider_)
    return 1.0f;
  const double percent = delay_slider_->valueToProportionOfLength(delay_slider_->getValue());
  return 1.0f + (getWidth() - 1.0f) * percent * DELAY_RANGE_PERCENT;
}

float GraphicalEnvelope::getAttackX() {
  if (!attack_slider_)
    return 0.0;

  const double percent = attack_slider_->valueToProportionOfLength(attack_slider_->getValue());
  return getDelayX() + (getWidth() - 1) * percent * ATTACK_RANGE_PERCENT;
}

float GraphicalEnvelope::getHoldX() {
  if (!hold_slider_)
    return getAttackX();
  const double percent = hold_slider_->valueToProportionOfLength(hold_slider_->getValue());
  return getAttackX() + (getWidth() - 1.0f) * percent * HOLD_RANGE_PERCENT;
}

float GraphicalEnvelope::getDecayX() {
  if (!decay_slider_)
    return 0.0;

  const double percent = decay_slider_->valueToProportionOfLength(decay_slider_->getValue());
  return getHoldX() + getWidth() * percent * DECAY_RANGE_PERCENT;
}

float GraphicalEnvelope::getSustainY() {
  if (!sustain_slider_)
    return 0.0;

  double percent = sustain_slider_->valueToProportionOfLength(sustain_slider_->getValue());
  return getHeight() * (1.0 - percent);
}

float GraphicalEnvelope::getReleaseX() {
  if (!release_slider_)
    return 0.0;

  const double percent = release_slider_->valueToProportionOfLength(release_slider_->getValue());
  return getDecayX() + getWidth() * percent * getReleaseRange();
}

float GraphicalEnvelope::getReleaseRange() {
  const double delay_percent = delay_slider_
      ? delay_slider_->valueToProportionOfLength(delay_slider_->getValue()) : 0.0;
  const double hold_percent = hold_slider_
      ? hold_slider_->valueToProportionOfLength(hold_slider_->getValue()) : 0.0;
  return 1.0f - ATTACK_RANGE_PERCENT - DECAY_RANGE_PERCENT -
         delay_percent * DELAY_RANGE_PERCENT - hold_percent * HOLD_RANGE_PERCENT;
}

void GraphicalEnvelope::setAttackX(double x) {
  if (!attack_slider_)
    return;

  const double percent = (x - getDelayX()) /
                         ((getWidth() - 1.0f) * ATTACK_RANGE_PERCENT);
  attack_slider_->setValue(attack_slider_->proportionOfLengthToValue(percent));
}

void GraphicalEnvelope::setDelayX(double x) {
  if (!delay_slider_ || getWidth() <= 1)
    return;
  const double percent = (x - 1.0) / ((getWidth() - 1.0) * DELAY_RANGE_PERCENT);
  delay_slider_->setValue(delay_slider_->proportionOfLengthToValue(percent));
}

void GraphicalEnvelope::setHoldX(double x) {
  if (!hold_slider_ || getWidth() <= 1)
    return;
  const double percent = (x - getAttackX()) /
                         ((getWidth() - 1.0) * HOLD_RANGE_PERCENT);
  hold_slider_->setValue(hold_slider_->proportionOfLengthToValue(percent));
}

void GraphicalEnvelope::setDecayX(double x) {
  if (!decay_slider_)
    return;

  const double percent = (x - getHoldX()) / (getWidth() * DECAY_RANGE_PERCENT);
  decay_slider_->setValue(decay_slider_->proportionOfLengthToValue(percent));
}

void GraphicalEnvelope::setSustainY(double y) {
  if (!sustain_slider_)
    return;

  sustain_slider_->setValue(sustain_slider_->proportionOfLengthToValue(1.0 - y / getHeight()));
}

void GraphicalEnvelope::setReleaseX(double x) {
  if (!release_slider_)
    return;

  const double percent = (x - getDecayX()) /
                         (getWidth() * getReleaseRange());
  release_slider_->setValue(release_slider_->proportionOfLengthToValue(percent));
}

void GraphicalEnvelope::setDelaySlider(SynthSlider* delay_slider) {
  if (delay_slider_ != nullptr)
    delay_slider_->removeSliderListener(this);
  delay_slider_ = delay_slider;
  if (delay_slider_ != nullptr)
    delay_slider_->addSliderListener(this);
  resetEnvelopeLine();
  repaint();
}

void GraphicalEnvelope::setAttackSlider(SynthSlider* attack_slider) {
  if (attack_slider_ != nullptr)
    attack_slider_->removeSliderListener(this);
  attack_slider_ = attack_slider;
  if (attack_slider_ != nullptr)
    attack_slider_->addSliderListener(this);
  resetEnvelopeLine();
  repaint();
}

void GraphicalEnvelope::setHoldSlider(SynthSlider* hold_slider) {
  if (hold_slider_ != nullptr)
    hold_slider_->removeSliderListener(this);
  hold_slider_ = hold_slider;
  if (hold_slider_ != nullptr)
    hold_slider_->addSliderListener(this);
  resetEnvelopeLine();
  repaint();
}

void GraphicalEnvelope::setDecaySlider(SynthSlider* decay_slider) {
  decay_slider_ = decay_slider;
  decay_slider_->addSliderListener(this);
  resetEnvelopeLine();
  repaint();
}

void GraphicalEnvelope::setSustainSlider(SynthSlider* sustain_slider) {
  sustain_slider_ = sustain_slider;
  sustain_slider_->addSliderListener(this);
  resetEnvelopeLine();
  repaint();
}

void GraphicalEnvelope::setReleaseSlider(SynthSlider* release_slider) {
  release_slider_ = release_slider;
  release_slider_->addSliderListener(this);
  resetEnvelopeLine();
  repaint();
}

void GraphicalEnvelope::resetEnvelopeLine() {
  const std::array<float, 6> base_controls = getSliderValues();
  if (!effective_controls_valid_) {
    effective_controls_ = base_controls;
    effective_controls_valid_ = true;
  }

  envelope_line_ = createEnvelopeLine(base_controls);
  auto geometry = std::make_shared<GeometrySnapshot>();
  effective_envelope_line_ = createEnvelopeLine(effective_controls_, geometry.get());
  geometry->width = static_cast<float>(getWidth());
  geometry->height = static_cast<float>(getHeight());
  geometry->line = effective_shape_differs_ ? effective_envelope_line_ : envelope_line_;
  geometry_snapshot_.store(std::move(geometry), std::memory_order_release);
}

std::array<float, 6> GraphicalEnvelope::getSliderValues() const {
  return {
      delay_slider_ ? static_cast<float>(delay_slider_->getValue()) : 0.0f,
      attack_slider_ ? static_cast<float>(attack_slider_->getValue()) : 0.0f,
      hold_slider_ ? static_cast<float>(hold_slider_->getValue()) : 0.0f,
      decay_slider_ ? static_cast<float>(decay_slider_->getValue()) : 0.0f,
      sustain_slider_ ? static_cast<float>(sustain_slider_->getValue()) : 0.0f,
      release_slider_ ? static_cast<float>(release_slider_->getValue()) : 0.0f};
}

Path GraphicalEnvelope::createEnvelopeLine(const std::array<float, 6>& controls,
                                           GeometrySnapshot* geometry) {
  const std::array<SynthSlider*, 6> sliders {
      delay_slider_, attack_slider_, hold_slider_, decay_slider_,
      sustain_slider_, release_slider_};
  std::array<float, 6> proportions {};
  for (size_t index = 0; index < sliders.size(); ++index) {
    if (sliders[index] != nullptr)
      proportions[index] = static_cast<float>(
          sliders[index]->valueToProportionOfLength(controls[index]));
  }

  const float delay_x = delay_slider_
      ? 1.0f + (getWidth() - 1.0f) * proportions[0] * DELAY_RANGE_PERCENT : 1.0f;
  const float attack_x = delay_x + (getWidth() - 1.0f) *
                                     proportions[1] * ATTACK_RANGE_PERCENT;
  const float hold_x = attack_x + (getWidth() - 1.0f) *
                                  proportions[2] * HOLD_RANGE_PERCENT;
  const float decay_x = hold_x + getWidth() * proportions[3] * DECAY_RANGE_PERCENT;
  const float sustain_y = getHeight() * (1.0f - proportions[4]);
  const float release_range = std::max(0.0f, 1.0f - ATTACK_RANGE_PERCENT -
      DECAY_RANGE_PERCENT - proportions[0] * DELAY_RANGE_PERCENT -
      proportions[2] * HOLD_RANGE_PERCENT);
  const float release_x = decay_x + getWidth() * proportions[5] * release_range;

  if (geometry != nullptr) {
    geometry->delay_x = delay_x;
    geometry->attack_x = attack_x;
    geometry->hold_x = hold_x;
    geometry->decay_x = decay_x;
    geometry->release_x = release_x;
    geometry->sustain = proportions[4];
  }

  Path line;
  line.startNewSubPath(1.0f, static_cast<float>(getHeight()));
  line.lineTo(delay_x, static_cast<float>(getHeight()));
  line.lineTo(attack_x, 0.0f);
  line.lineTo(hold_x, 0.0f);
  line.quadraticTo(0.5f * (hold_x + decay_x), sustain_y, decay_x, sustain_y);
  line.quadraticTo(0.5f * (release_x + decay_x), static_cast<float>(getHeight()),
                   release_x, static_cast<float>(getHeight()));
  return line;
}

Point<float> GraphicalEnvelope::valuesToPosition(const GeometrySnapshot& geometry,
                                                 float phase, float amp,
                                                 float progress) const {
  if (phase < 0.0f || phase > static_cast<float>(mopo::Envelope::State::Holding) ||
      geometry.width <= 0.0f || geometry.height <= 0.0f)
    return Point<float>(-2.0f, -2.0f);

  const int stage = static_cast<int>(phase);
  float x = 0.0f;
  const float delay_x = geometry.delay_x;
  const float attack_x = geometry.attack_x;
  const float hold_x = geometry.hold_x;
  const float decay_x = geometry.decay_x;
  const float release_x = geometry.release_x;

  if (stage == static_cast<int>(mopo::Envelope::State::Delaying))
    x = 1.0f + progress * (delay_x - 1.0f);
  else if (stage == static_cast<int>(mopo::Envelope::State::Attacking))
    x = delay_x + amp * (attack_x - delay_x);
  else if (stage == static_cast<int>(mopo::Envelope::State::Holding))
    x = attack_x + progress * (hold_x - attack_x);
  else if (stage == static_cast<int>(mopo::Envelope::State::Decaying)) {
    const float difference = 1.0f - geometry.sustain;
    x = difference <= 0.0f ? decay_x
        : hold_x + ((1.0f - amp) / difference) * (decay_x - hold_x);
  }
  else {
    const float release_length = release_x - decay_x;
    x = geometry.sustain <= 0.0f ? decay_x
        : decay_x + release_length - release_length * (amp / geometry.sustain);
  }

  const float y = (1.0f - amp) * geometry.height;
  Point<float> closest;
  geometry.line.getNearestPoint(Point<float>(x, y), closest);
  if (phase > 1.5f && phase < 2.5f && closest.x < decay_x) {
    closest.x = decay_x;
    closest.y = y;
  }
  if (phase > 0.5f && phase < 1.5f && closest.x > decay_x) {
    closest.x = decay_x;
    closest.y = y;
  }
  return Point<float>(2.0f * closest.x / geometry.width - 1.0f,
                      1.0f - 2.0f * closest.y / geometry.height);
}
