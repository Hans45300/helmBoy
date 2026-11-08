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

#include "wave_viewer.h"

#include "colors.h"
#include "synth_base.h"
#include "synth_gui_interface.h"
#include "wave_selector.h"
#include <cmath>

#define GRID_CELL_WIDTH 8
#define FRAMES_PER_SECOND 30
#define PADDING 5.0f
#define MARKER_WIDTH 6.0f

namespace {
  template <typename ValueAtPhase>
  void buildWavePaths(Path& curve, Path& fill, int resolution, float width,
                      float center_y, float padding, float draw_height,
                      ValueAtPhase value_at_phase) {
    curve.clear();
    fill.clear();
    if (resolution < 2)
      return;

    fill.startNewSubPath(0.0f, center_y);
    for (int i = 0; i < resolution; ++i) {
      const float t = static_cast<float>(i) / (resolution - 1);
      const float value = value_at_phase(t);
      const float y = padding + draw_height * ((1.0f - value) / 2.0f);
      if (i == 0) {
        curve.startNewSubPath(0.0f, y);
        fill.lineTo(0.0f, y);
      }
      else {
        const float x = t * width;
        curve.lineTo(x, y);
        fill.lineTo(x, y);
      }
    }
    fill.lineTo(width, center_y);
    fill.closeSubPath();
  }
} // namespace

WaveViewer::WaveViewer(int resolution) {
  wave_slider_ = nullptr;
  amplitude_slider_ = nullptr;
  phase_stretch_slider_ = nullptr;
  phase_offset_slider_ = nullptr;
  resolution_ = resolution;
  is_control_rate_ = false;
  phase_ = -1.0f;
  amp_ = 0.0;
  setOpaque(true);
}

WaveViewer::~WaveViewer() {
  stopTimer();
  for (auto* slider : { wave_slider_, amplitude_slider_, phase_stretch_slider_,
                        phase_offset_slider_ }) {
    if (auto* synth_slider = dynamic_cast<SynthSlider*>(slider))
      synth_slider->removeSliderListener(this);
  }
}

void WaveViewer::paint(juce::Graphics &g) {
  g.drawImageWithin(background_,
                    0, 0, getWidth(), getHeight(), RectanglePlacement());

  if (effective_shape_differs_) {
    const Colour waveform_colour = is_control_rate_ ? Colors::modulation : Colors::audio;
    g.setColour(waveform_colour.withAlpha(0.5f));
    g.fillPath(effective_wave_fill_path_);
    PathStrokeType stroke(1.0f * getRatio(), PathStrokeType::beveled,
                          PathStrokeType::rounded);
    g.setColour(waveform_colour);
    g.strokePath(effective_wave_path_, stroke);
  }

  if (phase_ >= 0.0f && phase_ < 1.0f) {
    float x = phaseToX(phase_);
    g.setColour(Colour(Colors::Color_33ffffff));
    g.fillRect(x - 0.5f, 0.0f, 1.0f, (float)getHeight());

    float y = PADDING + (getHeight() - 2 * PADDING) * (1.0f - amp_) / 2.0f;

    g.setColour(Colors::modulation);
    g.fillEllipse(x - MARKER_WIDTH / 2.0f, y - MARKER_WIDTH / 2.0f,
            MARKER_WIDTH, MARKER_WIDTH);
    g.setColour(Colour(Colors::Color_ff000000));
    g.fillEllipse(x - MARKER_WIDTH / 4.0f, y - MARKER_WIDTH / 4.0f,
            MARKER_WIDTH / 2.0f, MARKER_WIDTH / 2.0f);
  }
}

