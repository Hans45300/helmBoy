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

#ifndef BROWSER_LOOK_AND_FEEL_H
#define BROWSER_LOOK_AND_FEEL_H

#include <JuceHeader.h>
#include "text_look_and_feel.h"

class BrowserLookAndFeel : public juce::LookAndFeel_V3 {
  public:
    void drawButtonBackground(Graphics& g, Button& button, const Colour &backgroundColour,
                              bool hover, bool is_down) override;

    void drawToggleButton(Graphics& g, ToggleButton& button,
                          bool hover, bool is_down) override;

    Font getTextButtonFont(TextButton&, int buttonHeight) override;

    static BrowserLookAndFeel* instance() {
      static BrowserLookAndFeel instance;
      return &instance;
    }
};

#endif // BROWSER_LOOK_AND_FEEL_H
