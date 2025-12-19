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

 #include "colors.h"
#include "modulation_highlight.h"

ModulationHighlight::ModulationHighlight() {
  setInterceptsMouseClicks(false, false);
}

ModulationHighlight::~ModulationHighlight() {
}

void ModulationHighlight::paint(Graphics& g) {
  g.setColour(Colour(Colors::Color_44ffffff));
  g.fillEllipse(1, 2, getWidth() - 2, getHeight() - 2);
}

void ModulationHighlight::resized() {
  Component::resized();
  repaint();
}
