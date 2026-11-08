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

#include "text_look_and_feel.h"

#include "colors.h"
#include "fonts.h"
#include "synth_button.h"
#include "synth_slider.h"

TextLookAndFeel::TextLookAndFeel() {
  setColour(ComboBox::backgroundColourId, Colors::background);
  setColour(ComboBox::arrowColourId, Colors::combo_arrow);
  setColour(ComboBox::outlineColourId, Colors::combo_outline);
  setColour(ComboBox::textColourId, Colors::control_label_text);
  setColour(Label::textColourId, Colors::label_text);
  setColour(ListBox::backgroundColourId, Colors::background);
  setColour(ListBox::textColourId, Colors::list_text);
}

void TextLookAndFeel::drawRotarySlider(Graphics& g, int x, int y, int width, int height,
                                       float slider_t, float start_angle, float end_angle,
                                       Slider& slider) {
  float text_percentage = 0.7f;
  static const int mod_buffer = 5;
  Rectangle<int> text_bounds(x + mod_buffer, y, width - 2 * mod_buffer, height);

  bool active = true;
  SynthSlider* s_slider = dynamic_cast<SynthSlider*>(&slider);
  if (s_slider) {
    active = s_slider->isActive();
    if (s_slider->getName() == "osc_1_unison_detune" ||
        s_slider->getName() == "osc_2_unison_detune")
      text_percentage = 0.5f;
  }

  g.setColour(Colors::text_background);
  g.fillRect(x + mod_buffer, y, width - 2 * mod_buffer, height);

  if (active)
    g.setColour(Colors::text_background_highlighted);
  else
    g.setColour(Colors::text_background);
  g.drawRect(slider.getLocalBounds());

  if (active)
    g.setColour(Colours::white);
  else
    g.setColour(Colors::text_button_normal);

  g.setFont(Fonts::instance()->monospace().withPointHeight(height * text_percentage));
  g.drawFittedText(slider.getTextFromValue(slider.getValue()), text_bounds,
                   Justification::centred, 1, 0.7f);
}

void TextLookAndFeel::drawToggleButton(Graphics& g, ToggleButton& button,
                                       bool hover, bool is_down) {
  static const float text_percentage = 0.7f;

  if (button.getToggleState())
    g.setColour(Colour(0xffffc400));
  else
    g.setColour(Colour(0xff313131));
  g.fillRect(button.getLocalBounds());

  SynthButton* synth_button = dynamic_cast<SynthButton*>(&button);
  if (synth_button && synth_button->isModulationActive()) {
    g.setColour(Colors::modulation.withAlpha(0.3f));
    g.fillRect(button.getLocalBounds().toFloat().reduced(1.5f));
  }

  g.setColour(Colours::white);
  int height = button.getHeight();
  g.setFont(Fonts::instance()->monospace().withPointHeight(height * text_percentage));
  g.drawText(button.getButtonText(), 0, 0,
             button.getWidth(), button.getHeight(), Justification::centred);

  g.setColour(Colour(0xff565656));
  g.drawRect(button.getLocalBounds());

  if (is_down) {
    g.setColour(Colour(0x11000000));
    g.fillRect(button.getLocalBounds());
  }
  else if (hover) {
    g.setColour(Colour(0x11ffffff));
    g.fillRect(button.getLocalBounds());
  }

  if (synth_button && synth_button->isModulationEditActive()) {
    g.setColour(Colors::modulation.withAlpha(0.85f));
    g.drawRect(button.getLocalBounds().toFloat().reduced(1.5f), 2.5f);
  }
}

void TextLookAndFeel::drawTickBox(Graphics& g, Component& component,
                                  float x, float y, float w, float h, bool ticked,
                                  bool enabled, bool mouse_over, bool button_down) {
  float border_width = 1.5f;
  g.setColour(Colour(0xffbbbbbb));
  g.drawRect(x + border_width, y + border_width,
             w - 2 * border_width, h - 2 * border_width, border_width);

  if (ticked) {
    g.setColour(Colour(0xffffd740));
    g.fillRect(x + 3 * border_width, y + 3 * border_width,
               w - 6 * border_width, h - 6 * border_width);
  }
}
