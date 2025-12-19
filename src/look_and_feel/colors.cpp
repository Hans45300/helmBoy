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

#include "colors.h"

// Couleurs du gradient
const Colour Colors::amethyst(0xff9966cc); // 0% - Améthyste
const Colour Colors::sapphire(0xff0f52ba); // 20% - Saphir
const Colour Colors::emerald(0xff50c878);  // 40% - Émeraude
const Colour Colors::topaze(0xffffc87c);   // 60% - Topaze
const Colour Colors::orange(0xffff8c00);   // 80% - Orange chaleureux
const Colour Colors::ruby(0xffe0115f);     // 100% - Rubis
// === Couleurs de base de l'interface ===
// Couleur de référence pour les gris : arrière-plan principal RGB(24, 24, 24)
const Colour Colors::background = Colour::fromRGB(24, 24, 24);     // Gris très foncé
const Colour Colors::tab_heading = background.brighter(0.8f);      // Gris plus clair - calculé depuis background
const Colour Colors::tab_body = background.brighter(0.8f);         // Gris plus clair - calculé depuis background
const Colour Colors::tab_heading_text = background.brighter(2.5f); // Gris très clair - calculé depuis background
const Colour Colors::control_label_text = background.brighter(2.5f); // Gris très clair - calculé depuis background
const Colour Colors::info_background = background.brighter(0.8f);  // Gris plus clair - calculé depuis background
const Colour Colors::overlay_screen = background.brighter(0.2f).withAlpha(0.73f); // Gris légèrement plus clair avec alpha

// === Couleurs des signaux audio et modulation ===
// Couleur de référence : bleu primaire RGB(3, 169, 244)
const Colour Colors::audio = Colors::sapphire;         // Bleu principal - signaux audio
const Colour Colors::modulation = Colors::emerald; // Cyan - calculé depuis audio (hue vers cyan)
const Colour Colors::graph_disable = background.brighter(1.8f);      // Gris neutre - calculé depuis background
const Colour Colors::graph_fill = background.brighter(1.5f);         // Gris neutre - calculé depuis background

// === PopupMenu couleurs ===
const Colour Colors::popup_background = background.brighter(1.2f);           // Gris moyen - calculé depuis background
const Colour Colors::popup_text = background.brighter(3.5f);                 // Gris très clair - calculé depuis background
const Colour Colors::popup_header_text = background.brighter(1.2f);          // Gris moyen - calculé depuis background
const Colour Colors::popup_highlighted_background = background.darker(0.3f); // Gris plus foncé - calculé depuis background
const Colour Colors::popup_highlighted_text = background.brighter(3.5f);     // Gris très clair - calculé depuis background
const Colour Colors::bubble_background = background.brighter(0.4f);          // Gris légèrement plus clair - calculé depuis background
const Colour Colors::tooltip_text = background.brighter(4.0f);               // Gris très clair - calculé depuis background

// === Sliders couleurs ===
const Colour Colors::slider_active = background.brighter(2.0f);              // Gris clair actif - calculé depuis background
const Colour Colors::slider_inactive = background.brighter(1.0f);            // Gris moyen inactif - calculé depuis background
const Colour Colors::slider_thumb_active = Colours::white;                   // Blanc pur
const Colour Colors::slider_thumb_inactive = background.brighter(2.0f);      // Gris clair - calculé depuis background
const Colour Colors::slider_lighten_active = Colours::white.withAlpha(0.33f);  // Blanc semi-transparent (33%)
const Colour Colors::slider_lighten_inactive = Colours::white.withAlpha(0.13f); // Blanc semi-transparent (13%)
const Colour Colors::slider_track_background = background.brighter(0.5f);    // Gris légèrement plus clair - calculé depuis background
const Colour Colors::slider_track_fill = background.brighter(1.3f);          // Gris moyen - calculé depuis background

// === Couleurs de texte et composants texte ===
const Colour Colors::combo_arrow = background.brighter(2.0f);                // Gris clair - calculé depuis background
const Colour Colors::combo_outline = background.brighter(2.0f);              // Gris clair - calculé depuis background
const Colour Colors::label_text = background.brighter(2.3f);                 // Gris clair - calculé depuis background
const Colour Colors::list_text = background.brighter(2.3f);                  // Gris clair - calculé depuis background
const Colour Colors::text_background = background.brighter(1.2f);            // Gris moyen - calculé depuis background
const Colour Colors::text_background_highlighted = background.brighter(1.6f); // Gris moyen-clair - calculé depuis background
const Colour Colors::text_selection = audio.darker(0.15f);     // Bleu plus foncé - calculé depuis audio
const Colour Colors::text_selection_browser = audio.darker(0.1f); // Bleu intermédiaire - calculé depuis audio

// === HelmBoyGraphics couleurs ===
// Palette d'accents calculée à partir de la couleur de référence audio (bleu #03a9f4)
// Harmonie de couleurs froides utilisant des opérations HSL
const Colour Colors::helm_primary_accent = audio.darker(0.2f);              // Bleu plus foncé - accent principal
const Colour Colors::helm_secondary_accent = audio.withHue(0.52f);         // Cyan (hue vers 0.52) - accent secondaire
const Colour Colors::helm_tertiary_accent = audio.withHue(0.33f);          // Vert-bleu (hue vers 0.33) - accent tertiaire

// === Couleurs d'ombres et effets ===
const Colour Colors::shadow_dark = Colours::black.withAlpha(0.8f);           // Ombre foncée - noir à 80%
const Colour Colors::shadow_light = Colours::black.withAlpha(0.13f);         // Ombre claire - noir à 13%
const Colour Colors::shadow_thumb = Colours::black.withAlpha(0.53f);         // Ombre bouton - noir à 53%

