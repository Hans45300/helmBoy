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

#include "wave_selector.h"
#include "colors.h"
#include "fixed_point_wave.h"
#include "utils.h"
#include "wave_selector_geometry.h"
#include "wave.h"

#define TYPE_PADDING_X 1.0f
#define TYPE_PADDING_Y 4.0f

namespace {
  template<size_t steps> requires (steps > 1)
  void resizeSteps(Path& path, float x, float y, float width, float height) {
    path.clear();
    path.startNewSubPath(x, y + height / 2.0f);

    float inc_x = width / steps;
    float inc_y = height / (steps - 1);
    for (int i = 0; i < steps; ++i) {
      path.lineTo(x + i * inc_x, y + height - i * inc_y);
      path.lineTo(x + (i + 1) * inc_x, y + height - i * inc_y);
    }

    path.lineTo(x + width, y + height / 2.0f);
  }

  template<size_t steps> requires (steps > 1)
  void resizePyramid(Path& path, float x, float y, float width, float height) {
    static int parts = 2 * (steps - 1);

    path.clear();
    path.startNewSubPath(x, y + height / 2.0f);

    float inc_x = width / parts;
    float inc_y = height / (steps - 1);
    float cur_x = x + inc_x;

    for (int i = 0; i < (steps - 1) / 2; ++i) {
      path.lineTo(cur_x, y + height / 2.0f - i * inc_y);
      path.lineTo(cur_x, y + height / 2.0f - (i + 1) * inc_y);
      cur_x += inc_x;
    }

    for (int i = 0; i < steps - 1; ++i) {
      path.lineTo(cur_x, y + i * inc_y);
      path.lineTo(cur_x, y + (i + 1) * inc_y);
      cur_x += inc_x;
    }

    for (int i = 0; i < (steps - 1) / 2; ++i) {
      path.lineTo(cur_x, y + height - i * inc_y);
      path.lineTo(cur_x, y + height - (i + 1) * inc_y);
      cur_x += inc_x;
    }

    path.lineTo(x + width, y + height / 2.0f);
  }
} // namespace

WaveSelector::WaveSelector(String name, WaveSelectorKind kind)
  : SynthSlider(name), kind_(kind) { }

void WaveSelector::paint(Graphics& g) {
  static const PathStrokeType stroke(1.000f, PathStrokeType::curved, PathStrokeType::rounded);

  int selected = getValue();
  g.setColour(Colour(Colors::Color_ff000000));
  g.fillRect(getCellBounds(selected));

  paintCommonWaveforms(g, selected, stroke);
  if (kind_ == WaveSelectorKind::Oscillator)
    paintOscillatorWaveforms(g, selected, stroke);
  else
    paintLfoWaveforms(g, selected, stroke);
}

void WaveSelector::resized() {
  SynthSlider::resized();
  const int types_per_line = getNumberOfTypesPerLine(getMinimum(), getMaximum());
  if (types_per_line <= 0)
    return;

  cell_width = float(getWidth()) / types_per_line;
  type_width = cell_width - 2 * TYPE_PADDING_X;
  type_height = getHeight() / 2 - 2 * TYPE_PADDING_Y;
  resizeCommonWaveforms();
  if (kind_ == WaveSelectorKind::Oscillator)
    resizeOscillatorWaveforms();
  else
    resizeLfoWaveforms();
}

void WaveSelector::paintCommonWaveforms(Graphics& g, int selected,
                                         const PathStrokeType& stroke) {
  Path* paths[] = { &sine_, &triangle_, &square_, &down_saw_, &up_saw_,
                    &three_step_, &four_step_, &eight_step_, &three_pyramid_,
                    &five_pyramid_, &nine_pyramid_ };
  for (int index = 0; index < static_cast<int>(std::size(paths)); ++index) {
    g.setColour(selected == index ? Colour(Colors::emerald) : Colors::Color_ffaaaaaa);
    g.strokePath(*paths[index], stroke);
  }
}

void WaveSelector::paintOscillatorWaveforms(Graphics& g, int selected,
                                             const PathStrokeType& stroke) {
  const int index = mopo::FixedPointWaveLookup::kSawMobius;
  g.setColour(selected == index ? Colour(Colors::emerald) : Colors::Color_ffaaaaaa);
  g.strokePath(saw_mobius_, stroke);
}

void WaveSelector::paintLfoWaveforms(Graphics& g, int selected,
                                      const PathStrokeType& stroke) {
  const int sample_and_hold_index = static_cast<int>(mopo::Wave::Type::WhiteNoise);
  const int sample_and_glide_index = static_cast<int>(mopo::Wave::Type::SampleAndGlide);
  const int smooth_random_index = static_cast<int>(mopo::Wave::Type::SmoothRandom);
  g.setColour(selected == sample_and_hold_index ? Colour(Colors::emerald)
                                                 : Colors::Color_ffaaaaaa);
  g.strokePath(sample_and_hold_, stroke);
  g.setColour(selected == sample_and_glide_index ? Colour(Colors::emerald)
                                                  : Colors::Color_ffaaaaaa);
  g.strokePath(noise_, stroke);
  g.setColour(selected == smooth_random_index ? Colour(Colors::emerald)
                                              : Colors::Color_ffaaaaaa);
  g.strokePath(smooth_random_, stroke);
}

