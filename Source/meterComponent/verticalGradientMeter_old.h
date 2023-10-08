/*
  ==============================================================================

    verticalGradientMeter.h
    Created: 25 Jul 2023 3:39:20pm
    Author:  Tantep

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
using namespace juce;
namespace Gui
{
	class verticalGradientMeter : public Component, public Timer
	{
	public:
		verticalGradientMeter(std::function<float()>&& valueFunction) : valueSupplier(std::move(valueFunction))
		//verticalGradientMeter(std::function<float()> valueFunction) : valueSupplier(std::move(valueFunction))
		{
			startTimerHz(100);
			//grill = ImageCache::getFromMemory(BinaryData::MeterGrill_png, BinaryData::MeterGrill_pngSize);
		}

		void paint(Graphics& g) override
		{
			auto bounds = getLocalBounds().toFloat();// .reduced(3.f);

			g.setColour(Colours::black.withAlpha(0.2f));
			g.fillRect(bounds);

			g.setGradientFill(gradient);
			//const auto scaledY = jmap(valueSupplier(), -96.0f, 12.0f, 0.f, static_cast<float>(getHeight()));
			//g.fillRect(bounds.removeFromBottom(scaledY));

			const auto scaledY = jlimit(0.f, static_cast<float>(getHeight()), jmap(valueSupplier(), -60.0f, 12.0f, 0.f, static_cast<float>(getHeight())));
			g.fillRect(bounds.removeFromBottom(scaledY));

		}
		void resized() override
		{
			const auto bounds = getLocalBounds().toFloat();
			gradient = ColourGradient{ Colours::green, bounds.getBottomLeft(), Colours::red, bounds.getTopLeft(), false };
			gradient.addColour(0.707, Colours::yellow);
		}

		void paintOverChildren(::juce::Graphics& g) override
		{
			auto bounds = getLocalBounds().toFloat();// .reduced(3.f);

			//g.setColour(Colours::white);
			

			int numGrillLines = 13; // Set the number of grill lines here
			float grillHeight = bounds.getHeight() / static_cast<float>(numGrillLines);


			for (int i = 0; i < numGrillLines; ++i)
			{
				// Calculate the position of the current grill line
				float yPos = bounds.getBottom() - (i + 1) * grillHeight;

				// Draw the current grill line
				Rectangle<float> grillLine(bounds.getX(), yPos, bounds.getWidth(), grillHeight);

				ColourGradient grillGradient(Colours::lightgrey, { bounds.getX(), yPos+i }, Colours::black, { bounds.getRight(), yPos-i }, false);
				g.setGradientFill(grillGradient);
				g.drawRoundedRectangle(grillLine, 1.f, 1.f);
			}

			//Rectangle<float> areaGrill(bounds.getX(), bounds.getBottom()- grillHeight, bounds.getWidth(), grillHeight);
			//g.drawRoundedRectangle(areaGrill, 1.f, 1.f);
		}

		void timerCallback() override
		{
			repaint();
		}
	private:
		std::function<float()> valueSupplier;
		ColourGradient gradient{};
		Image grill;
	};
}