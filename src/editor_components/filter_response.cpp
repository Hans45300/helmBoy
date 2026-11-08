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

#include "filter_response.h"

#include "colors.h"
#include "full_interface.h"
#include "midi_lookup.h"
#include "utils.h"

#include <cmath>

#define MIN_GAIN_DB -30.0
#define MAX_GAIN_DB 24.0
#define MIN_RESONANCE 0.5
#define MAX_RESONANCE 16.0
#define GRID_CELL_WIDTH 8
#define DELTA_SLOPE_REDRAW_THRESHOLD 0.01
#define X_REDRAW_THRESHOLD 30

FilterResponse::FilterResponse(int resolution) {
  resolution_ = resolution;
  cutoff_slider_ = nullptr;
  resonance_slider_ = nullptr;
  filter_blend_slider_ = nullptr;
  filter_shelf_slider_ = nullptr;

  filter_low_.setSampleRate(44100);
  filter_band_.setSampleRate(44100);
  filter_high_.setSampleRate(44100);
  filter_shelf_.setSampleRate(44100);
  filter_notch_.setSampleRate(44100);
  style_ = mopo::StateVariableFilter::Styles::NumStyles;
  active_ = false;
  resetResponsePath();

  setOpaque(true);
  setBufferedToImage(true);
  startTimerHz(30);
}

FilterResponse::~FilterResponse() {
  stopTimer();
}

void FilterResponse::paintBackground(Graphics& g) {
  // Fond de la grille d'affichage de la courbe de filtre
  g.fillAll(Colour(Colors::Color_ff424242));
  // Quadrillage de la grille d'affichage de la courbe de filtre
  g.setColour(Colour(Colors::Color_ff4a4a4a));
  for (int x = 0; x < getWidth(); x += GRID_CELL_WIDTH)
    g.drawLine(x, 0, x, getHeight());
  for (int y = 0; y < getHeight(); y += GRID_CELL_WIDTH)
    g.drawLine(0, y, getWidth(), y);
}

void FilterResponse::paint(Graphics& g) {
  static const DropShadow shadow(Colour(Colors::shadow_dark), 5, Point<int>(0, 0));

  g.drawImage(background_,
              0, 0, getWidth(), getHeight(),
              0, 0, background_.getWidth(), background_.getHeight());

  shadow.drawForPath(g, base_filter_response_path_);

  g.setColour(Colors::graph_fill);
  g.fillPath(base_filter_response_path_);

  if (active_)
    g.setColour(Colors::modulation);
  else
    g.setColour(Colors::graph_disable);

  float line_width = 1.0f * getHeight() / 150.0f;
  PathStrokeType stroke(line_width, PathStrokeType::beveled, PathStrokeType::rounded);
  g.strokePath(base_filter_response_path_, stroke);

  if (effective_shape_differs_) {
    g.setColour(Colors::modulation.withAlpha(0.5f));
    g.fillPath(filter_response_path_);
    g.setColour(Colors::modulation);
    g.strokePath(filter_response_path_, stroke);
  }
}

void FilterResponse::resized() {
  auto *display = Desktop::getInstance().getDisplays().getPrimaryDisplay();
  float scale = display != nullptr ? static_cast<float>(display->scale) : 1.0f;
  background_ = Image(Image::RGB, scale * getWidth(), scale * getHeight(), true);
  Graphics g(background_);
  g.addTransform(AffineTransform::scale(scale, scale));
  paintBackground(g);

  computeFilterCoefficients();
  resetResponsePath();
}

void FilterResponse::mouseDown(const MouseEvent& e) {
  if (cutoff_slider_ != nullptr)
    cutoff_slider_->beginParameterEditHistoryGesture();
  if (resonance_slider_ != nullptr)
    resonance_slider_->beginParameterEditHistoryGesture();
  setFilterSettingsFromPosition(e.getPosition());
  repaint();
}

void FilterResponse::mouseDrag(const MouseEvent& e) {
  setFilterSettingsFromPosition(e.getPosition());
  repaint();
}

