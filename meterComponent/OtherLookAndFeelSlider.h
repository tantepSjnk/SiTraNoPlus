/*
  ==============================================================================

    OtherLookAndFeelSlider.h
    Created: 8 Aug 2023 11:09:39am
    Author:  Tantep

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
using namespace juce;

//==============================================================================
 class OtherLookAndFeelWaveZoom : public juce::LookAndFeel_V4
{
public:
    OtherLookAndFeelWaveZoom()
    {
        setColour(juce::Slider::thumbColourId, juce::Colours::red);
    }
    void drawLinearSlider(Graphics & g, int x, int y, int width, int height, float sliderPos,
        float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider & slider) override
    {
        auto cornerSize = 1;
        auto lineThick = 1.5;
        if (style == Slider::SliderStyle::LinearHorizontal)
        {
            // Draw the track
            float trackWidth = width;
            float trackHeight = 5;
            float trackX = x;
            float trackY = height / 2 - trackHeight / 2;

            g.setColour(juce::Colour::fromRGB(64, 64, 64).darker(0.5)); 
            g.drawRoundedRectangle(trackX, trackY, trackWidth, trackHeight, cornerSize, lineThick*1.5);
            ColourGradient trackGradient(Colour::fromRGB(165, 170, 177).darker(0.1), trackX, trackY,
                Colour::fromRGB(165, 170, 177).brighter(0.25), trackX + trackWidth, trackY + trackHeight, true);
            g.setGradientFill(trackGradient);
            g.fillRoundedRectangle(trackX, trackY, trackWidth, trackHeight, cornerSize);

            // Draw the ticks

            int numTicks = 2; // Number of ticks
            String tickLabels[] = { "+", "-" };
            for (int i = 0; i < numTicks; ++i)
            {
                float tickX = juce::jmap(i, 0, numTicks - 1, int(trackX), int(trackX + trackWidth));
                float tickY1 = trackY ;

                // Label the ticks with the specified values
                String tickLabel = tickLabels[i];
                g.setColour(Colours::white);
                // Create a bold font
                Font font("Microsoft Sans Serif", 16.0f, Font::bold);
                g.setColour(Colours::white);
                g.setFont(font);
                if (i == 0)
                    g.drawText(tickLabel, tickX - 23, tickY1 - trackHeight / 2, 30, 10, Justification::centred);
                else
                    g.drawText(tickLabel, tickX - 13, tickY1 - trackHeight / 2, 25, 10, Justification::right);
            }
            // Draw the thumb
            float thumbHeight = trackHeight + 2 * lineThick;
            float thumbWidth = thumbHeight * 2;
            

            float thumbX = jlimit<float>(x, x + width - thumbWidth, sliderPos - thumbWidth / 2); // Limit thumbX to the slider bounds
            float thumbY = height / 2 - thumbHeight / 2; // Centered horizontally

            g.setColour(juce::Colour::fromRGB(64, 64, 64).darker(0.5));
            g.drawRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, cornerSize, lineThick * 1.5);
            ColourGradient thumbGradient(Colour::fromRGB(64, 64, 64).darker(0.1), trackX, trackY,
                Colour::fromRGB(64, 64, 64).brighter(0.4), trackX + trackWidth, trackY + trackHeight, true);
            g.setGradientFill(thumbGradient);
            g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, cornerSize);
        }
        else if (style == Slider::SliderStyle::LinearVertical)
        {
            // Draw the track
            float trackWidth = 5;
            float trackHeight = height;
            float trackX = width / 2 - trackWidth / 2; 
            float trackY = y;

            g.setColour(juce::Colour::fromRGB(64, 64, 64).darker(0.5));
            g.drawRoundedRectangle(trackX, trackY, trackWidth, trackHeight, cornerSize, lineThick*1.5);
            ColourGradient trackGradient(Colour::fromRGB(165, 170, 177).darker(0.1), trackX, trackY,
                Colour::fromRGB(165, 170, 177).brighter(0.1), trackX + trackWidth, trackY + trackHeight, true);
            g.setGradientFill(trackGradient);
            g.fillRoundedRectangle(trackX, trackY, trackWidth, trackHeight, cornerSize);

            // Draw the ticks

            int numTicks = 2; // Number of ticks
            String tickLabels[] = { "+", "-" };
            for (int i = 0; i < numTicks; ++i)
            {

                // Label the ticks with the specified values
                String tickLabel = tickLabels[i];
                g.setColour(Colours::white);
                // Create a bold font
                Font font("Microsoft Sans Serif", 16.0f, Font::bold);
                g.setColour(Colours::white);
                g.setFont(font);
                if (i == 0)
                    g.drawText(tickLabel, trackX -2, trackY -13 , 35, 10, Justification::left);
                else
                    g.drawText(tickLabel, trackX -0.005, trackY + trackHeight , 35, 10, Justification::left);
            }
            // Draw the thumb
            float thumbWidth = trackWidth  + 2 * lineThick;
            float thumbHeight = thumbWidth * 2;

            float thumbX = width / 2 - thumbWidth / 2; 
            float thumbY = jlimit<float>(y, y + height - thumbHeight, sliderPos - thumbHeight / 2); 
            

            g.setColour(juce::Colour::fromRGB(64, 64, 64).darker(0.5));
            g.drawRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, cornerSize, lineThick * 1.5);            
            ColourGradient thumbGradient(Colour::fromRGB(64, 64, 64).darker(0.1), trackX, trackY,
                Colour::fromRGB(64, 64, 64).brighter(0.4), trackX + trackWidth, trackY + trackHeight, true);
            g.setGradientFill(thumbGradient);
            g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, cornerSize);
        }
    } 
};
//==============================================================================

class OtherLookAndFeelProgressBar : public juce::LookAndFeel_V4
{
public:
    OtherLookAndFeelProgressBar()
    {
        setColour(juce::Slider::thumbColourId, juce::Colours::red);
    }
    void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
        float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& slider) override
    {
        float cornerSize = 1;
        float lineThick = 1.5;

        // Draw the track
        float trackWidth = width;
        float trackHeight = 4;

        float trackX = x;
        float trackY = height / 2 - trackHeight / 2;// Centered vertically

        float thumbWidth = height / 3;
        float thumbHeight = height / 3;

        float thumbX = jlimit<float>(x, x + width - thumbWidth, sliderPos - thumbWidth / 2); // Limit thumbX to the slider bounds
        float thumbY = height / 2 - thumbHeight / 2; // Centered horizontally

        
        
        g.setColour(juce::Colour::fromRGB(65, 70, 77)); // dark grey
        g.drawRoundedRectangle(trackX, trackY, trackWidth, trackHeight, cornerSize, lineThick*2);
        // Draw the colored track to the left of the thumb
        ColourGradient trackGradient(Colour::fromRGB(100, 153, 3).darker(0.1), trackX, trackY,
            Colour::fromRGB(100, 153, 3).brighter(0.25), trackX + trackWidth, trackY + trackHeight, true);
        g.setGradientFill(trackGradient);
        g.fillRoundedRectangle(trackX, trackY, thumbX - trackX, trackHeight, cornerSize);

        // Draw the grey track to the right of the thumb
        g.setColour(juce::Colour::fromRGB(165,170,177)); // grey
        g.fillRoundedRectangle(thumbX + thumbWidth / 2, trackY, trackX + trackWidth - (thumbX + thumbWidth / 2), trackHeight, cornerSize);

        // Draw the thumb
        g.setColour(Colour::fromRGB(120, 170, 130));
        g.drawEllipse(thumbX, thumbY, thumbWidth, thumbHeight, lineThick * 2.f);
        g.setColour(juce::Colour::fromRGB(73,73,73));
        g.fillEllipse(thumbX, thumbY, thumbWidth, thumbHeight);

        // Draw the small thumb
        float smallThumbWidth = thumbWidth / 1.45;
        float smallThumbHeight = thumbHeight / 1.45;
        if(slider.getThumbBeingDragged() == 0)
        {
            smallThumbWidth = thumbWidth / 2;
            smallThumbHeight = thumbHeight / 2;
        }
        

        float smallThumbX = thumbX + thumbWidth / 2 - smallThumbWidth / 2; // Centered horizontally within the normal thumb
        float smallThumbY = height / 2 - smallThumbHeight / 2; // Centered vertically


        ColourGradient smallThumbGradient(Colours::green.darker(0.5), smallThumbX, smallThumbY,
            Colours::forestgreen.brighter(0.25), smallThumbX + smallThumbWidth, smallThumbY + smallThumbHeight, true);
        g.setGradientFill(smallThumbGradient);
        g.fillEllipse(smallThumbX, smallThumbY, smallThumbWidth, smallThumbHeight);


    }

    void drawLabel(Graphics& g, Label& label) override
    {
        // Fill background
        g.fillAll(label.findColour(Label::backgroundColourId));

        g.setColour(Colours::white);
        const Font font(getLabelFont(label));
        g.setFont(font);
        g.setFont(juce::Font(13.0f, juce::Font::plain));
        // Draw enabled text
        auto perc = std::round( label.getText().getFloatValue() * 100);

        g.drawFittedText("Progress: " + String(perc) + " %", label.getLocalBounds(), Justification::centredTop, 1, 1.0f);
    }
};

// // =============================================================================================

class OtherLookAndFeelHorizontal : public juce::LookAndFeel_V4
{
public:
    OtherLookAndFeelHorizontal()
    {
        setColour(juce::Slider::thumbColourId, juce::Colours::red);
    }
    void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
        float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& slider) override
    {
        auto cornerSize = 1;
        auto lineThick = 1.5;

        // Draw the track
        float trackWidth = width;
        float trackHeight = 4;

        float trackX = x; 
        float trackY = height / 2 - trackHeight / 2;// Centered vertically

        // Draw the ticks
        
        int numTicks = 3; // Number of ticks
        //String tickLabels[] = { "1/8", "",  "8" };
        for (int i = 0; i < numTicks; ++i) {
            g.setColour(Colour::fromRGB(70,70,70).darker(0.5));
            float tickX = juce::jmap(i, 0, numTicks - 1, int(trackX), int(trackX + trackWidth));
            float tickY1 = trackY - 5; // Upper y-coordinate of the tick
            float tickY2 = trackY + trackHeight + 5; // Lower y-coordinate of the tick
            g.drawLine(tickX, tickY1, tickX, tickY2, lineThick);

            //// Label the ticks with the specified values
            //String tickLabel = tickLabels[i];
            //g.setColour(Colours::white);
            //    g.drawText(tickLabel, tickX - 10 - 5 * bool(i), tickY1 + 20, 30, 20, Justification::centred);
        }

        //g.setColour(juce::Colour::fromRGB(191,144,3)); // dark yellow
        g.setColour(juce::Colour::fromRGB(64, 64, 64).darker(0.5));
        g.drawRoundedRectangle(trackX, trackY, trackWidth, trackHeight, cornerSize, lineThick);

        ColourGradient trackGradient(Colour::fromRGB(255, 153, 3).darker(0.1), trackX, trackY,
            Colour::fromRGB(255, 153, 3).brighter(0.25), trackX + trackWidth, trackY + trackHeight, true);
        g.setGradientFill(trackGradient);
        //g.setColour(juce::Colour::fromRGB(255,153,0)); // orange-yellow
        g.fillRoundedRectangle(trackX, trackY, trackWidth, trackHeight, cornerSize);


        // Draw the thumb
        float thumbWidth = width / 15;
        float thumbHeight = height / 1.75;

        float thumbX = jlimit<float>(x, x + width - thumbWidth, sliderPos - thumbWidth / 2); // Limit thumbX to the slider bounds
        float thumbY = height / 2 - thumbHeight / 2; // Centered horizontally
        
        g.setColour(juce::Colour::fromRGB(64, 64, 64).brighter(1));
        //g.setColour(juce::Colour::fromRGB(65, 70, 70).darker(1));
        g.drawRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, cornerSize, lineThick*2);
        g.setColour(juce::Colour::fromRGB(64, 64, 64).darker(1));
        g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, cornerSize);

        

        // Draw the small thumb
        float smallThumbWidth = thumbWidth / 3;
        float smallThumbHeight = thumbHeight / 1.5;
        if (slider.getThumbBeingDragged() == 0)
        {
            smallThumbWidth = thumbWidth / 3.5;
            smallThumbHeight = thumbHeight / 2.5;
        }

        //float smallThumbX = jlimit<float>(x, x + width - thumbWidth, sliderPos - thumbWidth / 2); // Limit smallThumbX to the slider bounds
        float smallThumbX = thumbX + thumbWidth / 2 - smallThumbWidth / 2; // Centered horizontally within the normal thumb
        float smallThumbY = height / 2 - smallThumbHeight / 2; // Centered vertically


        ColourGradient smallThumbGradient(Colour::fromRGB(255, 153, 3).darker(0.5), trackX, trackY,
            Colour::fromRGB(255, 153, 9).brighter(0.9), trackX + trackWidth, trackY + trackHeight, true);
        g.setGradientFill(smallThumbGradient);
        g.fillRoundedRectangle(smallThumbX, smallThumbY, smallThumbWidth, smallThumbHeight, cornerSize);

    }

    void drawLabel(Graphics& g, Label& label) override
    {
        // Fill background
        g.fillAll(label.findColour(Label::backgroundColourId));

        // Set text color and font
        g.setColour(Colours::white);
        const Font font(getLabelFont(label));
        g.setFont(font);

        if (!label.isBeingEdited())
        {
            const float alpha = label.isEnabled() ? 1.0f : 0.5f;

            // Draw fitted text with padding
            String labelText = label.getText();
            if (labelText.isNotEmpty())
            {
                float value = labelText.getFloatValue();
                String formattedText = String::formatted("%.2f", value); // Format the number to 2 decimal places

                g.drawFittedText(formattedText, label.getLocalBounds(), Justification::centred, 2, 1.0f);
            }
        }
        else if (label.isEnabled())
        {
            // Draw enabled text
            g.drawFittedText(label.getText(), label.getLocalBounds(), label.getJustificationType(), 2, 1.0f); // Line spacing factor
        }
    }
};

//==============================================================================

class OtherLookAndFeelVertical : public juce::LookAndFeel_V4
{
public:
    OtherLookAndFeelVertical()
    {
        setColour(juce::Slider::thumbColourId, juce::Colours::red);
    }
    void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
        float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& slider) override
    {
        auto cornerSize = 1;
        auto lineThick = 1.5;

        // Draw the track
        float trackWidth = 4;
        float trackHeight = height;

        float trackX = width / 2 - trackWidth / 2; // Centered horizontally
        float trackY = y;

        //g.setColour(juce::Colour::fromRGB(97, 97, 97)); // dark grey
        g.setColour(juce::Colour::fromRGB(64, 64, 64).darker(0.5));
        g.drawRoundedRectangle(trackX, trackY, trackWidth, trackHeight, cornerSize, lineThick);
        
        //g.setColour(juce::Colour::fromRGB(85, 147, 202)); // blue
        ColourGradient trackGradient(Colour::fromRGB(85, 147, 202).darker(0.1), trackX, trackY,
            Colour::fromRGB(85, 147, 202).brighter(0.25), trackX + trackWidth, trackY + trackHeight, true);
        g.setGradientFill(trackGradient);
        g.fillRoundedRectangle(trackX, trackY, trackWidth, trackHeight, cornerSize);

        // Draw the thumb
        float thumbWidth = width / 4;
        float thumbHeight = width / 4;

        float thumbX = width / 2 - thumbWidth / 2; // Centered horizontally
        float thumbY = sliderPos - thumbHeight / 2; // Keep it centered vertically

        g.setColour(juce::Colour::fromRGB(180, 199, 231));
        g.drawEllipse(thumbX, thumbY, thumbWidth, thumbHeight, lineThick * 2);
        g.setColour(juce::Colour::fromRGB(64, 64, 64));
        g.fillEllipse(thumbX, thumbY, thumbWidth, thumbHeight);

        // Draw the small thumb
        float smallThumbWidth = thumbWidth / 2;
        float smallThumbHeight = thumbHeight / 2;
        if (slider.getThumbBeingDragged() == 0)
        {
            smallThumbWidth = thumbWidth / 3;
            smallThumbHeight = thumbHeight / 3;
        }

        float smallThumbX = width / 2 - smallThumbWidth / 2; // Centered horizontally
        float smallThumbY = sliderPos - smallThumbHeight / 2; // Keep it centered vertically

        //g.setColour(juce::Colour::fromRGB(112, 121, 170));
        ColourGradient smallThumbGradient(Colour::fromRGB(113, 147, 202).darker(0.5), smallThumbX, smallThumbY,
                                          Colour::fromRGB(112, 121, 170).brighter(0.5), smallThumbX + smallThumbWidth/1.01, smallThumbY+smallThumbHeight / 1.01, true);

        g.setGradientFill(smallThumbGradient);
        g.fillEllipse(smallThumbX, smallThumbY, smallThumbWidth, smallThumbHeight);
        
    }

    void drawLabel(Graphics& g, Label& label) override
    {
        // Fill background
        g.fillAll(label.findColour(Label::backgroundColourId));

        // Set text color and font
        g.setColour(Colours::white);
        const Font font(getLabelFont(label));
        g.setFont(font);
        g.setFont(juce::Font(12.0f, juce::Font::plain));
        //g.setFont(12);

        if (!label.isBeingEdited())
        {
            const float alpha = label.isEnabled() ? 1.0f : 0.5f;

            // Draw fitted text with padding
            String labelText = label.getText();
            if (labelText.isNotEmpty())
            {
                float value = labelText.getFloatValue();
                String formattedText = String(value, 1);
                if (value > 0)
                    formattedText = "+" + formattedText;
                formattedText += "\ndB";

                g.drawFittedText(formattedText, label.getLocalBounds(), label.getJustificationType(), Justification::centredLeft, 1.0f); // Line spacing factor
            }
        }
        else if (label.isEnabled())
        {
            // Draw enabled text
            g.drawFittedText(label.getText(), label.getLocalBounds().reduced(3), label.getJustificationType(),
                2,  // Justification::centred
                1.0f); // Line spacing factor
        }
    }
};


