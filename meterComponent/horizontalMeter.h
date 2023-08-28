/*
  ==============================================================================

    HorizontalMeter.h
    Created: 24 Jul 2023 11:47:21pm
    Author:  Tantep

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
using namespace juce;
namespace Gui
{
	class horizontalMeter : public Component
	{
	public:

		void paint(Graphics& g) override
		{
			auto bounds = getLocalBounds().toFloat();

			g.setColour(Colours::white.withBrightness(0.4f));
			g.fillRoundedRectangle(bounds, 5.f);

			g.setColour(Colours::white);
			const auto scaledX = jmap(level, -60.f, 6.f, 0.f, static_cast<float>(getWidth()));
			g.fillRoundedRectangle(bounds.removeFromLeft(scaledX), 5.f);
		}

		void setLevel(const float value) { level = value; }
	private:
		float level = -60.f;
	};
}