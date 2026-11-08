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

#include "about_section.h"
#include "../editor_components/HelmBoyGraphics.h"
#include "colors.h"
#include "fonts.h"
#include "helmBoy_common.h"
#include "load_save.h"
#include "midi_2_manager.h"
#include "synth_gui_interface.h"
#include "synth_section.h"
#include "text_look_and_feel.h"

#define LOGO_WIDTH 128
#define INFO_WIDTH 470
#define STANDALONE_INFO_HEIGHT 650
#define PLUGIN_INFO_HEIGHT 280
#define PADDING_X 25
#define PADDING_Y 15
#define BUTTON_WIDTH 16
#define MIDI_PROFILE_BUTTON_WIDTH 50
#define MIDI_PROFILE_BUTTON_HEIGHT 24

#define MULT_SMALL 0.75f
#define MULT_LARGE 1.35f
#define MULT_EXTRA_LARGE 2.0f
#define MULT_OVER_LARGE 2.5f

AboutSection::AboutSection(String name) : Overlay(name) {
  developer_link_ = std::make_unique<HyperlinkButton>("Marc Scheffer", URL("https://github.com/Hans45/HelmBoy"));
  developer_link_->setFont(Fonts::instance()->proportional_light().withPointHeight(16.0f),
                           false, Justification::right);
  developer_link_->setColour(HyperlinkButton::textColourId, Colour(Colors::Color_ffffd740));
  addAndMakeVisible(developer_link_.get());

  free_software_link_ = std::make_unique<HyperlinkButton>(TRANS("Read more about free software"),
                                            URL("http://www.gnu.org/philosophy/free-sw.html"));
  free_software_link_->setFont(Fonts::instance()->proportional_light().withPointHeight(12.0f),
                               false, Justification::right);
  free_software_link_->setColour(HyperlinkButton::textColourId, Colour(Colors::Color_ffffd740));
  addAndMakeVisible(free_software_link_.get());

  enable_midi_2_ = std::make_unique<ToggleButton>();
  enable_midi_2_->setTooltip(TRANS("Enables explicit MIDI 2.0 UMP decoding; native host UMP input is not connected yet."));
  enable_midi_2_->setToggleState(LoadSave::shouldEnableMidi2(),
                                     NotificationType::dontSendNotification);
  enable_midi_2_->setLookAndFeel(TextLookAndFeel::instance());
  enable_midi_2_->addListener(this);
  addAndMakeVisible(enable_midi_2_.get());

  animate_ = std::make_unique<ToggleButton>();
  animate_->setToggleState(LoadSave::shouldAnimateWidgets(),
                           NotificationType::dontSendNotification);
  animate_->setLookAndFeel(TextLookAndFeel::instance());
  animate_->addListener(this);
  addAndMakeVisible(animate_.get());

  size_button_small_ = std::make_unique<TextButton>(String(100 * MULT_SMALL) + "%");
  addAndMakeVisible(size_button_small_.get());
  size_button_small_->addListener(this);

  size_button_normal_ = std::make_unique<TextButton>(String("100") + "%");
  addAndMakeVisible(size_button_normal_.get());
  size_button_normal_->addListener(this);

  size_button_large_ = std::make_unique<TextButton>(String(100 * MULT_LARGE) + "%");
  addAndMakeVisible(size_button_large_.get());
  size_button_large_->addListener(this);

  size_button_extra_large_ = std::make_unique<TextButton>(String(100 * MULT_EXTRA_LARGE) + "%");
  addAndMakeVisible(size_button_extra_large_.get());
  size_button_extra_large_->addListener(this);

  size_button_over_large_ = std::make_unique<TextButton>(String(100 * MULT_OVER_LARGE) + "%");
  addAndMakeVisible(size_button_over_large_.get());
  size_button_over_large_->addListener(this);

  save_midi_profile_ = std::make_unique<TextButton>(TRANS("Save"));
  addAndMakeVisible(save_midi_profile_.get());
  save_midi_profile_->addListener(this);

  load_midi_profile_ = std::make_unique<TextButton>(TRANS("Load"));
  addAndMakeVisible(load_midi_profile_.get());
  load_midi_profile_->addListener(this);

  size_button_over_large_->setLookAndFeel(DefaultLookAndFeel::instance());
}

