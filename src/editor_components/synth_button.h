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

#ifndef SYNTH_BUTTON_H
#define SYNTH_BUTTON_H

#include <JuceHeader.h>
#include "helmBoy_common.h"

class FullInterface;

/**
 * @file synth_button.h
 * @brief Boutons stylisés pour l'interface (toggle, contextual labels).
 */

class SynthButton : public ToggleButton {
  public:
    class ButtonListener {
      public:
        virtual ~ButtonListener() { }
        virtual void guiChanged(SynthButton* button) { }
        virtual void modulationsChanged(SynthButton* button) { }
        virtual bool modulationGesture(SynthButton* button, const ModifierKeys& mods) {
          juce::ignoreUnused(button, mods);
          return false;
        }
    };

    SynthButton(String name);

    void setStringLookup(const std::string* lookup) {
      string_lookup_ = lookup;
    }
    const std::string* getStringLookup() const { return string_lookup_; }
    String getTextFromValue(bool value);

    void setActive(bool active = true);
    bool isActive() const { return active_; }

    void setModulated(bool modulated = true);
    bool isModulated() const { return modulated_; }

    void setModulationEditActive(bool active = true);
    bool isModulationEditActive() const { return modulation_edit_active_; }

    /** @brief Set whether modulation currently forces this button on. */
    void setModulationActive(bool active);
    /** @brief Return whether modulation currently forces this button on. */
    bool isModulationActive() const { return modulation_active_; }

    void handlePopupResult(int result);
    void notifyModulationsChanged();

    virtual void mouseDown(const MouseEvent& e) override;
    virtual void mouseUp(const MouseEvent& e) override;
    virtual void mouseEnter(const MouseEvent& e) override;

    void addButtonListener(ButtonListener* listener);
    void removeButtonListener(ButtonListener* listener) { std::erase(button_listeners_, listener); }

  private:
    void buttonStateChanged() override;
    void notifyTooltip();

    bool active_;
    bool modulated_;
    bool modulation_edit_active_;
    bool modulation_active_;
    const std::string* string_lookup_;
    FullInterface* parent_;

    std::vector<ButtonListener*> button_listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthButton)
};

#endif // SYNTH_BUTTON_H
