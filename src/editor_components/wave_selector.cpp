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

WaveSelector::WaveSelector(String name) : SynthSlider(name) { }

void WaveSelector::paint(Graphics& g) {
  static const PathStrokeType stroke(1.000f, PathStrokeType::curved, PathStrokeType::rounded);

  //Définition du nombre de types de formes d'ondes des oscillateurs
  oscillator_wf_types = 11;

  int selected = getValue();
  float cell_width = float(getWidth()) / getNumberOfTypesPerLine(getMinimum(), getMaximum());

  g.setColour(Colour(Colors::Color_ff000000));
  g.fillRect(getCellOffsetFromTypeNumber(selected) * cell_width, getHeightFromTypeNumber(selected) - TYPE_PADDING_Y, cell_width, float(getHeight() / 2));

  g.setColour(selected == 0 ? Colour(Colors::emerald) : Colors::Color_ffaaaaaa);
  g.strokePath(sine_, stroke);

  g.setColour(selected == 1 ? Colour(Colors::emerald) : Colors::Color_ffaaaaaa);
  g.strokePath(triangle_, stroke);

  g.setColour(selected == 2 ? Colour(Colors::emerald) : Colors::Color_ffaaaaaa);
  g.strokePath(square_, stroke);

  g.setColour(selected == 3 ? Colour(Colors::emerald) : Colors::Color_ffaaaaaa);
  g.strokePath(down_saw_, stroke);

  g.setColour(selected == 4 ? Colour(Colors::emerald) : Colors::Color_ffaaaaaa);
  g.strokePath(up_saw_, stroke);

  g.setColour(selected == 5 ? Colour(Colors::emerald) : Colors::Color_ffaaaaaa);
  g.strokePath(three_step_, stroke);

  g.setColour(selected == 6 ? Colour(Colors::emerald) : Colors::Color_ffaaaaaa);
  g.strokePath(four_step_, stroke);

  g.setColour(selected == 7 ? Colour(Colors::emerald) : Colors::Color_ffaaaaaa);
  g.strokePath(eight_step_, stroke);

  g.setColour(selected == 8 ? Colour(Colors::emerald) : Colors::Color_ffaaaaaa);
  g.strokePath(three_pyramid_, stroke);

  g.setColour(selected == 9 ? Colour(Colors::emerald) : Colors::Color_ffaaaaaa);
  g.strokePath(five_pyramid_, stroke);

  g.setColour(selected == 10 ? Colour(Colors::emerald) : Colors::Color_ffaaaaaa);
  g.strokePath(nine_pyramid_, stroke);

  if (getMaximum() >= oscillator_wf_types)
  {
    g.setColour(selected == 11 ? Colour(Colors::emerald) : Colors::Color_ffaaaaaa);
    g.strokePath(sample_and_hold_, stroke);
    g.setColour(selected == 12 ? Colour(Colors::emerald) : Colors::Color_ffaaaaaa);
    g.strokePath(noise_, stroke);
  }
}