/**
 * @file about_section.cpp
 * @brief Implementation of the AboutSection overlay UI.
 *
 * Paints and lays out the info overlay containing project links,
 * device selector and UI scaling controls.
 */

void AboutSection::paint(Graphics& g) {
  static const DropShadow shadow(Colour(Colors::Color_ff000000), 5, Point<int>(0, 0));

  g.setColour(Colors::overlay_screen);
  g.fillAll();

  Rectangle<int> info_rect = getInfoRect();
  shadow.drawForRectangle(g, info_rect);
  g.setColour(Colour(Colors::Color_ff303030));
  g.fillRect(info_rect);

  g.saveState();
  g.setOrigin(info_rect.getX() + PADDING_X, info_rect.getY() + PADDING_Y);

  // Utiliser HelmBoyGraphics au lieu d'images BinaryData
  HelmBoyGraphics logo_graphics;
  logo_graphics.setIdentity(HelmBoyGraphics::Identity::logo);
  logo_graphics.setActivated(true);
  logo_graphics.setSelected(true);
  logo_graphics.setForceSquareBounds(true);
  logo_graphics.setBounds(0, 0, 128, 128);
  logo_graphics.paint(g);

  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(32.0));
  g.setColour(Colour(Colors::Color_ff2196f3));
  g.drawText(TRANS("HELMBOY"),
             0.0f, 0.0f,
             info_rect.getWidth() - 2 * PADDING_X, 32.0f, Justification::centredTop);

  g.setFont(Fonts::instance()->proportional_light().withPointHeight(12.0));
  g.setColour(Colour(Colors::Color_ff666666));
  g.drawText(TRANS("v") + " " + ProjectInfo::versionString,
             0.0f, 36.0f,
             info_rect.getWidth() - 2 * PADDING_X, 32.0f, Justification::centredTop);

  g.setFont(Fonts::instance()->proportional_light().withPointHeight(12.0));
  g.drawText(TRANS("Developed by"),
             0.0f, 4.0f,
             info_rect.getWidth() - 2 * PADDING_X, 20.0f, Justification::right);

  g.setColour(Colour(Colors::Color_ffaaaaaa));
  g.drawText(TRANS("HelmBoy is free software and"),
             0.0f, 62.0,
             info_rect.getWidth() - 2 * PADDING_X, 20.0f, Justification::topRight);

  g.drawText(TRANS("comes with no warranty"),
             0.0f, 76.0f,
             info_rect.getWidth() - 2 * PADDING_X, 20.0f, Justification::topRight);

  g.setFont(Fonts::instance()->proportional_light().withPointHeight(12.0));
  g.drawText(TRANS("MIDI 2.0"),
             0.0f, 141.0f,
             info_rect.getWidth() - 2 * PADDING_X - 1.5 * BUTTON_WIDTH,
             20.0f, Justification::topRight);
  g.drawText(TRANS("Animate graphics"),
             0.0f, 141.0f,
             273.0f - PADDING_X - 0.5 * BUTTON_WIDTH,
             20.0f, Justification::topRight);
  g.drawText(TRANS("Window size"),
             0.0f, 180.0f,
             155.0f,
             20.0f, Justification::topRight);
  g.drawText(TRANS("MIDI profile"),
             0.0f, 215.0f,
             155.0f,
             20.0f, Justification::topRight);

  g.restoreState();
}

