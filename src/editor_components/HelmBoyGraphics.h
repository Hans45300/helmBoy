/*
  ==============================================================================

    HelmBoyGraphics.h
    Created: 1 Nov 2025 1:43:59pm
    Author:  marcs

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

/**
 * @file HelmBoyGraphics.h
 * @brief Petites primitives graphiques partagées (logo, icônes de modulation).
 */

/**
 * @brief Composant graphique pour affichage du logo et des icônes de modulation.
 */
class HelmBoyGraphics : public juce::Component {
public:
    enum class Identity {
        logo,
        Modulation
    };

    /** @brief Constructeur. */
    HelmBoyGraphics();

    /** @brief Dessine le composant. */
    void paint(juce::Graphics& g) override;
    /** @brief Réception d'un clic souris. */
    void mouseDown(const juce::MouseEvent& event) override;
    /** @brief Gestion du redimensionnement. */
    void resized() override;

    // Callback pour les clics
    std::function<void()> onClick;

    // Getters / Setters
    /** @brief Définit l'identité graphique (logo / modulation). */
    void setIdentity(Identity newIdentity);
    /** @brief Force des bords carrés si true. */
    void setForceSquareBounds(bool forceSquare);

    /** @brief Marque l'élément comme sélectionné. */
    void setSelected(bool isSelected);
    /** @brief Indique si l'élément est sélectionné. */
    bool isSelected() const;

    /** @brief Active/désactive l'élément. */
    void setActivated(bool isActive);
    /** @brief Retourne l'état activé. */
    bool isActivated() const;

    /**
     * @brief Crée une image représentant l'icône de modulation.
     * @param size Taille en pixels demandée.
     * @param selected Si vrai, dessine l'état sélectionné.
     * @param active Si vrai, dessine l'état actif.
     * @return Image générée.
     */
    static juce::Image createModulationImage(int size, bool selected, bool active);

private:
    Identity identity = Identity::logo;
    bool selected = false;
    bool active = false;
    bool forceSquareBounds = false;

    //D�finition des couleurs
    juce::Colour activeContourColour;
    juce::Colour inactiveContourColour;

    juce::Path logoPath;
    juce::Path cnxPath;

    /** @brief Convertit une couleur en nuance de gris. */
    juce::Colour toGrayScale(const juce::Colour& colour);
};
