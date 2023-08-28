#pragma once

#include <JuceHeader.h>
using namespace juce;
namespace Gui
{
	class LightBox : public Component
	{
	public:
		LightBox(const Colour& c) : colour(c) {}
		void paint(Graphics& g) override
		{
			if (isOn)
			{
				g.setColour(colour);

			}
			else
			{
				g.setColour(Colours::black);
			}
			const auto delta = 1.f;
			const auto bounds = getLocalBounds().toFloat().reduced(delta);
			const auto side = jmin(bounds.getWidth(), bounds.getHeight());
			const auto lightBoxFillBounds = Rectangle<float>{ bounds.getX(), bounds.getY(), side, side };
			g.fillRoundedRectangle(lightBoxFillBounds,1.f);
			g.setColour(Colour::fromRGB(64, 64, 64));
			g.drawRoundedRectangle(lightBoxFillBounds, 2.f, 2.f);
			if (isOn)
			{
				g.setGradientFill(ColourGradient{ colour.withAlpha(0.3f), lightBoxFillBounds.getCentre(), colour.withLightness(1.5f).withAlpha(0.f), {}, true });
				g.fillRoundedRectangle(lightBoxFillBounds.expanded(delta),1.f);
			}

		}
		void setState(const bool state) { isOn = state; }
	private:
		bool isOn = false;
		Colour colour{};
	};

	class verticalGradientMeter : public Component, public Timer
	{
	public:
		//VerticalDiscreteMeter(std::function<float()>&& valueFunction) : valueSupplier(std::move(valueFunction))
		verticalGradientMeter(std::function<float()>&& valueFunction) : valueSupplier(std::move(valueFunction))
		{
			startTimerHz(1000);
		}

		void paint(Graphics& g) override
		{
			auto width = getWidth();
			auto height = getHeight();
			const auto value = jmap(valueSupplier(), -72.f, 6.f, 0.f, 1.f);
			for (auto i = 0; i < totalNumberOfLightBoxs; i++)
			{
				if (value >= static_cast<float>(i + 1) / totalNumberOfLightBoxs)
					lightBoxs[i]->setState(true);
				else
					lightBoxs[i]->setState(false);
			}
			// Draw tick lines
			String tickLabels[] = { " -72",""," -48","","",""," -24",""," -12","   -6","    0","   +6"};
			
			for (auto i = 0; i < totalNumberOfLightBoxs; ++i)
			{
				float x = lightBoxs[i]->getRight();
				float y = lightBoxs[i]->getY() + lightBoxs[i]->getHeight()/10 + 1;
				float left = lightBoxs[i]->getX() + lightBoxs[i]->getWidth() / 5;
				float right = left + 8.f;
				g.setColour(Colours::lightgrey.darker(0.1));
				g.drawHorizontalLine(y, left ,right );
				Font font("Microsoft Sans Serif", 11.0f, Font::plain);
				g.setColour(Colours::lightgrey);
				g.setFont(font);
				g.drawText(tickLabels[i], left + 5.f , lightBoxs[i]->getY(), 20, 15, Justification::right, true);
			}
		}

		void resized() override
		{
			const auto bounds = getLocalBounds().toFloat();
			ColourGradient gradient{ Colours::green.darker(0.1f), bounds.getBottomLeft(), Colours::indianred.darker(0.1f), bounds.getTopLeft(), false };
			gradient.addColour(0.6, Colours::yellow.darker(0.2f));
			gradient.addColour(0.9, Colours::red.darker(0.1f));

			const auto lightBoxHeight = getLocalBounds().getHeight() / totalNumberOfLightBoxs;
			auto lightBoxBounds = getLocalBounds();
			lightBoxs.clear();
			for (auto i = 0; i < totalNumberOfLightBoxs; i++)
			{
				auto lightBox = std::make_unique<LightBox>(gradient.getColourAtPosition(static_cast<double>(i) / totalNumberOfLightBoxs));
				addAndMakeVisible(lightBox.get());
				lightBox->setBounds(lightBoxBounds.removeFromBottom(lightBoxHeight));
				lightBoxs.push_back(std::move(lightBox));
			}
		}

		void timerCallback() override
		{
			repaint();
		}

	private:
		std::function<float()> valueSupplier;
		std::vector<std::unique_ptr<LightBox>> lightBoxs;
		const int totalNumberOfLightBoxs = 12;
	};
}