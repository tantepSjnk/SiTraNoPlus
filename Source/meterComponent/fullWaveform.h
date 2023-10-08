/*
  ==============================================================================

    fullWaveform.h
    Created: 29 Jul 2023 2:41:19pm
    Author:  Tantep

  ==============================================================================
*/

#pragma once
#pragma once
#include <JuceHeader.h>
using namespace juce;
namespace Gui
{
	class fullWaveform : public Component, public Timer
	{
	public:
		fullWaveform(std::function < std::vector< float() >> && valueFunction) : valueSupplier(std::move(valueFunction))
			//fullWaveform(std::function<float()> valueFunction) : valueSupplier(std::move(valueFunction))
		{
			startTimerHz(1);
			//grill = ImageCache::getFromMemory(BinaryData::MeterGrill_png, BinaryData::MeterGrill_pngSize);
		}

		void paint(Graphics& g) override
		{
			auto bounds = getLocalBounds().toFloat().reduced(3.f);

			g.setColour(Colours::black);
			g.fillRect(bounds);

			g.setGradientFill(gradient);
			const auto scaledY = jmap(valueSupplier(), -96.0f, 12.0f, 0.f, static_cast<float>(getHeight()));
			g.fillRect(bounds.removeFromBottom(scaledY));
		}

		void resized() override
		{
			const auto bounds = getLocalBounds().toFloat();
			gradient = ColourGradient{ Colours::green, bounds.getBottomLeft(), Colours::red, bounds.getTopLeft(), false };
			gradient.addColour(0.5, Colours::yellow);
		}

		void paintOverChildren(::juce::Graphics& g) override
		{
			//g.drawImage(grill, getLocalBounds().toFloat());
		}

		void timerCallback() override
		{
			repaint();
		}
	private:
		std::function<std::vector<float()>> valueSupplier;
		ColourGradient gradient{};
	};
}


//juce::Rectangle<float> areaProgressBar(defaultPlotLeft, 220, defaultPlotRight, 50);
//g.drawRect(areaProgressBar, lineThick);
//if (yS.size() > 0 && isPaintWave)
//{
//	auto fullWaveData = yS;
//	// Calculate scale factors for mapping samples to the graphics area
//	float scaleX = areaProgressBar.getWidth() / fullWaveData.size();// numSamples;
//	float scaleY = height; // Assuming the waveform is centered vertically
//
//	g.setColour(juce::Colours::slategrey); // Set the color for the waveform
//
//	// Draw the waveform for each channel
//
//
//	// Move to the starting position of the waveform
//	float x = areaProgressBar.getX();
//	float y = areaProgressBar.getY() + (height / 2.0f);
//
//	// Draw the waveform by connecting line segments
//	for (int sample = 0; sample < fullWaveData.size() - 1; ++sample)
//	{
//		float x1 = x + scaleX * sample;
//		float y1 = y + scaleY * yS[sample];
//		float x2 = x + scaleX * (sample + 1);
//		float y2 = y + scaleY * yS[sample + 1];
//		g.drawLine(x1, y1, x2, y2, lineThick);
//	}
//	//isPaintWave = false;
//}