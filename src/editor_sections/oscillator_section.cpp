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

#include "oscillator_section.h"

#include "colors.h"
#include "fonts.h"
#include "synth_button.h"
#include "synth_slider.h"
#include "tempo_selector.h"
#include "text_look_and_feel.h"

#define WAVE_RESOLUTION 256
#define CROSS_MOD_WIDTH_PERCENT 0.2
#define KNOB_SECTION_HEIGHT 75
#define TEXT_WIDTH 56
#define TEXT_HEIGHT 16
#define TRANS_WIDTH 42
#define WAVE_SELECTOR_HEIGHT 20
#define PHASE_SLIDER_HEIGHT 10

OscillatorSection::OscillatorSection(String name) : SynthSection(name) {
  static const int UNISON_DRAG_SENSITIVITY = 100;
  static const int TRANSPOSE_MOUSE_SENSITIVITY = 800;

  addSlider((wave_selector_1_ = std::make_unique<WaveSelector>(
      "osc_1_waveform", WaveSelectorKind::Oscillator)).get());
  wave_selector_1_->setSliderStyle(Slider::LinearBar);
  wave_selector_1_->setStringLookup(mopo::strings::oscillator_waveforms);
  wave_selector_1_->setPopupPlacement(BubbleComponent::above);

  addSlider((wave_selector_2_ = std::make_unique<WaveSelector>(
      "osc_2_waveform", WaveSelectorKind::Oscillator)).get());
  wave_selector_2_->setSliderStyle(Slider::LinearBar);
  wave_selector_2_->setStringLookup(mopo::strings::oscillator_waveforms);
  wave_selector_2_->setPopupPlacement(BubbleComponent::above);

  addAndMakeVisible((wave_viewer_1_ = std::make_unique<WaveViewer>(WAVE_RESOLUTION)).get());
  wave_viewer_1_->setWaveSlider(wave_selector_1_.get());
  addAndMakeVisible((wave_viewer_2_ = std::make_unique<WaveViewer>(WAVE_RESOLUTION)).get());
  wave_viewer_2_->setWaveSlider(wave_selector_2_.get());

  addSlider((cross_modulation_ = std::make_unique<SynthSlider>("cross_modulation")).get());
  cross_modulation_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addSlider((fm_amount_ = std::make_unique<SynthSlider>("FM_amount")).get());
  fm_amount_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addSlider((ring_mod_ = std::make_unique<SynthSlider>("ring_modulation")).get());
  ring_mod_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addSlider((phase_stretch_1_ = std::make_unique<SynthSlider>("osc_1_phase_stretch")).get());
  phase_stretch_1_->setSliderStyle(Slider::LinearBar);
  phase_stretch_1_->setTooltip(TRANS("Phase stretch (PS)"));

  addSlider((phase_stretch_2_ = std::make_unique<SynthSlider>("osc_2_phase_stretch")).get());
  phase_stretch_2_->setSliderStyle(Slider::LinearBar);
  phase_stretch_2_->setTooltip(TRANS("Phase stretch (PS)"));

  addSlider((phase_offset_1_ = std::make_unique<SynthSlider>("osc_1_phase_offset")).get());
  phase_offset_1_->setSliderStyle(Slider::LinearBar);
  phase_offset_1_->setTooltip(TRANS("Phase offset (PH)"));
  addSlider((phase_offset_2_ = std::make_unique<SynthSlider>("osc_2_phase_offset")).get());
  phase_offset_2_->setSliderStyle(Slider::LinearBar);
  phase_offset_2_->setTooltip(TRANS("Phase offset (PH)"));

  wave_viewer_1_->setPhaseStretchSlider(phase_stretch_1_.get());
  wave_viewer_2_->setPhaseStretchSlider(phase_stretch_2_.get());
  wave_viewer_1_->setPhaseOffsetSlider(phase_offset_1_.get());
  wave_viewer_2_->setPhaseOffsetSlider(phase_offset_2_.get());

  addSlider((transpose_1_ = std::make_unique<SynthSlider>("osc_1_transpose")).get());
  transpose_1_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  transpose_1_->setBipolar();
  transpose_1_->setMouseDragSensitivity(TRANSPOSE_MOUSE_SENSITIVITY);

  addSlider((transpose_2_ = std::make_unique<SynthSlider>("osc_2_transpose")).get());
  transpose_2_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  transpose_2_->setBipolar();
  transpose_2_->setMouseDragSensitivity(TRANSPOSE_MOUSE_SENSITIVITY);

  addSlider((tune_1_ = std::make_unique<SynthSlider>("osc_1_tune")).get());
  tune_1_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  tune_1_->setBipolar();

  addSlider((tune_2_ = std::make_unique<SynthSlider>("osc_2_tune")).get());
  tune_2_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  tune_2_->setBipolar();

  addSlider((unison_detune_1_ = std::make_unique<SynthSlider>("osc_1_unison_detune")).get());
  unison_detune_1_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  unison_detune_1_->setLookAndFeel(TextLookAndFeel::instance());

  addSlider((unison_detune_2_ = std::make_unique<SynthSlider>("osc_2_unison_detune")).get());
  unison_detune_2_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  unison_detune_2_->setLookAndFeel(TextLookAndFeel::instance());

  addSlider((unison_voices_1_ = std::make_unique<SynthSlider>("osc_1_unison_voices")).get());
  unison_voices_1_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  unison_voices_1_->setLookAndFeel(TextLookAndFeel::instance());
  unison_voices_1_->setMouseDragSensitivity(UNISON_DRAG_SENSITIVITY);

  addSlider((unison_voices_2_ = std::make_unique<SynthSlider>("osc_2_unison_voices")).get());
  unison_voices_2_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  unison_voices_2_->setLookAndFeel(TextLookAndFeel::instance());
  unison_voices_2_->setMouseDragSensitivity(UNISON_DRAG_SENSITIVITY);

  addButton((unison_harmonize_1_ = std::make_unique<SynthButton>("unison_1_harmonize")).get());
  unison_harmonize_1_->setLookAndFeel(TextLookAndFeel::instance());
  unison_harmonize_1_->setButtonText("H");

  addButton((unison_harmonize_2_ = std::make_unique<SynthButton>("unison_2_harmonize")).get());
  unison_harmonize_2_->setLookAndFeel(TextLookAndFeel::instance());
  unison_harmonize_2_->setButtonText("H");

  addButton((hard_sync_ = std::make_unique<SynthButton>("osc_hard_sync")).get());
  hard_sync_->setLookAndFeel(TextLookAndFeel::instance());
  hard_sync_->setButtonText(TRANS("Hard Sync"));
  hard_sync_->setTooltip(TRANS("Hard Sync"));
}