void AboutSection::resized() {
  static const float software_link_width = 200.0f;
  static const float developer_link_width = 120.0f;

  Rectangle<int> info_rect = getInfoRect();
  developer_link_->setBounds(info_rect.getRight() - PADDING_X - developer_link_width,
                             info_rect.getY() + PADDING_Y + 24.0f, developer_link_width, 20.0f);

  free_software_link_->setBounds(info_rect.getRight() - PADDING_X - software_link_width,
                                 info_rect.getY() + PADDING_Y + 105.0f, software_link_width, 20.0f);

  enable_midi_2_->setBounds(info_rect.getRight() - PADDING_X - BUTTON_WIDTH,
                                info_rect.getY() + PADDING_Y + 140.0f, BUTTON_WIDTH, BUTTON_WIDTH);

  animate_->setBounds(info_rect.getX() + 273.0f,
                      info_rect.getY() + PADDING_Y + 140.0f, BUTTON_WIDTH, BUTTON_WIDTH);

  int size_y = animate_->getBottom() + PADDING_Y;
  int size_height = 2 * BUTTON_WIDTH;
  int size_width = 40;
  int size_padding = 5;
  size_button_over_large_->setBounds(info_rect.getRight() - PADDING_X - size_width, size_y,
                                      size_width, size_height);
  size_button_extra_large_->setBounds(size_button_over_large_->getX() - size_padding - size_width, size_y,
                                      size_width, size_height);
  size_button_large_->setBounds(size_button_extra_large_->getX() - size_padding - size_width,
                                size_y, size_width, size_height);
  size_button_normal_->setBounds(size_button_large_->getX() - size_padding - size_width, size_y,
                                 size_width, size_height);
  size_button_small_->setBounds(size_button_normal_->getX() - size_padding - size_width, size_y,
                                size_width, size_height);

  int midi_profile_y = size_button_over_large_->getBottom() + PADDING_Y;
  save_midi_profile_->setBounds(info_rect.getRight() - PADDING_X - (2 * MIDI_PROFILE_BUTTON_WIDTH) - size_padding,
                                midi_profile_y,
                                MIDI_PROFILE_BUTTON_WIDTH,
                                MIDI_PROFILE_BUTTON_HEIGHT);
  load_midi_profile_->setBounds(save_midi_profile_->getRight() + size_padding,
                                midi_profile_y,
                                MIDI_PROFILE_BUTTON_WIDTH,
                                MIDI_PROFILE_BUTTON_HEIGHT);

  if (device_selector_) {
    int y = load_midi_profile_->getBottom() + PADDING_Y;
    device_selector_->setBounds(info_rect.getX(), y,
                                info_rect.getWidth(), info_rect.getBottom() - y);
  }
}

void AboutSection::mouseUp(const MouseEvent &e) {
  if (!getInfoRect().contains(e.getPosition()))
    setVisible(false);
}

void AboutSection::setVisible(bool should_be_visible) {
  if (should_be_visible && device_selector_.get() == nullptr) {
    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    AudioDeviceManager* device_manager = parent->getAudioDeviceManager();
    if (device_manager) {
        device_selector_ = std::make_unique<AudioDeviceSelectorComponent>(*device_manager, 0, 0,
                                                          mopo::NUM_CHANNELS, mopo::NUM_CHANNELS,
                                                          true, false, false, false);
      device_selector_->setLookAndFeel(TextLookAndFeel::instance());
      addAndMakeVisible(device_selector_.get());
      Rectangle<int> info_rect = getInfoRect();
      int y = info_rect.getY() + LOGO_WIDTH + 2 * PADDING_Y;
      device_selector_->setBounds(info_rect.getX(), y,
                                  info_rect.getWidth(), info_rect.getBottom() - y);
      resized();
    }
  }

  Overlay::setVisible(should_be_visible);
}