void WaveViewer::paintBackground(Graphics& g) {
  g.fillAll(Colour(Colors::Color_ff424242));

  g.setColour(Colour(Colors::Color_ff4a4a4a));
  for (int x = 0; x < getWidth(); x += GRID_CELL_WIDTH)
    g.drawLine(x, 0, x, getHeight());
  for (int y = 0; y < getHeight(); y += GRID_CELL_WIDTH)
    g.drawLine(0, y, getWidth(), y);

  g.setColour(Colors::graph_fill);
  g.fillPath(wave_fill_path_);

  if (is_control_rate_)
    g.setColour(Colors::modulation);
  else
    g.setColour(Colors::audio);

  float line_width = 1.0f * getRatio();
  PathStrokeType stroke(line_width, PathStrokeType::beveled, PathStrokeType::rounded);
  g.strokePath(wave_path_, stroke);
}

void WaveViewer::resized() {
  auto* display = Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds());
  const float scale = display != nullptr ? static_cast<float>(display->scale) : 1.0f;
  const int pixel_width = roundToInt(scale * getWidth());
  const int pixel_height = roundToInt(scale * getHeight());
  if (background_.getWidth() != pixel_width || background_.getHeight() != pixel_height ||
      background_scale_ != scale) {
    background_ = Image(Image::RGB, pixel_width, pixel_height, true);
    background_scale_ = scale;
  }
  if (realtime_feedback_enabled_)
    resolveRealtimeTelemetry();
  resetWavePath();
}

void WaveViewer::moved() {
  resized();
}

void WaveViewer::mouseDown(const MouseEvent& e) {
  mouse_down_position_ = e.position;
  phase_drag_started_ = false;
  if (wave_slider_ != nullptr)
    wave_slider_->beginParameterEditHistoryGesture();
  if (phase_offset_slider_ != nullptr)
    phase_offset_slider_->beginParameterEditHistoryGesture();
}

void WaveViewer::mouseDrag(const MouseEvent& e) {
  if (phase_offset_slider_ == nullptr || !e.mods.isLeftButtonDown() || getWidth() <= 0 ||
      e.position.getDistanceFrom(mouse_down_position_) < 2.0f)
    return;

  phase_drag_started_ = true;
  const float proportion = mopo::utils::clamp(e.position.x / getWidth(), 0.0f, 1.0f);
  phase_offset_slider_->setValue(phase_offset_slider_->getMinimum() + proportion *
      (phase_offset_slider_->getMaximum() - phase_offset_slider_->getMinimum()));
}

void WaveViewer::mouseUp(const MouseEvent& e) {
  if (!phase_drag_started_ && wave_slider_ != nullptr) {
    int current_value = wave_slider_->getValue();
    if (e.mods.isRightButtonDown())
      current_value += wave_slider_->getMaximum();
    else
      current_value += 1;
    wave_slider_->setValue(current_value % static_cast<int>(wave_slider_->getMaximum() + 1));
    resetWavePath();
  }

  if (wave_slider_ != nullptr)
    wave_slider_->endParameterEditHistoryGesture();
  if (phase_offset_slider_ != nullptr)
    phase_offset_slider_->endParameterEditHistoryGesture();
  phase_drag_started_ = false;
}