void FilterResponse::mouseUp(const MouseEvent& e) {
  if (cutoff_slider_ != nullptr)
    cutoff_slider_->endParameterEditHistoryGesture();
  if (resonance_slider_ != nullptr)
    resonance_slider_->endParameterEditHistoryGesture();
}

float FilterResponse::getPercentForMidiNote(float midi_note) {
  float frequency = mopo::utils::midiNoteToFrequency(midi_note);

  float response = 0.0f;
  if (style_ == mopo::StateVariableFilter::Styles::Shelf)
    response = fabs(filter_shelf_.getAmplitudeResponse(frequency));
  else if (style_ == mopo::StateVariableFilter::Styles::Notch)
    response = fabs(filter_notch_.getAmplitudeResponse(frequency));
  else if (style_ == mopo::StateVariableFilter::Styles::Comb) {
    const float phase = 2.0f * static_cast<float>(mopo::PI) * frequency * comb_period_seconds_;
    const float denominator = std::sqrt(1.0f + comb_feedback_ * comb_feedback_ -
                                        2.0f * comb_feedback_ * std::cos(phase));
    response = 1.0f / denominator;
  }
  else {
    float blend = filter_blend_slider_->getValue();
    float low_pass_amount = mopo::utils::clamp(1.0 - blend, 0.0, 1.0);
    float band_pass_amount = mopo::utils::clamp(1.0 - fabs(blend - 1.0), 0.0, 1.0);
    float high_pass_amount = mopo::utils::clamp(blend - 1.0, 0.0, 1.0);

    response = low_pass_amount * filter_low_.getAmplitudeResponse(frequency) +
               band_pass_amount * filter_band_.getAmplitudeResponse(frequency) +
               high_pass_amount * filter_high_.getAmplitudeResponse(frequency);

    response = fabs(response);
  if (style_ == mopo::StateVariableFilter::Styles::dB24)
      response *= response;
  }

  float gain_db = mopo::utils::gainToDb(response);
  return (gain_db - MIN_GAIN_DB) / (MAX_GAIN_DB - MIN_GAIN_DB);
}

void FilterResponse::resetResponsePath() {
  if (cutoff_slider_ == nullptr || resonance_slider_ == nullptr ||
      filter_blend_slider_ == nullptr || filter_shelf_slider_ == nullptr)
    return;

  const double base_cutoff = cutoff_slider_->getValue();
  const double base_resonance = resonance_slider_->getValue();
  const double effective_cutoff = displayed_parameters_valid_
      ? displayed_cutoff_ : base_cutoff;
  const double effective_resonance = displayed_parameters_valid_
      ? displayed_resonance_ : base_resonance;

  setFilterCoefficients(effective_cutoff, effective_resonance);
  filter_response_path_ = createResponsePath();

  const double cutoff_change = std::abs(
      cutoff_slider_->valueToProportionOfLength(effective_cutoff) -
      cutoff_slider_->valueToProportionOfLength(base_cutoff));
  const double resonance_change = std::abs(
      resonance_slider_->valueToProportionOfLength(effective_resonance) -
      resonance_slider_->valueToProportionOfLength(base_resonance));
  effective_shape_differs_ = cutoff_change >= 0.0005 || resonance_change >= 0.0005;

  setFilterCoefficients(base_cutoff, base_resonance);
  base_filter_response_path_ = createResponsePath();
  setFilterCoefficients(effective_cutoff, effective_resonance);
}

