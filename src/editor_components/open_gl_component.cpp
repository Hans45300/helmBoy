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

#include "open_gl_component.h"
#include "full_interface.h"

using namespace juce::gl;

void OpenGLComponent::setViewPort(OpenGLContext& open_gl_context) {
  float scale = open_gl_context.getRenderingScale();
  FullInterface* parent = findParentComponentOfClass<FullInterface>();
  Rectangle<int> top_level_bounds = parent->getBounds();
  Rectangle<int> global_bounds = parent->getLocalArea(this, getLocalBounds());

  glViewport(scale * global_bounds.getX(),
             scale * (top_level_bounds.getHeight() - global_bounds.getBottom()),
             scale * global_bounds.getWidth(), scale * global_bounds.getHeight());
}


























































