void WaveViewer::timerCallback() {
  auto* interface = findParentComponentOfClass<FullInterface>();
  const bool animate_feedback = interface != nullptr && interface->isAnimating();
  if (isShowing()) {
    if (realtime_feedback_enabled_)
      resolveRealtimeTelemetry();
    const bool animate = interface != nullptr && interface->isAnimating();
    const bool had_effective_shape = effective_shape_differs_;

    const auto updateModulatedValue = [interface, animate](
        Slider* slider, float base_value, float minimum, float maximum,
        float& modulated_value, bool& has_modulation) {
      has_modulation = false;
      if (!animate || slider == nullptr)
        return;

      const std::string name = slider->getName().toStdString();
      if (!interface->isDestinationModulated(name))
        return;

      float value = base_value;
      if (interface->copyModulatedValue(name, value) && std::isfinite(value)) {
        modulated_value = mopo::utils::clamp(value, minimum, maximum);
        has_modulation = true;
      }
    };

    updateModulatedValue(phase_stretch_slider_, getBasePhaseStretch(), 0.01f, 0.99f,
                        modulated_phase_stretch_, has_modulated_phase_stretch_);
    updateModulatedValue(phase_offset_slider_, getBasePhaseOffset(), -180.0f, 180.0f,
              modulated_phase_offset_, has_modulated_phase_offset_);
    updateModulatedValue(amplitude_slider_, getBaseAmplitude(),
                         amplitude_slider_ ? amplitude_slider_->getMinimum() : 0.0f,
                         amplitude_slider_ ? amplitude_slider_->getMaximum() : 1.0f,
                         modulated_amplitude_, has_modulated_amplitude_);

    if (auto* selector = dynamic_cast<WaveSelector*>(wave_slider_)) {
      has_modulated_waveform_ = false;
      if (interface != nullptr) {
        const std::string name = wave_slider_->getName().toStdString();
        if (interface->isDestinationModulated(name)) {
          float value = static_cast<float>(wave_slider_->getValue());
          if (interface->copyModulatedValue(name, value) && std::isfinite(value)) {
            modulated_waveform_index_ = selector->getWaveformIndex(value);
            has_modulated_waveform_ = true;
          }
        }
      }
    }

    const bool random_preview_changed = updateRandomPreview();
    const int effective_waveform_index = has_modulated_waveform_
        ? modulated_waveform_index_ : getBaseWaveformIndex();
    const float effective_phase_stretch = getPhaseStretch();
    const float effective_phase_offset = getPhaseOffset();
    const float effective_amplitude = getAmplitude();
    const bool next_shape_differs =
      (phaseStretchAffectsWave() &&
       std::abs(effective_phase_stretch - getBasePhaseStretch()) >= 0.0001f) ||
      std::abs(effective_phase_offset - getBasePhaseOffset()) >= 0.0001f ||
      std::abs(effective_amplitude - getBaseAmplitude()) >= 0.0001f ||
      effective_waveform_index != getBaseWaveformIndex();
    if (effective_waveform_index != displayed_waveform_index_ ||
        std::abs(effective_phase_stretch - displayed_phase_stretch_) >= 0.0001f ||
        std::abs(effective_phase_offset - displayed_phase_offset_) >= 0.0001f ||
        std::abs(effective_amplitude - displayed_amplitude_) >= 0.0001f ||
        had_effective_shape != next_shape_differs || random_preview_changed)
      resetWavePath();
  }

    if (isShowing() && animate_feedback && realtime_feedback_enabled_ &&
      synth_ != nullptr && telemetry_indices_[0] >= 0 && telemetry_indices_[1] >= 0 &&
      synth_->copyVisualTelemetry(telemetry_indices_, telemetry_values_)) {
    const float phase = telemetry_values_[0];
    amp_ = telemetry_values_[1];
    if (std::isfinite(phase) && phase >= 0.0f && phase < 1.0f && phase != phase_) {
      float last_x = phaseToX(phase_);
      float new_x = phaseToX(phase);
      phase_ = phase;
      repaint(last_x - MARKER_WIDTH / 2.0f - 1, 0.0, MARKER_WIDTH + 2, getHeight());
      repaint(new_x - MARKER_WIDTH / 2.0f - 1, 0.0, MARKER_WIDTH + 2, getHeight());
    }
  }
}

void WaveViewer::setWaveSlider(SynthSlider* slider) {
  wave_slider_ = slider;
  if (wave_slider_ != nullptr)
    wave_slider_->addSliderListener(this);
  startTimerHz(FRAMES_PER_SECOND);
  resetWavePath();
}

void WaveViewer::setPhaseOffsetSlider(SynthSlider* slider) {
  phase_offset_slider_ = slider;
  if (phase_offset_slider_ != nullptr)
    phase_offset_slider_->addSliderListener(this);
  has_modulated_phase_offset_ = false;
  startTimerHz(FRAMES_PER_SECOND);
  resetWavePath();
}

void WaveViewer::setAmplitudeSlider(SynthSlider* slider) {
  amplitude_slider_ = slider;
  if (amplitude_slider_ != nullptr)
    amplitude_slider_->addSliderListener(this);
  resetWavePath();
}

