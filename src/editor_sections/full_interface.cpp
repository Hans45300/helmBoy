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

#include "full_interface.h"

#include "colors.h"
#include "fonts.h"
#include "helmBoy_engine.h"
#include "helmBoy_common.h"
#include "../editor_components/HelmBoyGraphics.h"
#include "load_save.h"
#include "synth_gui_interface.h"
#include "text_look_and_feel.h"

#define TOP_HEIGHT 64

#ifndef PAY_NAG
  #define PAY_NAG 1
#endif

FullInterface::FullInterface(SynthBase* synth, mopo::control_map controls, mopo::output_map modulation_sources,
                             mopo::output_map mono_modulations,
                             mopo::output_map poly_modulations,
                             MidiKeyboardState* keyboard_state) : SynthSection("full_interface") {
  animate_ = true;
  addSubSection((synthesis_interface_ = std::make_unique<SynthesisInterface>(controls, keyboard_state)).get());
  addSubSection((arp_section_ = std::make_unique<ArpSection>(TRANS("ARP"))).get());
  addSubSection((bpm_section_ = std::make_unique<BpmSection>(TRANS("BPM"))).get());
  addSubSection((patch_selector_ = std::make_unique<PatchSelector>()).get());
  addAndMakeVisible((global_tool_tip_ = std::make_unique<GlobalToolTip>()).get());
  addSubSection((volume_section_ = std::make_unique<VolumeSection>("VOLUME")).get());
  addSubSection((limiter_section_ = std::make_unique<LimiterSection>("LIMITER")).get());
  addAndMakeVisible((oscilloscope_ = std::make_unique<Oscilloscope>()).get());
  setAllValues(controls);
  createModulationSliders(modulation_sources, mono_modulations, poly_modulations, synth);

  logo_graphics_ = std::make_unique<HelmBoyGraphics>();
  logo_graphics_->setIdentity(HelmBoyGraphics::Identity::logo);
  // Configure les couleurs selon le th�me Helm
  logo_graphics_->onClick = [this]() {
    about_section_->setVisible(true);
  };
  addAndMakeVisible(logo_graphics_.get());
  addChildComponent((patch_browser_ = std::make_unique<PatchBrowser>()).get());
  patch_selector_->setBrowser(patch_browser_.get());
  // Link the selector back to the browser so the browser can notify it
  patch_browser_->setSelector(patch_selector_.get());
  addChildComponent((save_section_ = std::make_unique<SaveSection>("save_section")).get());
  patch_browser_->setSaveSection(save_section_.get());
  patch_selector_->setSaveSection(save_section_.get());
  addChildComponent((delete_section_ = std::make_unique<DeleteSection>("delete_section")).get());
  patch_browser_->setDeleteSection(delete_section_.get());
  addChildComponent((about_section_ = std::make_unique<AboutSection>("about")).get());
  synthesis_interface_->toFront(true);
  modulation_manager_->toFront(false);
  patch_browser_->toFront(false);
  about_section_->toFront(false);
  save_section_->toFront(false);
  delete_section_->toFront(false);

  setOpaque(false);
}

/**
 * @file full_interface.cpp
 * @brief Implementation of the top-level FullInterface UI component.
 */

FullInterface::~FullInterface() {
  modulation_manager_ = nullptr;
  about_section_ = nullptr;
  arp_section_ = nullptr;
  oscilloscope_ = nullptr;
  synthesis_interface_ = nullptr;
  bpm_section_ = nullptr;
  global_tool_tip_ = nullptr;
  limiter_section_ = nullptr;
  patch_selector_ = nullptr;
  save_section_ = nullptr;
  delete_section_ = nullptr;
  volume_section_ = nullptr;
}

void FullInterface::paint(Graphics& g) {
  if (layout_ready_ && background_image_.isValid())
    g.drawImage(background_image_, getLocalBounds().toFloat());
  else
    g.fillAll(Colors::background);
}

