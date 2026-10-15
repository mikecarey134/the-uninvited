#include "doctest.h"
#include "Hud.h"
#include "RecordingCanvas.h"
#include "Theme.h"
#include <cstdlib>
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
	const RecordingCanvas::Call* image(const RecordingCanvas& canvas, const std::string& file)
	{
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::IMAGE && canvas.pathOf(call.texture) == "assets/hud/" + file)
				return &call;
		return nullptr;
	}

	int images(const RecordingCanvas& canvas, const std::string& file)
	{
		int count = 0;
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::IMAGE && canvas.pathOf(call.texture) == "assets/hud/" + file)
				++count;
		return count;
	}

	int order(const RecordingCanvas& canvas, const RecordingCanvas::Call* call)
	{
		return (int)(call - canvas.calls.data());
	}
}

TEST_CASE("the vitals cluster sits tight in the top-left corner on a dark see-through panel while there is no backing art")
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
	CHECK(image(canvas, "backing_panel.png") == nullptr);
	for (const RecordingCanvas::Call& call : canvas.calls)
		if (call.kind != RecordingCanvas::TEXT)
			CHECK(within(panel.area, call.area));
}

TEST_CASE("the rusted frame art is drawn over a square portrait, on top of the head-and-shoulders silhouette")
{
	RecordingCanvas canvas;
	drawVitals(canvas, hurt(100, 100, 100));
	const RecordingCanvas::Call* frame = image(canvas, "portrait_frame.png");
	REQUIRE(frame != nullptr);
	CHECK(frame->area.getWidth() == frame->area.getHeight());
	CHECK(frame->area.getWidth() >= 40);
	CHECK(frame->source.getWidth() > 0);
	int inside = 0;
	for (const RecordingCanvas::Call& call : canvas.calls)
		if (call.kind == RecordingCanvas::RECT && call.colour == theme::dim && within(frame->area, call.area))
		{
			++inside;
			CHECK(order(canvas, &call) < order(canvas, frame));
		}
	CHECK(inside >= 2);
	for (const RecordingCanvas::Call& call : canvas.calls)
		CHECK_FALSE(call.kind == RecordingCanvas::RECT_OUTLINE);
}

TEST_CASE("each bar draws the metal track art with its liquid art cut back from the right to the character's value, and its number")
{
	RecordingCanvas canvas;
	drawVitals(canvas, hurt(50, 25, 100));
	CHECK(images(canvas, "bar_track.png") == 3);
	const RecordingCanvas::Call* health = image(canvas, "fill_hp.png");
	const RecordingCanvas::Call* food = image(canvas, "fill_food.png");
	const RecordingCanvas::Call* water = image(canvas, "fill_h2o.png");
	REQUIRE(health != nullptr);
	REQUIRE(food != nullptr);
	REQUIRE(water != nullptr);
	CHECK(health->area.getWidth() * 2 == doctest::Approx(water->area.getWidth()).epsilon(0.03));
	CHECK(food->area.getWidth() * 4 == doctest::Approx(water->area.getWidth()).epsilon(0.06));
	CHECK(health->source.getWidth() * 2 == doctest::Approx(water->source.getWidth()).epsilon(0.03));
	CHECK(health->area.UpperLeftCorner.X == water->area.UpperLeftCorner.X);
	CHECK(health->area.UpperLeftCorner.Y < food->area.UpperLeftCorner.Y);
	CHECK(food->area.UpperLeftCorner.Y < water->area.UpperLeftCorner.Y);
	for (const RecordingCanvas::Call& call : canvas.calls)
		if (call.kind == RecordingCanvas::IMAGE && canvas.pathOf(call.texture) == "assets/hud/bar_track.png" &&
			call.area.UpperLeftCorner.Y <= water->area.UpperLeftCorner.Y && call.area.LowerRightCorner.Y >= water->area.LowerRightCorner.Y)
		{
			CHECK(within(call.area, water->area));
			CHECK(call.area.getHeight() <= 16);
			CHECK(order(canvas, &call) < order(canvas, water));
		}
	CHECK(says(canvas, L"50"));
	CHECK(says(canvas, L"25"));
	CHECK(says(canvas, L"100"));
	CHECK_FALSE(says(canvas, L"HP"));
	CHECK_FALSE(says(canvas, L"FOOD"));
	CHECK_FALSE(says(canvas, L"H2O"));
}

TEST_CASE("a heart, a tin and a drop sit left of their bars in place of the old labels")
{
	RecordingCanvas canvas;
	drawVitals(canvas, hurt(100, 100, 100));
	const char* icons[] = { "icon_hp.png", "icon_food.png", "icon_h2o.png" };
	const char* fills[] = { "fill_hp.png", "fill_food.png", "fill_h2o.png" };
	for (int i = 0; i < 3; ++i)
	{
		INFO(icons[i]);
		const RecordingCanvas::Call* icon = image(canvas, icons[i]);
		const RecordingCanvas::Call* fill = image(canvas, fills[i]);
		REQUIRE(icon != nullptr);
		REQUIRE(fill != nullptr);
		CHECK(icon->area.LowerRightCorner.X <= fill->area.UpperLeftCorner.X);
		CHECK(icon->area.getHeight() >= 10);
		CHECK(icon->area.getHeight() <= 18);
		s32 iconMiddle = (icon->area.UpperLeftCorner.Y + icon->area.LowerRightCorner.Y) / 2;
		s32 fillMiddle = (fill->area.UpperLeftCorner.Y + fill->area.LowerRightCorner.Y) / 2;
		CHECK(std::abs(iconMiddle - fillMiddle) <= 2);
	}
}

TEST_CASE("an empty meter draws no liquid at all, and a full one fills the whole groove")
{
	RecordingCanvas canvas;
	drawVitals(canvas, hurt(100, 0, 100));
	CHECK(image(canvas, "fill_food.png") == nullptr);
	CHECK(says(canvas, L"0"));
	const RecordingCanvas::Call* health = image(canvas, "fill_hp.png");
	REQUIRE(health != nullptr);
	CHECK(health->area.getWidth() >= 100);
}
