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
	const character::Profession medic{ "Medic", { { character::MEDICINE, 30 } }, {} };

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

TEST_CASE("the vitals cluster sits tight in the top-left corner on the sheet-metal backing art, see-through")
{
	RecordingCanvas canvas;
	canvas.screenDimension = dimension2d<u32>(1280, 720);
	drawVitals(canvas, hurt(100, 100, 100));
	REQUIRE_FALSE(canvas.calls.empty());
	const RecordingCanvas::Call& panel = canvas.calls.front();
	REQUIRE(panel.kind == RecordingCanvas::IMAGE);
	CHECK(canvas.pathOf(panel.texture) == "assets/hud/backing_panel.png");
	CHECK(panel.colour.getAlpha() < 255);
	CHECK(panel.colour.getAlpha() > 80);
	CHECK(panel.source.getWidth() > 0);
	CHECK(panel.area.UpperLeftCorner.X <= 16);
	CHECK(panel.area.UpperLeftCorner.Y <= 16);
	CHECK(panel.area.getWidth() >= 288 * 3 / 2);
	CHECK(panel.area.getWidth() <= 288 * 2);
	CHECK(panel.area.getHeight() >= 72 * 3 / 2);
	CHECK(panel.area.getHeight() <= 72 * 2);
	for (const RecordingCanvas::Call& call : canvas.calls)
		if (call.kind != RecordingCanvas::TEXT && call.texture != image(canvas, "reticle.png")->texture && call.texture != image(canvas, "weapon_panel.png")->texture &&
			(call.kind != RecordingCanvas::IMAGE || canvas.pathOf(call.texture).find("hand") == std::string::npos))
			CHECK(within(panel.area, call.area));
}

TEST_CASE("the rusted frame art is drawn over a square portrait, on top of the head-and-shoulders silhouette")
{
	RecordingCanvas canvas;
	drawVitals(canvas, hurt(100, 100, 100));
	const RecordingCanvas::Call* frame = image(canvas, "portrait_frame.png");
	REQUIRE(frame != nullptr);
	CHECK(frame->area.getWidth() == frame->area.getHeight());
	CHECK(frame->area.getWidth() >= 84);
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
			CHECK(call.area.getHeight() >= 18);
			CHECK(call.area.getHeight() <= 28);
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
		CHECK(icon->area.getHeight() >= 18);
		CHECK(icon->area.getHeight() <= 30);
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
	CHECK(health->area.getWidth() >= 160);
}

TEST_CASE("a small crosshair is drawn last, centred exactly on the screen")
{
	RecordingCanvas canvas;
	canvas.screenDimension = dimension2d<u32>(1280, 720);
	drawVitals(canvas, hurt(100, 100, 100));
	const RecordingCanvas::Call& last = canvas.calls.back();
	REQUIRE(last.kind == RecordingCanvas::IMAGE);
	CHECK(canvas.pathOf(last.texture) == "assets/hud/reticle.png");
	CHECK(last.area.getWidth() >= 24);
	CHECK(last.area.getWidth() <= 32);
	CHECK(last.area.getHeight() == last.area.getWidth());
	CHECK(last.area.getCenter().X == 640);
	CHECK(last.area.getCenter().Y == 360);
	canvas.calls.clear();
	canvas.screenDimension = dimension2d<u32>(1920, 1080);
	drawVitals(canvas, hurt(100, 100, 100));
	CHECK(canvas.calls.back().area.getCenter().X == 960);
	CHECK(canvas.calls.back().area.getCenter().Y == 540);
	CHECK(image(canvas, "weapon_panel.png")->area.LowerRightCorner.X == 1920 - 12);
}

TEST_CASE("a see-through leather pouch sits in the bottom-right corner with the cluster's margin")
{
	RecordingCanvas canvas;
	canvas.screenDimension = dimension2d<u32>(1280, 720);
	drawVitals(canvas, hurt(100, 100, 100));
	const RecordingCanvas::Call* pouch = image(canvas, "weapon_panel.png");
	REQUIRE(pouch != nullptr);
	CHECK(pouch->area.LowerRightCorner.X == 1280 - 12);
	CHECK(pouch->area.LowerRightCorner.Y == 720 - 12);
	CHECK(pouch->area.getWidth() >= 180);
	CHECK(pouch->area.getWidth() <= 320);
	CHECK(pouch->area.getHeight() >= 120);
	CHECK(pouch->area.getHeight() <= 200);
	CHECK(pouch->colour.getAlpha() == 200);
	CHECK(pouch->source.getWidth() > 0);
}


namespace
{
	const RecordingCanvas::Call* fistOf(const RecordingCanvas& canvas)
	{
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::IMAGE && canvas.pathOf(call.texture) == "engine/gamecorev0.1.1/gamecorev0.1.1/bill/icons/hand.jpg")
				return &call;
		return nullptr;
	}
}

TEST_CASE("Mike's fist sits straight on the leather, in its upper part, with no metal frame round it")
{
	RecordingCanvas canvas;
	canvas.screenDimension = dimension2d<u32>(1280, 720);
	drawVitals(canvas, hurt(100, 100, 100));
	CHECK(image(canvas, "weapon_frame.png") == nullptr);
	const RecordingCanvas::Call* pouch = image(canvas, "weapon_panel.png");
	const RecordingCanvas::Call* fist = fistOf(canvas);
	REQUIRE(pouch != nullptr);
	REQUIRE(fist != nullptr);
	CHECK(order(canvas, pouch) < order(canvas, fist));
	CHECK(within(pouch->area, fist->area));
	CHECK(fist->area.getCenter().Y < pouch->area.getCenter().Y);
	CHECK(fist->area.getHeight() >= pouch->area.getHeight() / 3);
	CHECK(fist->area.getCenter().X < pouch->area.getCenter().X);
	f32 shape = (f32)fist->area.getWidth() / (f32)fist->area.getHeight();
	f32 artShape = (f32)fist->source.getWidth() / (f32)fist->source.getHeight();
	CHECK(shape == doctest::Approx(artShape).epsilon(0.05));
}

TEST_CASE("the ammo readout shows --/-- on the leather, centred under the fist, in the bar numbers' style")
{
	RecordingCanvas canvas;
	canvas.screenDimension = dimension2d<u32>(1280, 720);
	drawVitals(canvas, hurt(100, 100, 100));
	const RecordingCanvas::Call* pouch = image(canvas, "weapon_panel.png");
	const RecordingCanvas::Call* fist = fistOf(canvas);
	REQUIRE(pouch != nullptr);
	REQUIRE(fist != nullptr);
	const RecordingCanvas::Call* ammo = nullptr;
	const RecordingCanvas::Call* number = nullptr;
	for (const RecordingCanvas::Call& call : canvas.calls)
		if (call.kind == RecordingCanvas::TEXT)
		{
			if (call.text == L"--/--")
				ammo = &call;
			if (call.text == L"100")
				number = &call;
		}
	REQUIRE(ammo != nullptr);
	REQUIRE(number != nullptr);
	CHECK(canvas.pathOf(ammo->font) == canvas.pathOf(number->font));
	CHECK(ammo->colour == number->colour);
	CHECK(ammo->hcenter);
	CHECK(std::abs(ammo->area.getCenter().X - fist->area.getCenter().X) <= 1);
	CHECK(ammo->area.UpperLeftCorner.Y >= fist->area.LowerRightCorner.Y);
	CHECK(within(pouch->area, ammo->area));
}
