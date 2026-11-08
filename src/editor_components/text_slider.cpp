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

#include "text_slider.h"
#include "colors.h"
#include "fonts.h"

TextSlider::TextSlider(String name) : SynthSlider(name), short_lookup_(nullptr) { }

void TextSlider::paint(Graphics& g) {
  static const PathStrokeType stroke(1.000f, PathStrokeType::curved, PathStrokeType::rounded);

  int num_types = getMaximum() - getMinimum() + 1;
  float cell_width = float(getWidth()) / num_types;
  float height = getHeight();

  int selected = getValue();
  // Couleur du fond du slider
  g.setColour(Colour(Colors::Color_ff323232));
  g.fillRect(selected * cell_width, 0.0f, cell_width, height);

  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(height * text_size_scale_));

  const std::string* lookup = short_lookup_;
  if (lookup == nullptr)
    lookup = string_lookup_;

  for (int i = 0; i < num_types; ++i) {
    if (selected == i)
      // Couleur du texte lorsque le slider est actif
      g.setColour(Colour(Colors::Color_ffffffff));
    else
      // Couleur du texte lorsque le slider est inactif
      g.setColour(Colour(Colors::Color_ffaaaaaa));

    g.drawText(lookup[i], i * cell_width, 0.0f, cell_width, height, Justification::centred);
  }
}

void TextSlider::resized() {
  SynthSlider::resized();
}

void TextSlider::mouseEvent(const juce::MouseEvent &e) {
  float x = e.getPosition().getX();
  int index = x * (getMaximum() + 1) / getWidth();
  setValue(index);
}

void TextSlider::mouseDown(const juce::MouseEvent &e) {
  if (e.mods.isPopupMenu())
    SynthSlider::mouseDown(e);
  else
    mouseEvent(e);
}

void TextSlider::mouseDrag(const juce::MouseEvent &e) {
  mouseEvent(e);
}