void WaveViewer::setPhaseStretchSlider(SynthSlider* slider) {
  phase_stretch_slider_ = slider;
  if (phase_stretch_slider_ != nullptr)
    phase_stretch_slider_->addSliderListener(this);
  has_modulated_phase_stretch_ = false;
  startTimerHz(FRAMES_PER_SECOND);
  resetWavePath();
}

float WaveViewer::getPhaseStretch() const {
  const float value = has_modulated_phase_stretch_ ? modulated_phase_stretch_
                                                   : getBasePhaseStretch();
  return std::isfinite(value) ? mopo::utils::clamp(value, 0.01f, 0.99f) : 0.5f;
}

float WaveViewer::getBasePhaseStretch() const {

  const float value = phase_stretch_slider_
      ? static_cast<float>(phase_stretch_slider_->getValue()) : 0.5f;
  return std::isfinite(value) ? mopo::utils::clamp(value, 0.01f, 0.99f) : 0.5f;
}

float WaveViewer::getBasePhaseOffset() const {
  const float value = phase_offset_slider_
      ? static_cast<float>(phase_offset_slider_->getValue()) : 0.0f;
  return std::isfinite(value) ? mopo::utils::clamp(value, -180.0f, 180.0f) : 0.0f;
}

float WaveViewer::getPhaseOffset() const {
  const float value = has_modulated_phase_offset_ ? modulated_phase_offset_
                                                  : getBasePhaseOffset();
  return std::isfinite(value) ? mopo::utils::clamp(value, -180.0f, 180.0f) : 0.0f;
}

float WaveViewer::getBaseAmplitude() const {
  return amplitude_slider_ ? static_cast<float>(amplitude_slider_->getValue()) : 1.0f;
}

int WaveViewer::getBaseWaveformIndex() const {
  if (const auto* selector = dynamic_cast<const WaveSelector*>(wave_slider_))
    return selector->getWaveformIndex(static_cast<float>(wave_slider_->getValue()));
  return wave_slider_ != nullptr ? static_cast<int>(wave_slider_->getValue()) : 0;
}

float WaveViewer::getAmplitude() const {
  const float value = has_modulated_amplitude_ ? modulated_amplitude_ : getBaseAmplitude();
  return std::isfinite(value) ? value : getBaseAmplitude();
}

bool WaveViewer::phaseStretchAffectsWave() const {
  if (const auto* selector = dynamic_cast<const WaveSelector*>(wave_slider_);
      selector != nullptr && selector->isSawMobiusWaveform(getBaseWaveformIndex()))
    return true;
  return wave_slider_ != nullptr &&
      static_cast<mopo::Wave::Type>(static_cast<int>(wave_slider_->getValue())) !=
          mopo::Wave::Type::WhiteNoise;
}

void WaveViewer::drawRandom() {
  drawRandom(wave_path_, wave_fill_path_, getBaseAmplitude(),
             getBasePhaseStretch(), getBasePhaseOffset());
}

void WaveViewer::drawRandom(Path& path, Path& fill_path, float amplitude,
                            float phase_stretch, float phase_offset) {
  const float padding = getRatio() * PADDING;
  const float draw_height = getHeight() - 2.0f * padding;
  buildWavePaths(path, fill_path, resolution_, getWidth(), getHeight() / 2.0f,
                 padding, draw_height, [=](float t) {
    float shifted_phase = t + phase_offset / 360.0f;
    shifted_phase -= std::floor(shifted_phase);
    const float warped_phase = mopo::utils::remapPhase(shifted_phase, phase_stretch);
    return amplitude * mopo::Wave::sampleAndHoldValue(
      random_preview_values_[0], random_preview_values_[1], warped_phase);
  });
}