void FullInterface::paintBackground(Graphics& g) {
  static const DropShadow shadow(Colors::shadow_dark, 3, Point<int>(0, 1));
  static const DropShadow component_shadow(Colors::shadow_dark, 5, Point<int>(0, 1));

  g.setColour(Colors::background);
  g.fillRect(getLocalBounds());  shadow.drawForRectangle(g, arp_section_->getBounds());
  shadow.drawForRectangle(g, oscilloscope_->getBounds());
  shadow.drawForRectangle(g, limiter_section_->getBounds());
  shadow.drawForRectangle(g, patch_selector_->getBounds());

  int logo_padding = 2 * size_ratio_;
  int x = logo_graphics_->getX() - logo_padding;
  int width = logo_graphics_->getWidth() + 2 * logo_padding;


  shadow.drawForRectangle(g, Rectangle<int>(x, logo_graphics_->getY(),
                                            width, logo_graphics_->getHeight()));

  //g.setColour(Colors::tab_heading);
  //g.fillRoundedRectangle(x, logo_graphics_->getY(), width, logo_graphics_->getHeight(), 3.0f);

  component_shadow.drawForRectangle(g, patch_selector_->getBounds());
  component_shadow.drawForRectangle(g, volume_section_->getBounds());

  paintKnobShadows(g);
  paintChildrenBackgrounds(g);
}

void FullInterface::resized() {
  layout_ready_ = false;
  if (getWidth() <= 0 || getHeight() <= 0)
    return;

  int left = 0;
  int width = getWidth();
  int height = getHeight();
  float ratio = 1.0f;
  float width_ratio = getWidth() / (1.0f * mopo::DEFAULT_WINDOW_WIDTH);
  float height_ratio = getHeight() / (1.0f * mopo::DEFAULT_WINDOW_HEIGHT);
  if (width_ratio > height_ratio) {
    ratio = height_ratio;
    width = height_ratio * mopo::DEFAULT_WINDOW_WIDTH;
    left = (getWidth() - width) / 2;
  }
  else {
    ratio = width_ratio;
    height = width_ratio * mopo::DEFAULT_WINDOW_HEIGHT;
  }

  setSizeRatio(ratio);
  save_section_->setSizeRatio(ratio);
  delete_section_->setSizeRatio(ratio);
  patch_browser_->setSizeRatio(ratio);
  about_section_->setSizeRatio(ratio);
  int padding = 8 * ratio;
  int top_height = TOP_HEIGHT * ratio;

  int section_one_width = 320 * ratio;
  int section_two_width = section_one_width;
  int section_four_width = section_one_width;
  int section_three_width = width - section_one_width - section_two_width - section_four_width - 5 * padding;
  int section_three_left_width = (section_three_width - padding) / 2;
  int limiter_width = section_three_width - padding - section_three_left_width;

  synthesis_interface_->setPadding(padding);
  synthesis_interface_->setSectionOneWidth(section_one_width);
  synthesis_interface_->setSectionTwoWidth(section_two_width);
  synthesis_interface_->setSectionThreeWidth(section_three_width);
  synthesis_interface_->setSectionFourWidth(section_four_width);

  int logo_padding = 2 * ratio;
  int logo_width = top_height + 2 * logo_padding;

  int patch_selector_width = section_one_width - logo_width - padding;

  logo_graphics_->setBounds(left + padding + logo_padding, padding, top_height, top_height);
  patch_selector_->setBounds(logo_graphics_->getRight() + padding + logo_padding, padding,
                             patch_selector_width, top_height);
  global_tool_tip_->setBounds(patch_selector_->getX() + 0.11 * patch_selector_->getWidth(),
                              patch_selector_->getY(),
                              0.78 * patch_selector_->getWidth(),
                              patch_selector_->getBrowseHeight());

  int volume_width = (section_two_width - padding) / 2;
  int bpm_width = 40 * ratio;
  int oscilloscope_width = section_two_width + section_three_width - volume_width - limiter_width - padding;

  volume_section_->setBounds(patch_selector_->getRight() + padding, padding,
                             volume_width, top_height);

  oscilloscope_->setBounds(volume_section_->getRight() + padding, padding,
                           oscilloscope_width, top_height);
  limiter_section_->setBounds(oscilloscope_->getRight() + padding, padding,
                              limiter_width, top_height);

  // BPM aligné à gauche de la colonne 5
  bpm_section_->setBounds(limiter_section_->getRight() + padding, padding,
                          bpm_width, top_height);

  // Arp juste à droite du BPM
  int arp_width = section_four_width - bpm_width - padding;
  arp_section_->setBounds(bpm_section_->getRight() + padding, padding,
                          arp_width, top_height);

  synthesis_interface_->setBounds(left, top_height + padding,
                                  width, height - top_height - padding);

  about_section_->setBounds(getBounds());
  save_section_->setBounds(getBounds());
  delete_section_->setBounds(getBounds());

  patch_browser_->setBounds(synthesis_interface_->getX() + padding, synthesis_interface_->getY(),
                            arp_section_->getRight() - synthesis_interface_->getX() - padding,
                            synthesis_interface_->getHeight() - padding);

  SynthSection::resized();
  modulation_manager_->setBounds(getBounds());

  checkBackground();
  layout_ready_ = true;
}

