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

#include "synth_slider.h"
#include "colors.h"
#include "default_look_and_feel.h"
#include "full_interface.h"
#include "helmBoy_common.h"
#include "synth_gui_interface.h"
#include "text_look_and_feel.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <memory>

#define DEFAULT_POPUP_BUFFER 10

namespace {
  enum MenuIds {
    kCancel = 0,
    kArmMidiLearn,
    kClearMidiLearn,
    kDefaultValue,
    kEnterValue,
    kClearModulations,
    kModulationList
  };

} // namespace

const float SynthSlider::rotary_angle = 0.8f * static_cast<float>(mopo::PI);
const float SynthSlider::linear_rail_width = 2.0f;

SynthSlider::SynthSlider(String name) : Slider(name), bipolar_(false), flip_coloring_(false),
                                        active_(true), snap_to_value_(false), snap_value_(0.0),
                                        string_lookup_(nullptr), parent_(nullptr) {
  popup_placement_ = BubbleComponent::below;
  popup_buffer_ = DEFAULT_POPUP_BUFFER;

  if (!mopo::Parameters::isParameter(name.toStdString()))
    return;

  setRotaryParameters(2.0f * mopo::PI - rotary_angle, 2.0f * mopo::PI + rotary_angle, true);
  details_ = mopo::Parameters::getDetails(name.toStdString());
  if (details_.steps)
    setRange(details_.min, details_.max, (details_.max - details_.min) / (details_.steps - 1));
  else
    setRange(details_.min, details_.max);

  setDoubleClickReturnValue(true, details_.default_value);
  setTextBoxStyle(Slider::NoTextBox, true, 0, 0);

  // Avoid stale cached bitmaps when controls are toggled or relayouted.
  setBufferedToImage(false);
  setColour(Slider::backgroundColourId, Colour(Colors::Color_ff303030));
  setColour(Slider::textBoxOutlineColourId, Colour(Colors::Color_00000000));
}

void SynthSlider::resized() {
  if (parent_ == nullptr)
    parent_ = findParentComponentOfClass<FullInterface>();

  setPopupDisplayEnabled(true, false, parent_);
  Slider::resized();
}

void SynthSlider::mouseDown(const MouseEvent& e) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;
  SynthBase* synth = parent->getSynth();

  if (e.mods.isPopupMenu()) {
    PopupMenu m;
    m.setLookAndFeel(DefaultLookAndFeel::instance());

    if (isDoubleClickReturnEnabled())
      m.addItem(kDefaultValue, "Set to Default Value");

    m.addItem(kEnterValue, "Enter Value...");

    std::vector<mopo::ModulationConnection*> connections;
    m.addItem(kArmMidiLearn, "Learn MIDI Assignment");
    if (parent->getSynth()->isMidiMapped(getName().toStdString()))
      m.addItem(kClearMidiLearn, "Clear MIDI Assignment");

    connections = parent->getSynth()->getDestinationConnections(getName().toStdString());

    String disconnect("Disconnect from ");
    for (int i = 0; i < connections.size(); ++i)
      m.addItem(kModulationList + i, disconnect + connections[i]->source);

    if (connections.size() > 1)
      m.addItem(kClearModulations, "Disconnect all modulations");

    showModulationPopup(m, this, false, kModulationList + static_cast<int>(connections.size()),
               [this](int result) { handlePopupResult(result); });
  }
  else {
    if (parent)
      synth->beginChangeGesture(getName().toStdString());
    Slider::mouseDown(e);
    if (isRotary()) {
      click_position_ = e.getScreenPosition().toFloat();
      setMouseCursor(MouseCursor::NoCursor);
    }
  }
}

void SynthSlider::mouseUp(const MouseEvent& e) {
  if (!e.mods.isPopupMenu()) {
    Slider::mouseUp(e);

    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent)
      parent->getSynth()->endChangeGesture(getName().toStdString());

    if (isRotary()) {
      setMouseCursor(MouseCursor::ParentCursor);
      Desktop::getInstance().getMainMouseSource().setScreenPosition(click_position_);
    }
  }
}

void SynthSlider::beginParameterEditHistoryGesture() {
  if (auto* parent = findParentComponentOfClass<SynthGuiInterface>())
    parent->getSynth()->beginParameterEditGesture(getName().toStdString());
}

void SynthSlider::endParameterEditHistoryGesture() {
  if (auto* parent = findParentComponentOfClass<SynthGuiInterface>())
    parent->getSynth()->endParameterEditGesture(getName().toStdString());
}

void SynthSlider::mouseEnter(const MouseEvent &e) {
  Slider::mouseEnter(e);
  notifyTooltip();
  for (auto* listener : slider_listeners_)
    listener->hoverStarted(getName().toStdString());
}