void WaveViewer::drawSampleAndGlide(Path& path, Path& fill_path, float amplitude,
                                    float phase_stretch,
                                    float phase_offset) {
  const float padding = getRatio() * PADDING;
  const float draw_height = getHeight() - 2.0f * padding;
  buildWavePaths(path, fill_path, resolution_, getWidth(), getHeight() / 2.0f,
                 padding, draw_height, [=](float t) {
    float shifted_phase = t + phase_offset / 360.0f;
    shifted_phase -= std::floor(shifted_phase);
    const float warped_phase = mopo::utils::remapPhase(shifted_phase, phase_stretch);
    return amplitude * mopo::Wave::sampleAndGlideValue(
      random_preview_values_[0], random_preview_values_[1], warped_phase);
  });
}

void WaveViewer::drawSmoothRandom() {
  drawSmoothRandom(wave_path_, wave_fill_path_, getBaseAmplitude(),
                   getBasePhaseStretch(), getBasePhaseOffset());
}

void WaveViewer::drawSmoothRandom(Path& path, Path& fill_path, float amplitude,
                                  float phase_stretch,
                                  float phase_offset) {
  const float padding = getRatio() * PADDING;
  const float draw_height = getHeight() - 2.0f * padding;
  buildWavePaths(path, fill_path, resolution_, getWidth(), getHeight() / 2.0f,
                 padding, draw_height, [=](float t) {
    float shifted_phase = t + phase_offset / 360.0f;
    shifted_phase -= std::floor(shifted_phase);
    const float warped_phase = mopo::utils::remapPhase(shifted_phase, phase_stretch);
    return amplitude * mopo::Wave::smoothRandomValue(
      random_preview_values_.data(), warped_phase);
  });
}

void WaveViewer::buildWavePath(Path& path, Path& fill_path, int waveform_index,
                               float amplitude, float phase_stretch,
                               float phase_offset) {
  path.clear();
  fill_path.clear();
  if (wave_slider_ == nullptr)
    return;

  const float padding = getRatio() * PADDING;
  const float draw_height = getHeight() - 2.0f * padding;
  if (const auto* selector = dynamic_cast<const WaveSelector*>(wave_slider_);
      selector != nullptr && selector->isSawMobiusWaveform(waveform_index)) {
    buildWavePaths(path, fill_path, resolution_, getWidth(), getHeight() / 2.0f,
                   padding, draw_height, [=](float t) {
      float shifted_phase = t + phase_offset / 360.0f;
      shifted_phase -= std::floor(shifted_phase);
      const float warped_phase = mopo::utils::remapPhase(shifted_phase, phase_stretch);
      return amplitude * mopo::Wave::sawMobius(warped_phase);
    });
    return;
  }

  const auto type = static_cast<mopo::Wave::Type>(waveform_index);

  if (type < mopo::Wave::Type::WhiteNoise) {
    buildWavePaths(path, fill_path, resolution_, getWidth(), getHeight() / 2.0f,
                   padding, draw_height, [=](float t) {
      float shifted_phase = t + phase_offset / 360.0f;
      shifted_phase -= std::floor(shifted_phase);
      return amplitude * mopo::Wave::wave(
          type, mopo::utils::remapPhase(shifted_phase, phase_stretch));
    });
  }
  else if (type == mopo::Wave::Type::WhiteNoise)
    drawRandom(path, fill_path, amplitude, phase_stretch, phase_offset);
  else if (type == mopo::Wave::Type::SampleAndGlide)
    drawSampleAndGlide(path, fill_path, amplitude, phase_stretch, phase_offset);
  else
    drawSmoothRandom(path, fill_path, amplitude, phase_stretch, phase_offset);
}

