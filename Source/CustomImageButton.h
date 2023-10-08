#pragma once

#include <JuceHeader.h>

class CustomImageButton : public juce::ImageButton
{
public:
    CustomImageButton();

    // Override the paintButton method to customize the button appearance
    void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override;

private:
    float customOpacity = 1.0f; // The default opacity for the button
};

//#pragma once
//#ifndef CUSTOMIMAGEBUTTON_H_INCLUDED
//#define CUSTOMIMAGEBUTTON_H_INCLUDED
//
//#include <JuceHeader.h>
//
//class CustomImageButton : public juce::Button
//{
//public:
//    CustomImageButton(const juce::String& imagePath);
//    ~CustomImageButton();
//
//    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
//    void mouseDown(const juce::MouseEvent& event) override; // Add mouseDown function
//
//    bool hitTest(int x, int y) override; // Add hitTest function
//
//private:
//    juce::Image imageStrip;
//};
//
//#endif // CUSTOMIMAGEBUTTON_H_INCLUDED