void WaveSelector::resized() {
  SynthSlider::resized();
   cell_width = float(getWidth()) / getNumberOfTypesPerLine(getMinimum(), getMaximum());
   type_width = cell_width - 2 * TYPE_PADDING_X;
   type_height = getHeight() / 2 - 2 * TYPE_PADDING_Y;
   int actual_type_number = 0;
   resizeSin(getCellOffsetFromTypeNumber(actual_type_number) * cell_width + TYPE_PADDING_X, getHeightFromTypeNumber(actual_type_number), type_width, type_height);
   actual_type_number++;
   resizeTriangle(getCellOffsetFromTypeNumber(actual_type_number) * cell_width + TYPE_PADDING_X, getHeightFromTypeNumber(actual_type_number), type_width, type_height);
   actual_type_number++;
   resizeSquare(getCellOffsetFromTypeNumber(actual_type_number) * cell_width + TYPE_PADDING_X, getHeightFromTypeNumber(actual_type_number), type_width, type_height);
   actual_type_number++;
   resizeDownSaw(getCellOffsetFromTypeNumber(actual_type_number) * cell_width + TYPE_PADDING_X, getHeightFromTypeNumber(actual_type_number), type_width, type_height);
   actual_type_number++;
   resizeUpSaw(getCellOffsetFromTypeNumber(actual_type_number) * cell_width + TYPE_PADDING_X, getHeightFromTypeNumber(actual_type_number), type_width, type_height);
   actual_type_number++;
   resizeSteps<3>(three_step_, getCellOffsetFromTypeNumber(actual_type_number) * cell_width + TYPE_PADDING_X, getHeightFromTypeNumber(actual_type_number),
                  type_width, type_height);
   actual_type_number++;
   resizeSteps<4>(four_step_, getCellOffsetFromTypeNumber(actual_type_number) * cell_width + TYPE_PADDING_X, getHeightFromTypeNumber(actual_type_number),
                  type_width, type_height);
   actual_type_number++;
   resizeSteps<8>(eight_step_, getCellOffsetFromTypeNumber(actual_type_number) * cell_width + TYPE_PADDING_X, getHeightFromTypeNumber(actual_type_number),
                  type_width, type_height);
   actual_type_number++;
   resizePyramid<3>(three_pyramid_, getCellOffsetFromTypeNumber(actual_type_number) * cell_width + TYPE_PADDING_X, getHeightFromTypeNumber(actual_type_number),
                    type_width, type_height);
   actual_type_number++;
   resizePyramid<5>(five_pyramid_, getCellOffsetFromTypeNumber(actual_type_number) * cell_width + TYPE_PADDING_X, getHeightFromTypeNumber(actual_type_number),
                    type_width, type_height);
   actual_type_number++;
   resizePyramid<9>(nine_pyramid_, getCellOffsetFromTypeNumber(actual_type_number) * cell_width + TYPE_PADDING_X, getHeightFromTypeNumber(actual_type_number),
                    type_width, type_height);
   if (getMaximum() >= oscillator_wf_types)
   {
     actual_type_number++;
     resizeSampleAndHold(getCellOffsetFromTypeNumber(actual_type_number) * cell_width + TYPE_PADDING_X, getHeightFromTypeNumber(actual_type_number), type_width, type_height);
     actual_type_number++;
     resizeNoise(getCellOffsetFromTypeNumber(actual_type_number) * cell_width + TYPE_PADDING_X, getHeightFromTypeNumber(actual_type_number), type_width, type_height);
   }
}

  float WaveSelector::getCellOffsetFromTypeNumber(int type_number) {
    int num_types = getMaximum() - getMinimum() + 1;
    int types_per_line;
    float xOffset;
    // Si le nombre de waveforms est pair, le nombre de types par ligne est la moitié du nombre total
    // Sinon, c'est la moitié arrondie vers le haut

    types_per_line = (num_types % 2 == 0) ? (num_types / 2) : (num_types / 2 + 1);
    xOffset = (type_number >= types_per_line) ? (type_number - types_per_line) : type_number;
    return xOffset;
  }

  int WaveSelector::getNumberOfTypesPerLine(int minimum, int maximum)
  {
    int num_types = maximum - minimum + 1;
    // Si le nombre de waveforms est pair, le nombre de types par ligne est la moitié du nombre total
    // Sinon, c'est la moitié arrondie vers le haut
    return (num_types % 2 == 0) ? (num_types / 2) : (num_types / 2 + 1);
  }

  float WaveSelector::getHeightFromTypeNumber(int type_number)
  {
    int num_types = getMaximum() - getMinimum() + 1;
    int types_per_line;
    float yOffset;
    // Si le nombre de waveforms est pair, le nombre de types par ligne est la moitié du nombre total
    // Sinon, c'est la moitié arrondie vers le haut
    types_per_line = (num_types % 2 == 0) ? (num_types / 2) : (num_types / 2 + 1);
    float type_height = getHeight() / 2 - 2 * TYPE_PADDING_Y;

    if (type_number < types_per_line) {
      yOffset = TYPE_PADDING_Y;
    }
    else {
      yOffset = type_height + TYPE_PADDING_Y * 2;
    }
    return yOffset;
  }

void WaveSelector::mouseEvent(const juce::MouseEvent &e)
{
  //Choix de la waveform en fonction de la position du clic
  float x = e.getPosition().getX();
  float y = e.getPosition().getY();
  int index;
  index = (y > getHeight() / 2) ? getNumberOfTypesPerLine(getMinimum(), getMaximum()) : 0;
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