void AboutSection::buttonClicked(Button* clicked_button) {
  if (clicked_button == enable_midi_2_.get())
  {
    SynthGuiInterface* gui_interface = findParentComponentOfClass<SynthGuiInterface>();
    if (gui_interface) {
      bool enable = enable_midi_2_->getToggleState();

      LoadSave::saveMidi2Config(enable);

      if (auto* midi2 = gui_interface->getMidi2Manager()) {
        midi2->setMidiVersion(
          enable ? helmboy::Midi2Manager::MidiVersion::MIDI_2_0
                 : helmboy::Midi2Manager::MidiVersion::MIDI_1_0
        );
        DBG("MIDI 2.0 " << (enable ? "enabled" : "disabled"));
      }
    }
  }
  else if (clicked_button == animate_.get())
  {
    LoadSave::saveAnimateWidgets(animate_->getToggleState());

    SynthSection* parent = findParentComponentOfClass<SynthSection>();
    for (SynthSection* s = parent; s; s = parent->findParentComponentOfClass<SynthSection>())
      parent = s;

    parent->animate(animate_->getToggleState());
  }
  else if (clicked_button == size_button_small_.get())
    setGuiSize(MULT_SMALL);
  else if (clicked_button == size_button_normal_.get())
    setGuiSize(1.0);
  else if (clicked_button == size_button_large_.get())
    setGuiSize(MULT_LARGE);
  else if (clicked_button == size_button_extra_large_.get())
    setGuiSize(MULT_EXTRA_LARGE);
  else if (clicked_button == size_button_over_large_.get())
    setGuiSize(MULT_OVER_LARGE);
  else if (clicked_button == save_midi_profile_.get() || clicked_button == load_midi_profile_.get())
  {
    SynthGuiInterface* gui_interface = findParentComponentOfClass<SynthGuiInterface>();
    SynthBase* synth = gui_interface ? gui_interface->getSynth() : nullptr;
    MidiManager* midi_manager = synth ? synth->getMidiManager() : nullptr;
    if (midi_manager == nullptr) {
      AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                                       TRANS("MIDI profile"),
                                       TRANS("Unable to access MIDI mappings."));
      return;
    }

    if (clicked_button == save_midi_profile_.get()) {
      AudioDeviceManager* device_manager = gui_interface ? gui_interface->getAudioDeviceManager() : nullptr;
      auto chooser = std::make_shared<FileChooser>(TRANS("Save MIDI Profile"), File(), "*.hbMidiProfile");
      auto chooser_flags = FileBrowserComponent::saveMode |
                           FileBrowserComponent::canSelectFiles |
                           FileBrowserComponent::warnAboutOverwriting;
      chooser->launchAsync(chooser_flags, [chooser, midi_manager, device_manager](const FileChooser& file_chooser) {
        File selected = file_chooser.getResult();
        if (selected == File())
          return;

        File output_file = selected.withFileExtension(".hbMidiProfile");
        bool success = LoadSave::saveMidiProfileToFile(output_file, midi_manager, device_manager);
        AlertWindow::showMessageBoxAsync(success ? AlertWindow::InfoIcon : AlertWindow::WarningIcon,
                                         TRANS("MIDI profile"),
                                         success ? TRANS("MIDI profile saved.")
                                                 : TRANS("Failed to save MIDI profile."));
      });
    }
    else {
      AudioDeviceManager* device_manager = gui_interface ? gui_interface->getAudioDeviceManager() : nullptr;
      auto chooser = std::make_shared<FileChooser>(TRANS("Load MIDI Profile"), File(), "*.hbMidiProfile");
      auto chooser_flags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;
      chooser->launchAsync(chooser_flags, [gui_interface, chooser, midi_manager, device_manager](const FileChooser& file_chooser) {
        File selected = file_chooser.getResult();
        if (selected == File())
          return;

        bool success = LoadSave::loadMidiProfileFromFile(selected, midi_manager, device_manager);
        if (success && gui_interface)
          gui_interface->updateFullGui();

        AlertWindow::showMessageBoxAsync(success ? AlertWindow::InfoIcon : AlertWindow::WarningIcon,
                                         TRANS("MIDI profile"),
                                         success ? TRANS("MIDI profile loaded.")
                                                 : TRANS("Failed to load MIDI profile."));
      });
    }
  }
}

Rectangle<int> AboutSection::getInfoRect() {
  int info_height = device_selector_ ? STANDALONE_INFO_HEIGHT : PLUGIN_INFO_HEIGHT;
  int x = (getWidth() - INFO_WIDTH) / 2;
  int y = (getHeight() - info_height) / 2;
  return Rectangle<int>(x, y, INFO_WIDTH, info_height);
}

void AboutSection::setGuiSize(float multiplier) {
  float percent = sqrtf(multiplier);
  LoadSave::saveWindowSize(percent);

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent) {
    parent->setGuiSize(percent * mopo::DEFAULT_WINDOW_WIDTH,
                       percent * mopo::DEFAULT_WINDOW_HEIGHT);
  }
}