/**
 * @file oscillator_section.cpp
 * @brief Implementation of the oscillator UI section.
 */

OscillatorSection::~OscillatorSection() {
  wave_viewer_1_ = nullptr;
  wave_viewer_2_ = nullptr;
  wave_selector_1_ = nullptr;
  wave_selector_2_ = nullptr;
  transpose_1_ = nullptr;
  transpose_2_ = nullptr;
  tune_1_ = nullptr;
  tune_2_ = nullptr;
  unison_voices_1_ = nullptr;
  unison_voices_2_ = nullptr;
  unison_detune_1_ = nullptr;
  unison_detune_2_ = nullptr;
  unison_harmonize_1_ = nullptr;
  unison_harmonize_2_ = nullptr;
  hard_sync_ = nullptr;
  cross_modulation_ = nullptr;
  fm_amount_ = nullptr;
  ring_mod_ = nullptr;
  phase_stretch_1_ = nullptr;
  phase_stretch_2_ = nullptr;
  phase_offset_1_ = nullptr;
  phase_offset_2_ = nullptr;
}

void OscillatorSection::paintBackground(Graphics& g) {
  static const DropShadow component_shadow(Colour(Colors::Color_99000000), 4, Point<int>(0, 0));
  float extra_knob_padding = size_ratio_ * 4.0f;

  SynthSection::paintBackground(g);

  g.setColour(Colors::background);
  g.fillEllipse(transpose_1_->getBounds().toFloat().expanded(extra_knob_padding));
  g.fillEllipse(tune_1_->getBounds().toFloat().expanded(extra_knob_padding));
  g.fillEllipse(transpose_2_->getBounds().toFloat().expanded(extra_knob_padding));
  g.fillEllipse(tune_2_->getBounds().toFloat().expanded(extra_knob_padding));

  g.setColour(Colour(Colors::Color_ff303030));
  g.fillRect(0.0f, 1.0f * tune_1_->getBottom() + size_ratio_ * 2.0f,
             1.0f * getWidth(), size_ratio_ * 5.0f);

  g.setColour(Colour(Colors::Color_ff4fc3f7));
  g.strokePath(top_left_cross_path_, PathStrokeType(1.0f));

  g.setColour(Colour(Colors::Color_ff4fc3f7));
  g.strokePath(top_right_cross_path_, PathStrokeType(1.0f));

  g.setColour(Colour(Colors::Color_ff4fc3f7));
  g.strokePath(bottom_left_cross_path_, PathStrokeType(1.0f));

  g.setColour(Colour(Colors::Color_ff4fc3f7));
  g.strokePath(bottom_right_cross_path_, PathStrokeType(1.0f));

  g.setColour(Colors::control_label_text);
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 10.0f));

  drawTextForComponent(g, TRANS("MOD"), cross_modulation_.get());
  drawTextForComponent(g,TRANS("FM"), fm_amount_.get());
  drawTextForComponent(g, TRANS("RM"), ring_mod_.get());
  drawTextForComponent(g, TRANS("TRANS"), transpose_1_.get());
  drawTextForComponent(g, TRANS("TRANS"), transpose_2_.get());
  drawTextForComponent(g, TRANS("TUNE"), tune_1_.get());
  drawTextForComponent(g, TRANS("TUNE"), tune_2_.get());
  drawTextForComponent(g, TRANS("UNISON"), unison_detune_1_.get());
  drawTextForComponent(g, TRANS("UNISON"), unison_detune_2_.get());
  drawTextForComponent(g, TRANS("PS"), phase_stretch_1_.get(), 0);
  drawTextForComponent(g, TRANS("PS"), phase_stretch_2_.get(), 0);
  drawTextForComponent(g, TRANS("PH"), phase_offset_1_.get(), 0);
  drawTextForComponent(g, TRANS("PH"), phase_offset_2_.get(), 0);

  component_shadow.drawForRectangle(g, wave_viewer_1_->getBounds());
  component_shadow.drawForRectangle(g, wave_viewer_2_->getBounds());

  g.setColour(Colour(Colors::Color_ff424242));
  paintKnobShadows(g);
}