void WaveViewer::resetWavePath() {
  if (!background_.isValid())
    return;

  const float base_amplitude = getBaseAmplitude();
  const float base_phase_stretch = getBasePhaseStretch();
  const float base_phase_offset = getBasePhaseOffset();
  const float effective_amplitude = getAmplitude();
  const float effective_phase_stretch = getPhaseStretch();
  const float effective_phase_offset = getPhaseOffset();
  const int base_waveform_index = getBaseWaveformIndex();
  const int effective_waveform_index = has_modulated_waveform_
      ? modulated_waveform_index_ : base_waveform_index;
  displayed_phase_stretch_ = effective_phase_stretch;
  displayed_phase_offset_ = effective_phase_offset;
  displayed_amplitude_ = effective_amplitude;
  displayed_waveform_index_ = effective_waveform_index;
  effective_shape_differs_ =
      effective_waveform_index != base_waveform_index ||
      std::abs(effective_amplitude - base_amplitude) >= 0.0001f ||
      (phaseStretchAffectsWave() &&
       std::abs(effective_phase_stretch - base_phase_stretch) >= 0.0001f) ||
      std::abs(effective_phase_offset - base_phase_offset) >= 0.0001f;

  buildWavePath(wave_path_, wave_fill_path_, base_waveform_index, base_amplitude,
                base_phase_stretch, base_phase_offset);
  if (effective_shape_differs_)
    buildWavePath(effective_wave_path_, effective_wave_fill_path_, effective_waveform_index,
                  effective_amplitude,
                  effective_phase_stretch, effective_phase_offset);
  else {
    effective_wave_path_.clear();
    effective_wave_fill_path_.clear();
  }

  const float scale = background_scale_ > 0.0f ? background_scale_ : 1.0f;
  Graphics g(background_);
  g.addTransform(AffineTransform::scale(scale, scale));
  paintBackground(g);

  repaint();
}

void WaveViewer::guiChanged(SynthSlider* slider) {
  resetWavePath();
  repaint();
}

void WaveViewer::showRealtimeFeedback(bool show_feedback) {
  realtime_feedback_enabled_ = show_feedback;
  if (show_feedback) {
    resolveRealtimeTelemetry();
    startTimerHz(FRAMES_PER_SECOND);
  }
  else {
    telemetry_indices_ = {-1, -1};
    random_telemetry_indices_.fill(-1);
    synth_ = nullptr;
    stopTimer();
    if (phase_stretch_slider_ || phase_offset_slider_)
      startTimerHz(FRAMES_PER_SECOND);
    repaint();
  }
}

void WaveViewer::resolveRealtimeTelemetry() {
  if (!realtime_feedback_enabled_)
    return;

  if (synth_ == nullptr) {
    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
      return;
    synth_ = parent->getSynth();
  }

  if (synth_ == nullptr)
    return;

  const std::string source_name = getName().toStdString();
  mopo::Output* amplitude = synth_->getModSource(source_name + "_amp");
  if (amplitude == nullptr)
    amplitude = synth_->getModSource(source_name);

  auto& visual_sources = synth_->getEngine()->getVisualTelemetrySources();
  const auto phase = visual_sources.find(source_name + "_display_phase");
  telemetry_indices_[0] = synth_->getVisualTelemetryIndex(
      phase == visual_sources.end() ? nullptr : phase->second);
  telemetry_indices_[1] = synth_->getVisualTelemetryIndex(amplitude);

  for (int index = 0; index < static_cast<int>(random_telemetry_indices_.size()); ++index) {
    const auto point = visual_sources.find(
        source_name + "_random_point_" + std::to_string(index));
    random_telemetry_indices_[index] = synth_->getVisualTelemetryIndex(
        point == visual_sources.end() ? nullptr : point->second);
  }
}

bool WaveViewer::updateRandomPreview() {
  if (!realtime_feedback_enabled_ || synth_ == nullptr)
    return false;
  for (const int index : random_telemetry_indices_)
    if (index < 0)
      return false;
  if (!synth_->copyVisualTelemetry(random_telemetry_indices_, random_telemetry_values_))
    return false;

  bool changed = false;
  for (std::size_t index = 0; index < random_preview_values_.size(); ++index) {
    const float value = random_telemetry_values_[index];
    if (!std::isfinite(value))
      continue;
    const float bounded = mopo::utils::clamp(value, -1.0f, 1.0f);
    if (std::abs(bounded - random_preview_values_[index]) >= 0.0001f) {
      random_preview_values_[index] = bounded;
      changed = true;
    }
  }
  return changed;
}

float WaveViewer::phaseToX(float phase) {
  return phase * getWidth();
}

float WaveViewer::getRatio() {
  return getHeight() / 80.0f;
}
