#include "CustomImageButton.h"

CustomImageButton::CustomImageButton()
{
    // Initialize the button with your desired images and other settings
}

void CustomImageButton::paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
    if (isEnabled())
    {
        // Button is enabled, draw as usual
        juce::ImageButton::paintButton(g, isMouseOverButton, isButtonDown);
    }
    else
    {
        // Button is disabled, draw with custom opacity
        g.setOpacity(customOpacity);

        // Draw the button normally with the custom opacity
        juce::ImageButton::paintButton(g, isMouseOverButton, isButtonDown);

        // Reset the opacity back to 1.0 after drawing
        g.setOpacity(1.0f);
    }
}

//#include "CustomImageButton.h"
//
//CustomImageButton::CustomImageButton(const juce::String& imagePath)
//    : juce::Button("CustomImageButton") // Call the base class constructor with a unique name
//{
//    imageStrip = juce::ImageCache::getFromFile(juce::File(imagePath));
//}
//
//CustomImageButton::~CustomImageButton()
//{
//}
//
//void CustomImageButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
//{
//    int frameIndex = 0;
//
//    // Calculate the index of the frame based on the button's state
//    if (!isEnabled())
//        frameIndex = 3; // Disabled state
//    else if (getToggleState())
//        frameIndex = 2; // Clicked state
//    else if (isMouseOver())
//        frameIndex = 1; // Hover state
//
//    // Get the width of a single frame (assuming all frames have equal width)
//    int frameWidth = imageStrip.getWidth() / 4;
//
//    // Draw the appropriate frame from the filmstrip
//    g.drawImage(imageStrip, 0, 0, getWidth(), getHeight(),
//        frameIndex * frameWidth, 0, frameWidth, getHeight());
//}
//
//void CustomImageButton::mouseDown(const juce::MouseEvent& event)
//{
//    // Handle mouse-down event here, e.g., trigger a click or do something else
//    // This is called when the button is pressed.
//    // For example:
//    setToggleState(!getToggleState(), juce::NotificationType::sendNotification);
//    juce::Button::mouseDown(event);
//}
//
//bool CustomImageButton::hitTest(int x, int y)
//{
//    // Override the hitTest() method to define the clickable area of the button.
//    // For example, if the button should only respond to clicks inside its bounds:
//    return juce::Rectangle<int>(0, 0, getWidth(), getHeight()).contains(x, y);
//}