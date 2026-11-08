#include "splash_screen.h"

HelmBoySplashScreen::HelmBoySplashScreen(String name) : Overlay(name)
{}

HelmBoySplashScreen::~HelmBoySplashScreen()
{
}

void HelmBoySplashScreen::paint(Graphics &g)
{
}

void HelmBoySplashScreen::resized() {}
Rectangle<int> HelmBoySplashScreen::getInfoRect()
{
    return Rectangle<int>();
}
void HelmBoySplashScreen::mouseUp(const MouseEvent &e){}

void HelmBoySplashScreen::setVisible(bool should_be_visible) {}

void HelmBoySplashScreen::buttonClicked(Button *clicked_button) {}
