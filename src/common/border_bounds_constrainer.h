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

/**
 * @file border_bounds_constrainer.h
 * @brief Constrains window/component bounds within configurable borders.
 */

#pragma once
#ifndef BORDER_BOUNDS_CONSTRAINER_H
#define BORDER_BOUNDS_CONSTRAINER_H

#include <JuceHeader.h>
#include "delete_section.h"

/**
 * @class BorderBoundsConstrainer
 * @brief Enforces a visible border when resizing components or windows.
 */
class BorderBoundsConstrainer : public ComponentBoundsConstrainer {
  public:
    BorderBoundsConstrainer() : ComponentBoundsConstrainer() { }

    /**
     * @brief Enforce the configured border when resizing or moving a component.
     * @param bounds Bounds to check and possibly modify (in/out).
     * @param previous Previous bounds of the component.
     * @param limits Allowed limits for the component bounds.
     * @param stretching_top True if the top edge is being stretched.
     * @param stretching_left True if the left edge is being stretched.
     * @param stretching_bottom True if the bottom edge is being stretched.
     * @param stretching_right True if the right edge is being stretched.
     */
    virtual void checkBounds(Rectangle<int>& bounds, const Rectangle<int>& previous,
                 const Rectangle<int>& limits,
                 bool stretching_top, bool stretching_left,
                 bool stretching_bottom, bool stretching_right) override;

    void setBorder(BorderSize<int> border) { border_ = border; }

  protected:
    BorderSize<int> border_;
};

#endif // BORDER_BOUNDS_CONSTRAINER_H