Path FilterResponse::createResponsePath() {
  static const int wrap_size = 10;
  Path response_path;
  response_path.startNewSubPath(-wrap_size, getHeight() + wrap_size);
  float start_percent = getPercentForMidiNote(0.0);
  float last_y = getHeight() * (1.0f - start_percent);
  float last_slope = 0.0f;
  float last_x = 0.0f;

  response_path.lineTo(-wrap_size, last_y);

  for (int i = 0; i < resolution_; ++i) {
    float t = (1.0f * i) / (resolution_ - 1);
    float midi_note = cutoff_slider_->proportionOfLengthToValue(t);
    float percent = getPercentForMidiNote(midi_note);

    float new_x = getWidth() * t;
    float new_y = getHeight() * (1.0f - percent);
    float new_slope = (new_y - last_y) / (new_x - last_x);
    if (fabs(last_slope - new_slope) > DELTA_SLOPE_REDRAW_THRESHOLD ||
        new_x - last_x > X_REDRAW_THRESHOLD) {
      last_x = new_x;
      last_y = new_y;
      last_slope = new_slope;
      response_path.lineTo(new_x, new_y);
    }
  }

  float end_percent = getPercentForMidiNote(cutoff_slider_->getMaximum());

  response_path.lineTo(getWidth() + wrap_size, getHeight() * (1.0f - end_percent));
  response_path.lineTo(getWidth() + wrap_size, getHeight() + wrap_size);
  return response_path;
}

void FilterResponse::computeFilterCoefficients() {
  if (cutoff_slider_ == nullptr || resonance_slider_ == nullptr ||
      filter_blend_slider_ == nullptr || filter_shelf_slider_ == nullptr) {
    return;
  }

  computeFilterCoefficients(cutoff_slider_->getValue(), resonance_slider_->getValue());
}

void FilterResponse::computeFilterCoefficients(double cutoff, double resonance_value) {
  if (cutoff_slider_ == nullptr || resonance_slider_ == nullptr ||
      filter_blend_slider_ == nullptr || filter_shelf_slider_ == nullptr)
    return;

  displayed_cutoff_ = cutoff;
  displayed_resonance_ = resonance_value;
  displayed_parameters_valid_ = true;

  resetResponsePath();
}

void FilterResponse::setFilterCoefficients(double cutoff, double resonance_value) {
  if (cutoff_slider_ == nullptr || resonance_slider_ == nullptr ||
      filter_blend_slider_ == nullptr || filter_shelf_slider_ == nullptr)
    return;

  mopo::StateVariableFilter::Shelves shelf = static_cast<mopo::StateVariableFilter::Shelves>(
      static_cast<int>(filter_shelf_slider_->getValue()));
  double frequency = mopo::utils::midiNoteToFrequency(cutoff);
  double resonance = mopo::utils::magnitudeToQ(resonance_value);
  double decibels = mopo::utils::interpolate(MIN_GAIN_DB, MAX_GAIN_DB,
                                             resonance_value);
  double gain = mopo::utils::dbToGain(decibels);

  if (style_ == mopo::StateVariableFilter::Styles::dB24) {
    resonance = sqrt(resonance);
    gain = sqrt(gain);
  }

  const double cutoff_frequency = mopo::utils::midiNoteToFrequency(cutoff);
  comb_period_seconds_ = static_cast<float>(mopo::utils::clamp(1.0 / cutoff_frequency,
                                                               0.00001, 0.1));
  comb_feedback_ = static_cast<float>(mopo::utils::clamp(
      0.95 * resonance_value, 0.0, 0.95));

  if (style_ == mopo::StateVariableFilter::Styles::Shelf) {
  mopo::BiquadFilter::Type type = mopo::BiquadFilter::Type::LowShelf;
    if (shelf == mopo::StateVariableFilter::Shelves::BandShelf)
      type = mopo::BiquadFilter::Type::BandShelf;
    else if (shelf == mopo::StateVariableFilter::Shelves::HighShelf)
      type = mopo::BiquadFilter::Type::HighShelf;
    filter_shelf_.computeCoefficients(type, frequency, 1.0, gain);
  }
  else if (style_ == mopo::StateVariableFilter::Styles::Notch) {
    filter_notch_.computeCoefficients(mopo::BiquadFilter::Type::Notch,
                                      frequency, resonance, 1.0);
  }
  else if (style_ != mopo::StateVariableFilter::Styles::Comb) {
  filter_low_.computeCoefficients(mopo::BiquadFilter::Type::LowPass, frequency, resonance, 1.0);
  filter_band_.computeCoefficients(mopo::BiquadFilter::Type::BandPass, frequency, resonance, 1.0);
  filter_high_.computeCoefficients(mopo::BiquadFilter::Type::HighPass, frequency, resonance, 1.0);
  }
}

