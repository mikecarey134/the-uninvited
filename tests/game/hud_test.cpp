#include "doctest.h"
#include "Hud.h"
#include "RecordingCanvas.h"
#include "Theme.h"
#include <string>

using namespace irr;
using namespace core;

namespace
{
	const character::Profession medic{ "Medic", { { character::Skill::Medicine, 30 } }, {} };

	character::CharacterSheet hurt(int health, int hunger, int water)
	{
		std::string text = character::CharacterSheet::create("Ivy", character::Gender::Male, medic).toText();
		auto set = [&text](const std::string& key, int value) {
			std::string::size_type at = text.find("\n" + key + " = ");
			REQUIRE(at != std::string::npos);
			std::string::size_type end = text.find('\n', at + 1);
			text.replace(at + 1, end - at - 1, key + " = " + std::to_string(value));
		};
		set("health", health);
		set("hunger", hunger);
		set("water", water);
		std::string problem;
		character::CharacterSheet sheet = character::CharacterSheet::fromText(text, problem);
		REQUIRE(problem.empty());
		return sheet;
	}

	const RecordingCanvas::Call* filled(const RecordingCanvas& canvas, video::SColor colour)
	{
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::RECT && call.colour == colour)
				return &call;
		return nullptr;
	}

	bool within(const rect<s32>& outer, const rect<s32>& inner)
	{
		return outer.isPointInside(inner.UpperLeftCorner) && inner.LowerRightCorner.X <= outer.LowerRightCorner.X &&
			inner.LowerRightCorner.Y <= outer.LowerRightCorner.Y;
	}

	bool says(const RecordingCanvas& canvas, const std::wstring& text)
	{
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::TEXT && call.text == text)
				return true;
		return false;
	}
}

TEST_CASE("the vitals cluster sits tight in the top-left corner on a dark see-through panel")
{
	RecordingCanvas canvas;
	canvas.screenDimension = dimension2d<u32>(1280, 720);
	drawVitals(canvas, hurt(100, 100, 100));
	REQUIRE_FALSE(canvas.calls.empty());
	const RecordingCanvas::Call& panel = canvas.calls.front();
	CHECK(panel.kind == RecordingCanvas::RECT);
	CHECK(panel.colour.getAlpha() < 255);
	CHECK(panel.colour.getAlpha() > 80);
	CHECK(panel.area.UpperLeftCorner.X <= 16);
	CHECK(panel.area.UpperLeftCorner.Y <= 16);
	CHECK(panel.area.LowerRightCorner.X <= 320);
	CHECK(panel.area.LowerRightCorner.Y <= 96);
	for (const RecordingCanvas::Call& call : canvas.calls)
		if (call.kind != RecordingCanvas::TEXT)
			CHECK(within(panel.area, call.area));
}

TEST_CASE("a burnt-orange framed square portrait shows a head and shoulders")
{
	RecordingCanvas canvas;
	drawVitals(canvas, hurt(100, 100, 100));
	const RecordingCanvas::Call* frame = nullptr;
	for (const RecordingCanvas::Call& call : canvas.calls)
		if (call.kind == RecordingCanvas::RECT_OUTLINE && call.colour == theme::accent)
			frame = &call;
	REQUIRE(frame != nullptr);
	CHECK(frame->area.getWidth() == frame->area.getHeight());
	CHECK(frame->area.getWidth() >= 40);
	int inside = 0;
	for (const RecordingCanvas::Call& call : canvas.calls)
		if (call.kind == RecordingCanvas::RECT && call.colour == theme::dim && within(frame->area, call.area))
			++inside;
	CHECK(inside >= 2);
}

TEST_CASE("HP, food and water bars fill red, amber and blue in proportion to the character's values, each with its number")
{
	RecordingCanvas canvas;
	drawVitals(canvas, hurt(50, 25, 100));
	const RecordingCanvas::Call* health = filled(canvas, hudHealth);
	const RecordingCanvas::Call* food = filled(canvas, hudFood);
	const RecordingCanvas::Call* water = filled(canvas, hudWater);
	REQUIRE(health != nullptr);
	REQUIRE(food != nullptr);
	REQUIRE(water != nullptr);
	CHECK(health->colour.getRed() > health->colour.getBlue());
	CHECK(food->colour.getRed() > food->colour.getBlue());
	CHECK(food->colour.getGreen() > health->colour.getGreen());
	CHECK(water->colour.getBlue() > water->colour.getRed());
	CHECK(water->area.getWidth() == 2 * health->area.getWidth());
	CHECK(water->area.getWidth() == 4 * food->area.getWidth());
	CHECK(health->area.getHeight() <= 8);
	CHECK(health->area.UpperLeftCorner.Y < food->area.UpperLeftCorner.Y);
	CHECK(food->area.UpperLeftCorner.Y < water->area.UpperLeftCorner.Y);
	CHECK(health->area.UpperLeftCorner.X == water->area.UpperLeftCorner.X);
	CHECK(says(canvas, L"50"));
	CHECK(says(canvas, L"25"));
	CHECK(says(canvas, L"100"));
	CHECK(says(canvas, L"HP"));
	CHECK(says(canvas, L"FOOD"));
	CHECK(says(canvas, L"H2O"));
}

TEST_CASE("an empty meter draws no fill at all, and a full one fills the whole track")
{
	RecordingCanvas canvas;
	drawVitals(canvas, hurt(100, 0, 100));
	CHECK(filled(canvas, hudFood) == nullptr);
	CHECK(says(canvas, L"0"));
	const RecordingCanvas::Call* health = filled(canvas, hudHealth);
	REQUIRE(health != nullptr);
	CHECK(health->area.getWidth() >= 100);
}