void SynthSlider::mouseExit(const MouseEvent &e) {
  Slider::mouseExit(e);
  for (auto* listener : slider_listeners_)
    listener->hoverEnded(getName().toStdString());
}

void SynthSlider::valueChanged() {
  Slider::valueChanged();
  notifyTooltip();
  notifyGuis();

  if (popup_placement_ == BubbleComponent::below && popup_buffer_) {
    Component* popup = getCurrentPopupDisplay();
    if (popup) {
      Rectangle<int> bounds = popup->getBounds();
      Rectangle<int> local_bounds = getLocalArea(popup, popup->getLocalBounds());

      int y_diff = getHeight() + popup_buffer_ - local_bounds.getY();
      bounds.setY(bounds.getY() + y_diff);
      popup->setBounds(bounds);
    }
  }
}

String SynthSlider::getTextFromValue(double value) {
  if (string_lookup_) {
    int lookup = mopo::utils::iclamp(value, 0, getMaximum());
    return string_lookup_[lookup];
  }

  float display_value = value;
  switch (details_.display_skew) {
  case mopo::DisplaySkew::kQuadratic:
      display_value = powf(display_value, 2.0f);
      break;
  case mopo::DisplaySkew::kExponential:
      display_value = powf(2.0f, display_value);
      break;
  case mopo::DisplaySkew::kSquareRoot:
      display_value = sqrt(display_value);
      break;
    default:
      break;
  }
  display_value += details_.post_offset;
  if (details_.display_invert)
    display_value = 1.0 / display_value;
  display_value *= details_.display_multiply;

  return formatValue(display_value);
}

double SynthSlider::snapValue(double attempted_value, DragMode drag_mode) {
  const double percent = 0.05;
  if (!snap_to_value_ || drag_mode != DragMode::absoluteDrag)
    return attempted_value;

  double range = getMaximum() - getMinimum();
  double radius = percent * range;
  if (attempted_value - snap_value_ <= radius && attempted_value - snap_value_ >= -radius)
    return snap_value_;
  return attempted_value;
}

void SynthSlider::drawShadow(Graphics &g) {
  if (&getLookAndFeel() == TextLookAndFeel::instance())
    drawRectangularShadow(g);
  else if (isRotary())
    drawRotaryShadow(g);
  else {
    g.setColour(Colour(Colors::Color_ff222222));
    g.fillRect(getBounds());
  }
}

void SynthSlider::drawRotaryShadow(Graphics &g) {
  static const DropShadow shadow(Colour(Colors::Color_ee000000), 3, Point<int>(0, 0));
  static const float stroke_percent = 0.12f;

  g.saveState();
  g.setOrigin(getX(), getY());

  float full_radius = std::min(getWidth() / 2.0f, getHeight() / 2.0f);
  float stroke_width = 2.0f * full_radius * stroke_percent;
  Path shadow_path;
  float outer_radius = full_radius - stroke_width;
  shadow_path.addCentredArc(full_radius, full_radius,
                            0.89f * full_radius, 0.87f * full_radius,
                            0, -rotary_angle, rotary_angle, true);
  shadow.drawForPath(g, shadow_path);

  Path rail_outer;
  rail_outer.addCentredArc(full_radius, full_radius, outer_radius, outer_radius,
                           0.0f, -rotary_angle, rotary_angle, true);

  g.setColour(Colour(Colors::Color_ff333333));

  PathStrokeType outer_stroke =
      PathStrokeType(stroke_width, PathStrokeType::beveled, PathStrokeType::butt);
  g.strokePath(rail_outer, outer_stroke);

  g.restoreState();
}

void SynthSlider::drawRectangularShadow(Graphics &g) {
  static const DropShadow shadow(Colour(Colors::Color_bb000000), 2, Point<int>(0, 0));

  g.saveState();
  g.setOrigin(getX(), getY());
  shadow.drawForRectangle(g, getLocalBounds());
  g.setColour(Colour(Colors::Color_ff333333));
  g.fillRect(getLocalBounds());

  g.restoreState();
}

void SynthSlider::flipColoring(bool flip_coloring) {
  flip_coloring_ = flip_coloring;
  repaint();
}

void SynthSlider::setBipolar(bool bipolar) {
  bipolar_ = bipolar;
  repaint();
}

void SynthSlider::setActive(bool active) {
  active_ = active;
  repaint();
}

void SynthSlider::addSliderListener(SynthSlider::SliderListener* listener) {
  slider_listeners_.push_back(listener);
}

String SynthSlider::formatValue(float value) {
  static const int number_length = 5;
  static const int max_decimals = 3;

  if (details_.steps)
    return String(value) + " " + details_.display_units;

  String format = String(value, max_decimals);
  format = format.substring(0, number_length);
  int spaces = number_length - format.length();

  for (int i = 0; i < spaces; ++i)
    format = " " + format;

  return format + " " + details_.display_units;
}