void FilterResponse::timerCallback() {
  if (!isShowing() || cutoff_slider_ == nullptr || resonance_slider_ == nullptr)
    return;

  const double base_cutoff = cutoff_slider_->getValue();
  const double base_resonance = resonance_slider_->getValue();
  double effective_cutoff = base_cutoff;
  double effective_resonance = base_resonance;

  FullInterface* interface = findParentComponentOfClass<FullInterface>();
  if (interface != nullptr && interface->isAnimating()) {
    const auto readEffectiveValue = [interface](
        const char* name, SynthSlider* slider, double previous) {
      if (!interface->isDestinationModulated(name))
        return static_cast<double>(slider->getValue());

      float value = static_cast<float>(previous);
      if (!interface->copyModulatedValue(name, value) || !std::isfinite(value))
        return previous;
      return jlimit(slider->getMinimum(), slider->getMaximum(),
                    static_cast<double>(value));
    };

    effective_cutoff = readEffectiveValue(
        "cutoff", cutoff_slider_, displayed_parameters_valid_ ? displayed_cutoff_ : base_cutoff);
    effective_resonance = readEffectiveValue(
        "resonance", resonance_slider_,
        displayed_parameters_valid_ ? displayed_resonance_ : base_resonance);
  }

  const double cutoff_change = std::abs(
      cutoff_slider_->valueToProportionOfLength(effective_cutoff) -
      cutoff_slider_->valueToProportionOfLength(
          displayed_parameters_valid_ ? displayed_cutoff_ : base_cutoff));
  const double resonance_change = std::abs(
      resonance_slider_->valueToProportionOfLength(effective_resonance) -
      resonance_slider_->valueToProportionOfLength(
          displayed_parameters_valid_ ? displayed_resonance_ : base_resonance));
  if (!displayed_parameters_valid_ || cutoff_change >= 0.0005 ||
      resonance_change >= 0.0005) {
    computeFilterCoefficients(effective_cutoff, effective_resonance);
    repaint();
  }
}

void FilterResponse::setFilterSettingsFromPosition(Point<int> position) {
  if (cutoff_slider_) {
    double percent = mopo::utils::clamp((1.0 * position.x) / getWidth(), 0.0, 1.0);
    double frequency = cutoff_slider_->proportionOfLengthToValue(percent);
    cutoff_slider_->setValue(frequency);
  }
  if (resonance_slider_) {
    double percent = mopo::utils::clamp(1.0 - (1.0 * position.y) / getHeight(), 0.0, 1.0);
    resonance_slider_->setValue(resonance_slider_->proportionOfLengthToValue(percent));
  }

  computeFilterCoefficients();
}

void FilterResponse::guiChanged(SynthSlider* slider) {
  computeFilterCoefficients();
  repaint();
}

void FilterResponse::setResonanceSlider(SynthSlider* slider) {
  resonance_slider_ = slider;
  resonance_slider_->addSliderListener(this);
  computeFilterCoefficients();
  repaint();
}

void FilterResponse::setCutoffSlider(SynthSlider* slider) {
  cutoff_slider_ = slider;
  cutoff_slider_->addSliderListener(this);
  computeFilterCoefficients();
  repaint();
}

void FilterResponse::setFilterBlendSlider(SynthSlider* slider) {
  filter_blend_slider_ = slider;
  filter_blend_slider_->addSliderListener(this);
  computeFilterCoefficients();
  repaint();
}

void FilterResponse::setFilterShelfSlider(SynthSlider* slider) {
  filter_shelf_slider_ = slider;
  filter_shelf_slider_->addSliderListener(this);
  computeFilterCoefficients();
  repaint();
}

void FilterResponse::setStyle(mopo::StateVariableFilter::Styles style) {
  if (style_ == style)
    return;

  style_ = style;
  computeFilterCoefficients();
  repaint();
}

void FilterResponse::setActive(bool active) {
  active_ = active;
  repaint();
}