// === Couleurs de bordures et contours ===
const Colour Colors::border_light = Colours::white.withAlpha(0.067f);        // Bordure claire - blanc à 6.7%
const Colour Colors::border_dark = Colours::black.withAlpha(0.067f);         // Bordure foncée - noir à 6.7%
const Colour Colors::highlight_overlay = Colours::white.withAlpha(0.067f);   // Surbrillance - blanc à 6.7%
const Colour Colors::lowlight_overlay = Colours::black.withAlpha(0.067f);    // Sous-éclairage - noir à 6.7%

// === Couleurs supplémentaires pour sliders et composants ===
const Colour Colors::button_background_light = background.brighter(1.4f);    // Gris clair - calculé depuis background
const Colour Colors::button_background_dark = background.brighter(0.9f);     // Gris moyen-foncé - calculé depuis background
const Colour Colors::button_border = background.brighter(1.5f);              // Gris clair - calculé depuis background
const Colour Colors::knob_arc_background = background.brighter(1.45f);       // Gris clair - calculé depuis background
const Colour Colors::knob_arc_fill = background.brighter(2.2f);              // Gris plus clair - calculé depuis background
const Colour Colors::text_button_normal = background.brighter(1.6f);         // Gris moyen - calculé depuis background
const Colour Colors::text_button_down = background.brighter(1.8f);           // Gris plus clair - calculé depuis background
const Colour Colors::grid_full_white = Colour::fromRGB(255,255,255);         // Blanc franc

// === Couleurs supplémentaires résiduelles, regroupées d'après leurs valeurs ===
const Colour Colors::Color_00000000 = Colour::fromRGBA(0, 0, 0, 0);
const Colour Colors::Color_08ffffff = Colour::fromRGBA(255, 255, 255, 8);
const Colour Colors::Color_11ffffff = Colour::fromRGBA(255, 255, 255, 17);
const Colour Colors::Color_22000000 = Colour::fromRGBA(0, 0, 0, 34);
const Colour Colors::Color_33ffffff = Colour::fromRGBA(255, 255, 255, 51);
const Colour Colors::Color_4403a9f4 = Colour::fromRGBA(3, 169, 244, 68);
const Colour Colors::Color_44ffffff = Colour::fromRGBA(255, 255, 255, 68);
const Colour Colors::Color_66000000 = Colour::fromRGBA(0, 0, 0, 102);
const Colour Colors::Color_77ffffff = Colour::fromRGBA(255, 255, 255, 119);
const Colour Colors::Color_88000000 = Colour::fromRGBA(0, 0, 0, 136);
const Colour Colors::Color_99000000 = Colour::fromRGBA(0, 0, 0, 153);
const Colour Colors::Color_aaffffff = Colour::fromRGBA(255, 255, 255, 170);
const Colour Colors::Color_bb000000 = Colour::fromRGBA(0, 0, 0, 187);
const Colour Colors::Color_bb212121 = Colour::fromRGBA(33, 33, 33, 187);
const Colour Colors::Color_bbffffff = Colour::fromRGBA(255, 255, 255, 187);
const Colour Colors::Color_cc000000 = Colour::fromRGBA(0, 0, 0, 204);
const Colour Colors::Color_ee000000 = Colour::fromRGBA(0, 0, 0, 238);
const Colour Colors::Color_ff000000 = Colour::fromRGBA(0, 0, 0, 255);
const Colour Colors::Color_ff03a9f4 = Colour::fromRGBA(3, 169, 244, 255);
const Colour Colors::Color_ff111111 = Colour::fromRGBA(17, 17, 17, 255);
const Colour Colors::Color_ff2196f3 = Colour::fromRGBA(33, 150, 243, 255);
const Colour Colors::Color_ff222222 = Colour::fromRGBA(34, 34, 34, 255);
const Colour Colors::Color_ff303030 = Colour::fromRGBA(48, 48, 48, 255);
const Colour Colors::Color_ff323232 = Colour::fromRGBA(50, 50, 50, 255);
const Colour Colors::Color_ff333333 = Colour::fromRGBA(51, 51, 51, 255);
const Colour Colors::Color_ff383838 = Colour::fromRGBA(56, 56, 56, 255);
const Colour Colors::Color_ff424242 = Colour::fromRGBA(66, 66, 66, 255);
const Colour Colors::Color_ff444444 = Colour::fromRGBA(68, 68, 68, 255);
const Colour Colors::Color_ff464646 = Colour::fromRGBA(70, 70, 70, 255);
const Colour Colors::Color_ff4a4a4a = Colour::fromRGBA(74, 74, 74, 255);
const Colour Colors::Color_ff4fc3f7 = Colour::fromRGBA(79, 195, 247, 255);
const Colour Colors::Color_ff505050 = Colour::fromRGBA(80, 80, 80, 255);
const Colour Colors::Color_ff545454 = Colour::fromRGBA(84, 84, 84, 255);
const Colour Colors::Color_ff555555 = Colour::fromRGBA(85, 85, 85, 255);
const Colour Colors::Color_ff666666 = Colour::fromRGBA(102, 102, 102, 255);
const Colour Colors::Color_ff777777 = Colour::fromRGBA(119, 119, 119, 255);
const Colour Colors::Color_ff888888 = Colour::fromRGBA(136, 136, 136, 255);
const Colour Colors::Color_ffaaaaaa = Colour::fromRGBA(170, 170, 170, 255);
const Colour Colors::Color_ffbbbbbb = Colour::fromRGBA(187, 187, 187, 255);
const Colour Colors::Color_ffcccccc = Colour::fromRGBA(204, 204, 204, 255);
const Colour Colors::Color_ffffd740 = Colour::fromRGBA(255, 215, 64, 255);
const Colour Colors::Color_ffffffff = Colour::fromRGBA(255, 255, 255, 255);