void SynthSlider::notifyGuis() {
  for (auto* listener : slider_listeners_)
    listener->guiChanged(this);
}

void SynthSlider::handlePopupResult(int result) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  SynthBase* synth = parent->getSynth();
  std::vector<mopo::ModulationConnection*> connections =
      parent->getSynth()->getDestinationConnections(getName().toStdString());

  if (result == kArmMidiLearn)
    synth->armMidiLearn(getName().toStdString());
  else if (result == kClearMidiLearn)
    synth->clearMidiLearn(getName().toStdString());
  else if (result == kDefaultValue)
    setValue(getDoubleClickReturnValue());
  else if (result == kEnterValue)
    promptForDirectValueEntry();
  else if (result == kClearModulations)
    disconnectAllModulations();
  else if (result >= kModulationList) {
    int connection_index = result - kModulationList;
    if (connection_index < 0 || connection_index >= static_cast<int>(connections.size()))
      return;
    std::string source = connections[connection_index]->source;
    disconnectModulationFromSource(source);
  }
}

void SynthSlider::disconnectModulationFromSource(const std::string& source) {
  auto* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  auto connections = parent->getSynth()->getDestinationConnections(getName().toStdString());
  for (auto* connection : connections) {
    if (connection->source == source) {
      parent->getSynth()->disconnectModulation(connection);
      for (auto* listener : slider_listeners_)
        listener->modulationsChanged(getName().toStdString());
      return;
    }
  }
}

void SynthSlider::disconnectAllModulations() {
  auto* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  auto connections = parent->getSynth()->getDestinationConnections(getName().toStdString());
  if (connections.empty())
    return;

  for (auto* connection : connections)
    parent->getSynth()->disconnectModulation(connection);
  for (auto* listener : slider_listeners_)
    listener->modulationsChanged(getName().toStdString());
}

void SynthSlider::showModulationPopup(PopupMenu& menu, Component* owner, bool source_menu,
                                      int first_value_id,
                                      std::function<void(int)> handle_existing) {
  auto* parent = owner->findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  const std::string name = owner->getName().toStdString();
  const auto connections = source_menu ? parent->getSynth()->getSourceConnections(name) :
                                        parent->getSynth()->getDestinationConnections(name);
  std::vector<std::pair<std::string, std::string>> routes;
  for (auto* connection : connections) {
    menu.addItem(first_value_id + static_cast<int>(routes.size()),
                 "Enter Value for " + String(source_menu ? connection->destination : connection->source));
    routes.emplace_back(connection->source, connection->destination);
  }

  SafePointer<Component> safe_owner(owner);
  menu.showMenuAsync(PopupMenu::Options(), ModalCallbackFunction::create(
      [safe_owner, routes, first_value_id, handle_existing](int result) {
        if (safe_owner == nullptr || result == 0)
          return;
        const int route_index = result - first_value_id;
        if (route_index >= 0 && route_index < static_cast<int>(routes.size())) {
          auto* interface = safe_owner->findParentComponentOfClass<FullInterface>();
          if (interface != nullptr)
            interface->promptForModulationValueEntry(routes[route_index].first,
                                                     routes[route_index].second);
        }
        else
          handle_existing(result);
      }));
}

void SynthSlider::promptForModulationValueEntry(const std::string& source,
                                                std::function<void(double)> apply_value,
                                                Component* owner) {
  auto* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;
  mopo::ModulationConnection* connection = nullptr;
  for (auto* candidate : parent->getSynth()->getDestinationConnections(getName().toStdString())) {
    if (candidate->source == source) {
      connection = candidate;
      break;
    }
  }
  if (connection == nullptr)
    return;

  auto entry = std::make_shared<SynthSlider>(getName());
  entry->details_ = details_;
  entry->setStringLookup(string_lookup_);
  entry->setRange(getMinimum(), getMaximum(), getInterval());
  entry->setDoubleClickReturnValue(true, 0.0);
  entry->setValue(connection->amount.value(), dontSendNotification);
  entry->promptForDirectValueEntry(
      [entry, apply_value](double amount) { apply_value(amount); },
      String(source) + " -> " + getSliderDisplayName(), owner);
}

void SynthSlider::promptForDirectValueEntry(std::function<void(double)> apply_value,
                                           String display_name, Component* owner) {
  String current_text;
  if (string_lookup_)
    current_text = String(static_cast<int>(std::round(getValue())));
  else {
    current_text = getTextFromValue(getValue()).trim();
    const String units = getUnits().trim();
    if (units.isNotEmpty() && current_text.endsWith(units))
      current_text = current_text.dropLastCharacters(units.length()).trimEnd();
  }

  const String message = (display_name.isEmpty() ? getSliderDisplayName() : display_name) +
                         "\n" + getAcceptedRangeText();
  auto* alert = new AlertWindow("Enter Value", message, AlertWindow::NoIcon,
                               owner != nullptr ? owner : this);
  alert->addTextEditor("value", current_text, "Value");
  alert->addButton("OK", 1, KeyPress(KeyPress::returnKey));
  alert->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

  SafePointer<SynthSlider> safe_this(this);
  alert->enterModalState(true,
                         ModalCallbackFunction::create([safe_this, alert, apply_value](int result) {
                           std::unique_ptr<AlertWindow> alert_cleanup(alert);
                           if (result == 1 && safe_this != nullptr)
                             safe_this->applyDirectValueEntry(alert->getTextEditorContents("value"),
                                                               apply_value);
                         }),
                         true);
}