void OscillatorSection::resized() {
  int title_width = getTitleWidth();
  int knob_width = getStandardKnobSize();
  int tune_width = getSmallKnobSize();
  int text_width = size_ratio_ * TEXT_WIDTH;
  int text_height = size_ratio_ * TEXT_HEIGHT;
  int trans_width = size_ratio_ * TRANS_WIDTH;
  int wave_selector_height = size_ratio_ * WAVE_SELECTOR_HEIGHT;
  int knob_section_height = size_ratio_ * KNOB_SECTION_HEIGHT;
  int phase_slider_height = size_ratio_ * PHASE_SLIDER_HEIGHT;
  int phase_label_height = size_ratio_ * 10;
  if (phase_slider_height < 1)
    phase_slider_height = 1;
  if (phase_label_height < 1)
    phase_label_height = 1;
  int phase_footer_height = phase_slider_height + phase_label_height;

  float cross_mod_width = CROSS_MOD_WIDTH_PERCENT * getWidth();
  float osc_width = (getWidth() - cross_mod_width) / 2.0f;
  float osc_height = getHeight() - title_width - wave_selector_height - knob_section_height;
  float viewer_height = osc_height - phase_footer_height;
  if (viewer_height < 0.0f)
    viewer_height = 0.0f;
  float osc_y = title_width + wave_selector_height;

  wave_selector_1_->setBounds(0.0f, title_width, osc_width, wave_selector_height);
  wave_selector_2_->setBounds(getWidth() - osc_width, title_width, osc_width, wave_selector_height);
  wave_viewer_1_->setBounds(0.0f, osc_y, osc_width, viewer_height);
  wave_viewer_2_->setBounds(getWidth() - osc_width, osc_y, osc_width, viewer_height);

  cross_modulation_->setBounds((getWidth() - knob_width) / 2.0f,
                               osc_y + (viewer_height - knob_width) / 2.0f,
                               knob_width, knob_width);
  hard_sync_->setBounds((getWidth() - cross_mod_width) / 2.0f,
                        cross_modulation_->getBottom() + size_ratio_ * 16.0f,
                        cross_mod_width, size_ratio_ * TEXT_HEIGHT);

  float space = (getWidth() - (2.0f * text_width + 2.0f * trans_width + 2.0f * tune_width)) / 5.0f;
  int knob_bottom = getHeight() - size_ratio_ * 22.0f;

  tune_1_->setBounds(space * 1.75f, knob_bottom - tune_width, tune_width, tune_width);

  transpose_1_->setBounds(tune_width + space * 1.75f, knob_bottom - trans_width,
                          trans_width, trans_width);

  unison_detune_1_->setBounds(trans_width + tune_width + 2.45f * space,
                              knob_bottom - text_height,
                              text_width, text_height);

  unison_voices_1_->setBounds(trans_width + tune_width + 2.45f * space,
                              knob_bottom - 2 * text_height,
                              text_width - text_height, text_height);

  unison_harmonize_1_->setBounds(unison_voices_1_->getX() + unison_voices_1_->getWidth(),
                                 unison_voices_1_->getY(),
                                 text_height, text_height);

  unison_detune_2_->setBounds(trans_width + text_width + tune_width + 2.55f * space,
                              knob_bottom - text_height,
                              text_width, text_height);

  unison_voices_2_->setBounds(trans_width + text_width + tune_width + 2.55f * space,
                              knob_bottom - 2 * text_height,
                              text_width - text_height, text_height);

  unison_harmonize_2_->setBounds(unison_voices_2_->getX() + unison_voices_2_->getWidth(),
                                 unison_voices_2_->getY(),
                                 text_height, text_height);

  transpose_2_->setBounds(trans_width + 2 * text_width + tune_width + 3.25f * space,
                          knob_bottom - trans_width, trans_width, trans_width);

  tune_2_->setBounds(2 * trans_width + 2 * text_width + tune_width + 3.25f * space,
                     knob_bottom - tune_width, tune_width, tune_width);

  fm_amount_->setBounds((getWidth() - (knob_width / 2.0f)),
                         getHeight() - knob_width*.85f,
                         knob_width / 2.0f, knob_width / 2.0f);

  ring_mod_->setBounds(0.0f,
                        getHeight() - knob_width*.85f,
                        knob_width / 2.0f, knob_width / 2.0f);

  const int phase_slider_gap = size_ratio_ * 4;
  const float phase_slider_width = (osc_width - phase_slider_gap) / 2.0f;
  const float phase_slider_y = osc_y + viewer_height;
  phase_offset_1_->setBounds(0.0f, phase_slider_y, phase_slider_width, phase_slider_height);
  phase_stretch_1_->setBounds(phase_slider_width + phase_slider_gap, phase_slider_y,
                              phase_slider_width, phase_slider_height);
  phase_offset_2_->setBounds(getWidth() - osc_width, phase_slider_y,
                             phase_slider_width, phase_slider_height);
  phase_stretch_2_->setBounds(getWidth() - osc_width + phase_slider_width + phase_slider_gap,
                              phase_slider_y, phase_slider_width, phase_slider_height);

  float cross_x_padding = size_ratio_ * 8.0f;
  float cross_y_padding = size_ratio_ * 8.0f;
  float cross_width = cross_mod_width - 2.0f * cross_x_padding;
  float cross_height = viewer_height - 2.0f * cross_y_padding;
  float cross_percent = 0.2f;

  top_left_cross_path_.clear();
  top_left_cross_path_.startNewSubPath(osc_width, osc_y + cross_y_padding);
  top_left_cross_path_.lineTo(osc_width + cross_x_padding, osc_y + cross_y_padding);
  top_left_cross_path_.lineTo(osc_width + cross_x_padding + cross_percent * cross_width,
                              osc_y + cross_y_padding + cross_percent * cross_height);

  top_right_cross_path_.clear();
  top_right_cross_path_.startNewSubPath(getWidth() - osc_width, osc_y + cross_y_padding);
  top_right_cross_path_.lineTo(getWidth() - osc_width - cross_x_padding, osc_y + cross_y_padding);
  top_right_cross_path_.lineTo(getWidth() - osc_width - cross_x_padding -
                               cross_percent * cross_width,
                               osc_y + cross_y_padding + cross_percent * cross_height);

  bottom_left_cross_path_.clear();
  bottom_left_cross_path_.startNewSubPath(osc_width, osc_y + viewer_height - cross_y_padding);
  bottom_left_cross_path_.lineTo(osc_width + cross_x_padding, osc_y + viewer_height - cross_y_padding);
  bottom_left_cross_path_.lineTo(osc_width + cross_x_padding + cross_percent * cross_width,
                                 osc_y + viewer_height - cross_y_padding -
                                 cross_percent * cross_height);

  bottom_right_cross_path_.clear();
  bottom_right_cross_path_.startNewSubPath(getWidth() - osc_width,
                                           osc_y + viewer_height - cross_y_padding);
  bottom_right_cross_path_.lineTo(getWidth() - osc_width - cross_x_padding,
                                  osc_y + viewer_height - cross_y_padding);
  bottom_right_cross_path_.lineTo(getWidth() - osc_width - cross_x_padding -
                                  cross_percent * cross_width,
                                  osc_y + viewer_height - cross_y_padding -
                                  cross_percent * cross_height);

  SynthSection::resized();

  unison_detune_1_->setPopupDisplayEnabled(false, false, nullptr);
  unison_detune_2_->setPopupDisplayEnabled(false, false, nullptr);
  unison_voices_1_->setPopupDisplayEnabled(false, false, nullptr);
  unison_voices_2_->setPopupDisplayEnabled(false, false, nullptr);
}

void OscillatorSection::reset() {
  wave_viewer_1_->resetWavePath();
  wave_viewer_1_->repaint();
  wave_viewer_2_->resetWavePath();
  wave_viewer_2_->repaint();
  SynthSection::reset();
}
