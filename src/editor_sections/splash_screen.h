#pragma once
#ifndef HELMBOY_SPLASH_SCREEN_H
#define HELMBOY_SPLASH_SCREEN_H

#include <JuceHeader.h>
#include "overlay.h"

class HelmBoySplashScreen : public Overlay, public Button::Listener
{
public:
    /** @brief Construct the about overlay with a given display name. */
    HelmBoySplashScreen(String name);

    /** @brief Destructor (default). */
    ~HelmBoySplashScreen() {}

    /** @brief Paint about overlay contents (text, links). */
    void paint(Graphics &g) override;

    /** @brief Layout child components in the overlay. */
    void resized() override;

    /** @brief Get the rectangle area containing informational text. */
    Rectangle<int> getInfoRect();

    /** @brief Handle mouse release events on the overlay. */
    void mouseUp(const MouseEvent &e) override;

    /** @brief Set overlay visibility. */
    void setVisible(bool should_be_visible) override;

    /** @brief Button click handler for overlay buttons. */
    void buttonClicked(Button *clicked_button) override;

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelmBoySplashScreen)
};

#endif // HELMBOY_SPLASH_SCREEN_H