void FullInterface::moved() {
  checkBackground();
}

void FullInterface::setOutputMemory(const float* output_memory) {
  oscilloscope_->setOutputMemory(output_memory);
}

void FullInterface::setOutputMemorySource(SynthBase* synth) {
  oscilloscope_->setOutputMemorySource(synth);
  oscilloscope_->showRealtimeFeedback(animate_.load(std::memory_order_relaxed));
}

void FullInterface::createModulationSliders(mopo::output_map modulation_sources,
                                            mopo::output_map mono_modulations,
                                            mopo::output_map poly_modulations,
                                            SynthBase* synth) {
  std::map<std::string, SynthSlider*> all_sliders = getAllSliders();
  std::map<std::string, Button*> all_buttons = getAllButtons();
  std::map<std::string, SynthSlider*> modulatable_sliders;
  std::map<std::string, SynthButton*> modulatable_buttons;

  auto addDestinationIfModulatable = [&](const std::string& destination_name) {
    if (all_sliders.count(destination_name))
      modulatable_sliders[destination_name] = all_sliders[destination_name];

    if (all_buttons.count(destination_name)) {
      SynthButton* synth_button = dynamic_cast<SynthButton*>(all_buttons[destination_name]);
      if (synth_button)
        modulatable_buttons[destination_name] = synth_button;
    }
  };

  for (auto& destination : mono_modulations)
    addDestinationIfModulatable(destination.first);

  for (auto& destination : poly_modulations)
    addDestinationIfModulatable(destination.first);

  modulation_manager_ = std::make_unique<ModulationManager>(synth, modulation_sources,
                                                    getAllModulationButtons(),
                                                    modulatable_sliders,
                                                    modulatable_buttons,
                                                    mono_modulations, poly_modulations);
  modulation_manager_->setOpaque(false);
  addAndMakeVisible(modulation_manager_.get());
}

void FullInterface::setToolTipText(String parameter, String value) {
  if (global_tool_tip_)
    global_tool_tip_->setText(parameter, value);
}

void FullInterface::setUndoRedoAvailability(bool can_undo, bool can_redo) {
  if (patch_selector_ != nullptr)
    patch_selector_->setUndoRedoAvailability(can_undo, can_redo);
}

void FullInterface::buttonClicked(Button* clicked_button) {
  SynthSection::buttonClicked(clicked_button);
}

void FullInterface::animate(bool animate) {
  animate_ = animate;
  SynthSection::animate(animate);
  if (oscilloscope_)
    oscilloscope_->showRealtimeFeedback(animate);
  repaint();
}

void FullInterface::checkBackground() {
  if (getWidth() <= 0 || getHeight() <= 0)
    return;

  auto* display = Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds());
  float scale = display != nullptr ? static_cast<float>(display->scale) : 1.0f;
  int width = scale * getWidth();
  int height = scale * getHeight();

  if (background_image_.getWidth() != width || background_image_.getHeight() != height) {
    background_image_ = Image(Image::ARGB, width, height, true);
    {
      Graphics g(background_image_);
      g.addTransform(AffineTransform::scale(scale, scale));
      paintBackground(g);
    }
  }
}

void FullInterface::notifyFresh() {
  global_tool_tip_->setVisible(false);
  patch_selector_->setModified(false);
}