void WaveSelector::resizeCommonWaveforms() {
  for (int index = 0; index <= 10; ++index) {
    const float x = getCellOffsetFromTypeNumber(index) * cell_width + TYPE_PADDING_X;
    const float y = getHeightFromTypeNumber(index);
    switch (index) {
      case 0: resizeSin(x, y, type_width, type_height); break;
      case 1: resizeTriangle(x, y, type_width, type_height); break;
      case 2: resizeSquare(x, y, type_width, type_height); break;
      case 3: resizeDownSaw(x, y, type_width, type_height); break;
      case 4: resizeUpSaw(x, y, type_width, type_height); break;
      case 5: resizeSteps<3>(three_step_, x, y, type_width, type_height); break;
      case 6: resizeSteps<4>(four_step_, x, y, type_width, type_height); break;
      case 7: resizeSteps<8>(eight_step_, x, y, type_width, type_height); break;
      case 8: resizePyramid<3>(three_pyramid_, x, y, type_width, type_height); break;
      case 9: resizePyramid<5>(five_pyramid_, x, y, type_width, type_height); break;
      case 10: resizePyramid<9>(nine_pyramid_, x, y, type_width, type_height); break;
    }
  }
}

void WaveSelector::resizeOscillatorWaveforms() {
  const int index = mopo::FixedPointWaveLookup::kSawMobius;
  resizeSawMobius(getCellOffsetFromTypeNumber(index) * cell_width + TYPE_PADDING_X,
                  getHeightFromTypeNumber(index), type_width, type_height);
}

void WaveSelector::resizeLfoWaveforms() {
  const int sample_and_hold_index = static_cast<int>(mopo::Wave::Type::WhiteNoise);
  const int sample_and_glide_index = static_cast<int>(mopo::Wave::Type::SampleAndGlide);
  const int smooth_random_index = static_cast<int>(mopo::Wave::Type::SmoothRandom);
  resizeSampleAndHold(getCellOffsetFromTypeNumber(sample_and_hold_index) * cell_width + TYPE_PADDING_X,
                      getHeightFromTypeNumber(sample_and_hold_index), type_width, type_height);
  resizeNoise(getCellOffsetFromTypeNumber(sample_and_glide_index) * cell_width + TYPE_PADDING_X,
              getHeightFromTypeNumber(sample_and_glide_index), type_width, type_height);
  resizeSmoothRandom(getCellOffsetFromTypeNumber(smooth_random_index) * cell_width + TYPE_PADDING_X,
                     getHeightFromTypeNumber(smooth_random_index), type_width, type_height);
}

  float WaveSelector::getCellOffsetFromTypeNumber(int type_number) {
    const float width = float(getWidth()) /
        getNumberOfTypesPerLine(getMinimum(), getMaximum());
    return width > 0.0f ? getCellBounds(type_number).getX() / width : 0.0f;
  }

  int WaveSelector::getNumberOfTypesPerLine(int minimum, int maximum)
  {
    return helmBoy::ui::getWaveSelectorTypesPerLine(minimum, maximum);
  }

  float WaveSelector::getHeightFromTypeNumber(int type_number)
  {
    return getCellBounds(type_number).getY() + TYPE_PADDING_Y;
  }

  Rectangle<float> WaveSelector::getCellBounds(int waveform_index) const {
    const auto bounds = helmBoy::ui::getWaveSelectorCellBounds(
        waveform_index, getMinimum(), getMaximum(), float(getWidth()),
        float(getHeight()), TYPE_PADDING_Y);
    return { bounds.x, bounds.y, bounds.width, bounds.height };
  }

  int WaveSelector::getWaveformIndex(float value) const {
    const auto rounding = kind_ == WaveSelectorKind::Lfo
        ? mopo::utils::WaveformIndexRounding::Truncate
        : mopo::utils::WaveformIndexRounding::Nearest;
    return mopo::utils::waveformIndex(value, static_cast<int>(getMinimum()),
        static_cast<int>(getMaximum()), rounding);
  }

  bool WaveSelector::isSawMobiusWaveform(int waveform_index) const {
    return kind_ == WaveSelectorKind::Oscillator &&
        waveform_index == mopo::FixedPointWaveLookup::kSawMobius;
  }

void WaveSelector::mouseEvent(const juce::MouseEvent &e)
{
  //Choix de la waveform en fonction de la position du clic
  float x = e.getPosition().getX();
  float y = e.getPosition().getY();
  int index;
  index = static_cast<int>(getMinimum()) +
      ((y > getHeight() / 2) ? getNumberOfTypesPerLine(getMinimum(), getMaximum()) : 0);
  index += int(x / cell_width);
  if(index > getMaximum()) { return;}
  setValue(index);
}

