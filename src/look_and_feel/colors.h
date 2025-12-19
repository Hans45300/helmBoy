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
#ifndef COLORS_H
#define COLORS_H

#include <JuceHeader.h>

/*
 * Système de couleurs rationalisé HelmBoy
 * =======================================
 *
 * Philosophie :
 * - Palette FROIDE : Bleu de référence #03a9f4 et ses variations (audio, sélections)
 * - Palette CHAUDE : Orange de référence #ff7043 et ses variations (logos, états actifs)
 * - Gris NEUTRES : Conservés pour fonds, textes et interfaces (stabilité visuelle)
 *
 * Cette approche garantit :
 * - Cohérence visuelle par familles de couleurs
 * - Lisibilité préservée avec les gris neutres
 * - Hiérarchie claire : froid=fonctionnel, chaud=actif/important
 */

class Colors {
  public:

    static const Colour amethyst;
    static const Colour sapphire;
    static const Colour emerald;
    static const Colour topaze;
    static const Colour orange;
    static const Colour ruby;

  // === Couleurs de base de l'interface ===
    static const Colour background;
    static const Colour tab_heading;
    static const Colour tab_body;
    static const Colour tab_heading_text;
    static const Colour control_label_text;
    static const Colour info_background;
    static const Colour overlay_screen;

    // === Couleurs des signaux audio et modulation ===
    // Palette froide basée sur le bleu de référence #03a9f4
    static const Colour audio;           // Bleu principal - signaux audio
    static const Colour modulation;      // Cyan - modulation (variation froide)
    static const Colour graph_disable;   // Gris neutre
    static const Colour graph_fill;      // Gris neutre

    // === PopupMenu couleurs ===
    static const Colour popup_background;
    static const Colour popup_text;
    static const Colour popup_header_text;
    static const Colour popup_highlighted_background;
    static const Colour popup_highlighted_text;
    static const Colour bubble_background;
    static const Colour tooltip_text;

    // === Sliders couleurs ===
    static const Colour slider_active;
    static const Colour slider_inactive;
    static const Colour slider_thumb_active;
    static const Colour slider_thumb_inactive;
    static const Colour slider_lighten_active;
    static const Colour slider_lighten_inactive;
    static const Colour slider_track_background;
    static const Colour slider_track_fill;

    // === Couleurs de texte et composants texte ===
    // Gris neutres conservés + variations bleues pour sélections
    static const Colour combo_arrow;
    static const Colour combo_outline;
    static const Colour label_text;
    static const Colour list_text;
    static const Colour text_background;
    static const Colour text_background_highlighted;
    static const Colour text_selection;        // Bleu foncé (variation de référence)
    static const Colour text_selection_browser; // Bleu intermédiaire (variation de référence)

    // === HelmBoyGraphics couleurs ===
    // Palette d'accents pour logos et éléments visuels
    static const Colour helm_primary_accent;    // Accent principal (logos, éléments importants)
    static const Colour helm_secondary_accent;  // Accent secondaire (variations)
    static const Colour helm_tertiary_accent;   // Accent tertiaire (nuances)

    // === Couleurs d'ombres et effets ===
    static const Colour shadow_dark;
    static const Colour shadow_light;
    static const Colour shadow_thumb;

    // === Couleurs de bordures et contours ===
    static const Colour border_light;
    static const Colour border_dark;
    static const Colour highlight_overlay;
    static const Colour lowlight_overlay;

    // === Couleurs supplémentaires pour sliders et composants ===
    static const Colour button_background_light;
    static const Colour button_background_dark;
    static const Colour button_border;
    static const Colour knob_arc_background;
    static const Colour knob_arc_fill;
    static const Colour text_button_normal;
    static const Colour text_button_down;
    static const Colour grid_full_white;

    // === Couleurs supplémentaires résiduelles, regroupées d'après leurs valeurs ===
    static const Colour Color_00000000;
    static const Colour Color_08ffffff;
    static const Colour Color_11ffffff;
    static const Colour Color_22000000;
    static const Colour Color_33ffffff;
    static const Colour Color_4403a9f4;
    static const Colour Color_44ffffff;
    static const Colour Color_66000000;
    static const Colour Color_77ffffff;
    static const Colour Color_88000000;
    static const Colour Color_99000000;
    static const Colour Color_aaffffff;
    static const Colour Color_bb000000;
    static const Colour Color_bb212121;
    static const Colour Color_bbffffff;
    static const Colour Color_cc000000;
    static const Colour Color_ee000000;
    static const Colour Color_ff000000;
    static const Colour Color_ff03a9f4;
    static const Colour Color_ff111111;
    static const Colour Color_ff2196f3;
    static const Colour Color_ff222222;
    static const Colour Color_ff303030;
    static const Colour Color_ff323232;
    static const Colour Color_ff333333;
    static const Colour Color_ff383838;
    static const Colour Color_ff424242;
    static const Colour Color_ff444444;
    static const Colour Color_ff464646;
    static const Colour Color_ff4a4a4a;
    static const Colour Color_ff4fc3f7;
    static const Colour Color_ff505050;
    static const Colour Color_ff545454;
    static const Colour Color_ff555555;
    static const Colour Color_ff666666;
    static const Colour Color_ff777777;
    static const Colour Color_ff888888;
    static const Colour Color_ffaaaaaa;
    static const Colour Color_ffbbbbbb;
    static const Colour Color_ffcccccc;
    static const Colour Color_ffffd740;
    static const Colour Color_ffffffff;

  private:
    Colors()
    {
    }
};

#endif // COLORS_H
