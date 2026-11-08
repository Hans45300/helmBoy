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

#pragma once
#ifndef WAVE_VIEWER_H
#define WAVE_VIEWER_H

#include <JuceHeader.h>
#include <array>
#include "wave.h"
#include "helmBoy_common.h"
#include "synth_slider.h"

class SynthBase;

/**
 * @file wave_viewer.h
 * @brief Visualiseur d'onde/waveform pour oscillateurs et sources.
 */

class WaveViewer : public Component, public Timer, public SynthSlider::SliderListener {
  public:
    WaveViewer(int resolution);
    ~WaveViewer();

    void timerCallback() override;
    void setWaveSlider(Slider* slider);
    void setWaveSlider(SynthSlider* slider);
    void setAmplitudeSlider(SynthSlider* slider);
    void setPhaseStretchSlider(SynthSlider* slider);
    void setPhaseOffsetSlider(SynthSlider* slider);
    void drawRandom();
    void drawSmoothRandom();
    void resetWavePath();
    void guiChanged(SynthSlider* slider) override;
    void showRealtimeFeedback(bool show_feedback = true);
    void setControlRate(bool control_rate = true) { is_control_rate_ = control_rate; }

    void paint(Graphics& g) override;
    void paintBackground(Graphics& g);
    void resized() override;
    void moved() override;
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

  private:
    void buildWavePath(Path& path, Path& fill_path, int waveform_index, float amplitude,
               float phase_stretch, float phase_offset);
    void drawRandom(Path& path, Path& fill_path, float amplitude,
            float phase_stretch, float phase_offset);
    void drawSampleAndGlide(Path& path, Path& fill_path, float amplitude, float phase_stretch,
                float phase_offset);
    void drawSmoothRandom(Path& path, Path& fill_path, float amplitude,
                float phase_stretch, float phase_offset);
    float phaseToX(float phase);
    float getRatio();
    float getBaseAmplitude() const;
    int getBaseWaveformIndex() const;
    float getBasePhaseStretch() const;
    float getBasePhaseOffset() const;
    float getAmplitude() const;
    float getPhaseStretch() const;
    float getPhaseOffset() const;
    bool phaseStretchAffectsWave() const;
      void resolveRealtimeTelemetry();
      bool updateRandomPreview();
    float displayed_phase_stretch_ = 0.5f;
    float displayed_phase_offset_ = 0.0f;
    float displayed_amplitude_ = 1.0f;
    int displayed_waveform_index_ = 0;
    int modulated_waveform_index_ = 0;
    float modulated_phase_stretch_ = 0.5f;
    float modulated_phase_offset_ = 0.0f;
    float modulated_amplitude_ = 1.0f;
    bool has_modulated_phase_stretch_ = false;
    bool has_modulated_phase_offset_ = false;
    bool has_modulated_amplitude_ = false;
    bool has_modulated_waveform_ = false;
    bool effective_shape_differs_ = false;

    SynthSlider* wave_slider_;
    SynthSlider* amplitude_slider_;
    SynthSlider* phase_stretch_slider_;
    SynthSlider* phase_offset_slider_;
    SynthBase* synth_ = nullptr;
    std::array<int, 2> telemetry_indices_ { -1, -1 };
    std::array<float, 2> telemetry_values_ { 0.0f, 0.0f };
    std::array<int, 5> random_telemetry_indices_ { -1, -1, -1, -1, -1 };
    std::array<float, 5> random_telemetry_values_ {};
    std::array<mopo::mopo_float, 5> random_preview_values_ { 0.3, 0.9, -0.9, -0.2, 0.3 };
    bool realtime_feedback_enabled_ = false;
    Path wave_path_;
    Path wave_fill_path_;
    Path effective_wave_path_;
    Path effective_wave_fill_path_;
    bool is_control_rate_;
    int resolution_;
    float phase_;
    float amp_;
    Point<float> mouse_down_position_;
    bool phase_drag_started_ = false;
    Image background_;
    float background_scale_ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveViewer)
};

#endif // WAVE_VIEWER_H