void WaveSelector::mouseDown(const juce::MouseEvent &e) {
  if (e.mods.isPopupMenu()) {
    // Laisser SynthSlider gérer le menu contextuel (MIDI assignment, etc.)
    SynthSlider::mouseDown(e);
  }
  else {
    // Comportement normal de sélection de waveform
    mouseEvent(e);
  }
}

void WaveSelector::mouseDrag(const juce::MouseEvent &e) {
  mouseEvent(e);
}

void WaveSelector::valueChanged() {
  SynthSlider::valueChanged();
  repaint();
}

void WaveSelector::resizeSin(float x, float y, float width, float height) {
  sine_.clear();
  sine_.startNewSubPath(x, y + height / 2.0f);
  sine_.lineTo(x + width / 8.0f, y + height / 6.0f);
  sine_.lineTo(x + 2.0f * width / 8.0f, y);
  sine_.lineTo(x + 3.0f * width / 8.0f, y + height / 6.0f);
  sine_.lineTo(x + 5.0f * width / 8.0f, y + 5.0f * height / 6.0f);
  sine_.lineTo(x + 6.0f * width / 8.0f, y + height);
  sine_.lineTo(x + 7.0f * width / 8.0f, y + 5.0f * height / 6.0f);
  sine_.lineTo(x + 8.0f * width / 8.0f, y + height / 2.0f);
}

void WaveSelector::resizeTriangle(float x, float y, float width, float height) {
  triangle_.clear();
  triangle_.startNewSubPath(x, y + height / 2.0f);
  triangle_.lineTo(x + 1.0f * width / 4.0f, y);
  triangle_.lineTo(x + 3.0f * width / 4.0f, y + height);
  triangle_.lineTo(x + width, y + height / 2.0f);
}

void WaveSelector::resizeSquare(float x, float y, float width, float height) {
  square_.clear();
  square_.startNewSubPath(x, y + height / 2.0f);
  square_.lineTo(x, y);
  square_.lineTo(x + width / 2.0f, y);
  square_.lineTo(x + width / 2.0f, y + height);
  square_.lineTo(x + width, y + height);
  square_.lineTo(x + width, y + height / 2.0f);
}

void WaveSelector::resizeDownSaw(float x, float y, float width, float height) {
  down_saw_.clear();
  down_saw_.startNewSubPath(x, y + height / 2.0f);
  down_saw_.lineTo(x, y);
  down_saw_.lineTo(x + width, y + height);
  down_saw_.lineTo(x + width, y + height / 2.0f);
}

void WaveSelector::resizeUpSaw(float x, float y, float width, float height) {
  up_saw_.clear();
  up_saw_.startNewSubPath(x, y + height / 2.0f);
  up_saw_.lineTo(x, y + height);
  up_saw_.lineTo(x + width, y);
  up_saw_.lineTo(x + width, y + height / 2.0f);
}

void WaveSelector::resizeNoise(float x, float y, float width, float height) {
  static const int noise_icon_resolution = 14;
  srand(0);
  noise_.clear();
  noise_.startNewSubPath(x, y + height / 2.0f);
  float inc_x = width / noise_icon_resolution;

  for (int i = 1; i < noise_icon_resolution; ++i)
    noise_.lineTo(x + i * inc_x, y + height * (rand() % 100) / 100.0f);
  noise_.lineTo(x + width, y + height / 2.0f);
}

void WaveSelector::resizeSampleAndHold(float x, float y, float width, float height) {
  static const int sah_icon_resolution = 8;
  srand(0);
  sample_and_hold_.clear();
  sample_and_hold_.startNewSubPath(x, y + height / 2.0f);
  float inc_x = width / sah_icon_resolution;

  for (int i = 0; i < sah_icon_resolution; ++i) {
    float sample_value = y + height * (rand() % 100) / 100.0f;
    sample_and_hold_.lineTo(x + i * inc_x, sample_value);
    sample_and_hold_.lineTo(x + (i + 1) * inc_x, sample_value);
  }
  sample_and_hold_.lineTo(x + width, y + height / 2.0f);
}

void WaveSelector::resizeSmoothRandom(float x, float y, float width, float height) {
  static const float points[] = {0.5f, 0.2f, 0.8f, 0.35f, 0.65f, 0.5f};
  smooth_random_.clear();
  smooth_random_.startNewSubPath(x, y + height * points[0]);
  for (int i = 1; i < 6; ++i)
    smooth_random_.lineTo(x + width * i / 5.0f, y + height * points[i]);
}

void WaveSelector::resizeSawMobius(float x, float y, float width, float height) {
  constexpr int preview_points = 64;
  saw_mobius_.clear();
  for (int point = 0; point <= preview_points; ++point) {
    const float phase = static_cast<float>(point) / preview_points;
    const float value = static_cast<float>(mopo::Wave::sawMobius(phase));
    const float position_y = y + height * (1.0f - value) / 2.0f;
    const float position_x = x + width * phase;
    if (point == 0)
      saw_mobius_.startNewSubPath(position_x, position_y);
    else
      saw_mobius_.lineTo(position_x, position_y);
  }
}