void SynthSlider::applyDirectValueEntry(const String& input_text,
                                       const std::function<void(double)>& apply_value) {
  double parsed_display_value = 0.0;
  if (!tryParseDisplayValue(input_text, parsed_display_value)) {
    AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                                     "Invalid value",
                                     "Please enter a valid numeric value.");
    return;
  }

  double internal_value = 0.0;
  if (!tryConvertDisplayToInternalValue(parsed_display_value, internal_value)) {
    AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                                     "Invalid value",
                                     "This value cannot be represented for this control.");
    return;
  }

  const double safe_value = clampAndSnapToRange(internal_value);

  if (apply_value) {
    apply_value(safe_value);
    return;
  }

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent != nullptr)
    parent->getSynth()->beginChangeGesture(getName().toStdString());

  setValue(safe_value);

  if (parent != nullptr)
    parent->getSynth()->endChangeGesture(getName().toStdString());
}

bool SynthSlider::tryParseDisplayValue(const String& input_text, double& parsed_value) const {
  String normalized = input_text.trim().replaceCharacter(',', '.');
  if (normalized.isEmpty())
    return false;

  const std::string utf8 = normalized.toStdString();
  const char* begin = utf8.c_str();
  char* end = nullptr;
  const double value = std::strtod(begin, &end);
  if (end == begin || !std::isfinite(value))
    return false;

  while (*end != '\0') {
    const unsigned char current = static_cast<unsigned char>(*end);
    if (std::isspace(current) || std::isalpha(current) || *end == '%' || *end == '/' ||
        *end == '-' || *end == '_') {
      ++end;
      continue;
    }
    return false;
  }

  parsed_value = value;
  return true;
}

bool SynthSlider::tryConvertDisplayToInternalValue(double display_value, double& internal_value) const {
  if (!std::isfinite(display_value))
    return false;

  if (string_lookup_) {
    internal_value = display_value;
    return true;
  }

  return mopo::tryDisplayToInternal(details_, display_value, internal_value);
}

bool SynthSlider::tryConvertInternalToDisplayValue(double internal_value, double& display_value) const {
  if (!std::isfinite(internal_value))
    return false;

  if (string_lookup_) {
    display_value = std::round(internal_value);
    return true;
  }

  return mopo::tryInternalToDisplay(details_, internal_value, display_value);
}

String SynthSlider::getAcceptedRangeText() const {
  const double minimum = getMinimum();
  const double maximum = getMaximum();

  if (string_lookup_)
    return "Accepted range: " + String(static_cast<int>(minimum)) + " to " +
           String(static_cast<int>(maximum));

  double display_min = 0.0;
  double display_max = 0.0;
  if (!tryConvertInternalToDisplayValue(minimum, display_min) ||
      !tryConvertInternalToDisplayValue(maximum, display_max)) {
    display_min = minimum;
    display_max = maximum;
  }

  if (display_min > display_max)
    std::swap(display_min, display_max);

  const String units = getUnits().trim();
  String range = "Accepted range: " + String(display_min, 4) + " to " + String(display_max, 4);
  if (units.isNotEmpty())
    range += " " + units;

  return range;
}

String SynthSlider::getSliderDisplayName() const {
  const std::string name = getName().toStdString();
  if (mopo::Parameters::isParameter(name))
    return String(mopo::Parameters::getDetails(name).display_name);
  return getName();
}

double SynthSlider::clampAndSnapToRange(double value) const {
  if (!std::isfinite(value))
    value = getDoubleClickReturnValue();

  value = jlimit(getMinimum(), getMaximum(), value);

  const double interval = getInterval();
  if (interval > 0.0) {
    value = getMinimum() + std::round((value - getMinimum()) / interval) * interval;
    value = jlimit(getMinimum(), getMaximum(), value);
  }

  return value;
}

void SynthSlider::notifyTooltip() {
  if (parent_ == nullptr)
    parent_ = findParentComponentOfClass<FullInterface>();
  if (parent_) {
    std::string name = getName().toStdString();
    if (mopo::Parameters::isParameter(name))
      name = mopo::Parameters::getDetails(name).display_name;

    parent_->setToolTipText(name, getTextFromValue(getValue()));
  }
}
