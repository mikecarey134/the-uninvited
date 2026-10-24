#include "doctest.h"
#include "DpadHud.h"
#include "IconArt.h"
#include "RecordingCanvas.h"
#include <algorithm>
#include <cstdlib>
#include <string>

using namespace irr;
using namespace core;

namespace
{
	int count(const RecordingCanvas& canvas, RecordingCanvas::Kind kind)
	{
		int total = 0;
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == kind)
				++total;
		return total;
	}

	const RecordingCanvas::Call* text(const RecordingCanvas& canvas, const std::wstring& line)
	{
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::TEXT && call.text == line)
				return &call;
		return nullptr;
	}

	bool says(const RecordingCanvas& canvas, const std::wstring& line)
	{
		return text(canvas, line) != nullptr;
	}

	const RecordingCanvas::Call* image(const RecordingCanvas& canvas, const std::string& path)
	{
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::IMAGE && canvas.pathOf(call.texture) == path)
				return &call;
		return nullptr;
	}

	bool drew(const RecordingCanvas& canvas, const std::string& path)
	{
		return image(canvas, path) != nullptr;
	}

	void stock(DpadHud& dpad)
	{
		dpad.setTarget("Locked Door", "interactables/locked-door.png");
		dpad.setSlot(DpadDirection::UP, "Pick", "skills/lockpicking.png");
		dpad.setSlot(DpadDirection::RIGHT, "Force", "approach/force.png");
	}

	DpadHud active()
	{
		DpadHud dpad;
		stock(dpad);
		dpad.setState(DpadState::ACTIVE);
		dpad.update(1.0f);
		return dpad;
	}

	// The slot squares are the plus frame's left and right arms; the LEFT one is the leftmost backing rect.
	rect<s32> leftArm(const RecordingCanvas& canvas)
	{
		rect<s32> arm(10000, 0, 10000, 0);
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::RECT && call.area.getWidth() == call.area.getHeight() && call.area.UpperLeftCorner.X < arm.UpperLeftCorner.X)
				arm = call.area;
		return arm;
	}
}

TEST_CASE("a fresh D-pad is idle and still draws its plus frame, centre circle and arrows at 35% opacity, with no rings, text or icons")
{
	DpadHud dpad;
	stock(dpad);
	RecordingCanvas canvas;
	canvas.screenDimension = dimension2d<u32>(1920, 1080);
	dpad.render(canvas);
	CHECK(dpad.state() == DpadState::IDLE);
	CHECK(dpad.opacity() == doctest::Approx(0.35f));
	CHECK(count(canvas, RecordingCanvas::TEXT) == 0);
	CHECK(count(canvas, RecordingCanvas::IMAGE) == 0);
	CHECK(count(canvas, RecordingCanvas::RECT) == 3);
	CHECK(count(canvas, RecordingCanvas::LINE) == 12 + 32 + 8);
}

TEST_CASE("the D-pad sits bottom-left, about 126px square at 1080p, with room on its left for the LEFT label")
{
	DpadHud dpad;
	RecordingCanvas canvas;
	canvas.screenDimension = dimension2d<u32>(1920, 1080);
	dpad.render(canvas);
	s32 left = 1920, top = 1080, right = 0, bottom = 0;
	for (const RecordingCanvas::Call& call : canvas.calls)
		if (call.kind == RecordingCanvas::RECT)
		{
			left = std::min(left, call.area.UpperLeftCorner.X);
			top = std::min(top, call.area.UpperLeftCorner.Y);
			right = std::max(right, call.area.LowerRightCorner.X);
			bottom = std::max(bottom, call.area.LowerRightCorner.Y);
		}
	CHECK(right - left == 126);
	CHECK(bottom - top == 126);
	CHECK(left >= 150);
	CHECK(left < 250);
	CHECK(bottom > 1080 - 60);
}

TEST_CASE("an active D-pad fades in and frames each available icon in its slot's ring: cyan for both skill slots, green Guile, red Force; empty slots get no ring")
{
	DpadHud dpad;
	stock(dpad);
	dpad.setState(DpadState::ACTIVE);
	dpad.update(0.1f);
	CHECK(dpad.opacity() > 0.35f);
	CHECK(dpad.opacity() < 1.0f);
	dpad.update(0.15f);
	CHECK(dpad.opacity() == doctest::Approx(1.0f));
	RecordingCanvas canvas;
	dpad.render(canvas);
	CHECK(says(canvas, L"Pick"));
	CHECK(says(canvas, L"Force"));
	CHECK(says(canvas, L"Locked Door"));
	CHECK(drew(canvas, "assets/ui/icons/interactables/locked-door.png"));
	const RecordingCanvas::Call* cyan = image(canvas, "assets/ui/icons/rings/cyan.png");
	const RecordingCanvas::Call* pick = image(canvas, "assets/ui/icons/skills/lockpicking.png");
	REQUIRE(cyan != nullptr);
	REQUIRE(pick != nullptr);
	CHECK(std::abs(cyan->area.getCenter().X - pick->area.getCenter().X) <= 1);
	CHECK(std::abs(cyan->area.getCenter().Y - pick->area.getCenter().Y) <= 1);
	// Visible art, not PNG canvas: the icon fills about 70% of the ring as seen.
	f32 ringSeen = cyan->area.getWidth() * artBox("rings/cyan.png").fill;
	f32 iconSeen = pick->area.getWidth() * artBox("skills/lockpicking.png").fill;
	CHECK(iconSeen / ringSeen > 0.6f);
	CHECK(iconSeen / ringSeen < 0.8f);
	CHECK(cyan < pick);
	CHECK(drew(canvas, "assets/ui/icons/rings/green.png"));
	CHECK_FALSE(drew(canvas, "assets/ui/icons/rings/red.png"));
	CHECK_FALSE(drew(canvas, "assets/ui/icons/rings/gold.png"));
	int cyans = 0;
	for (const RecordingCanvas::Call& call : canvas.calls)
		cyans += call.kind == RecordingCanvas::IMAGE && canvas.pathOf(call.texture) == "assets/ui/icons/rings/cyan.png";
	CHECK(cyans == 1);
	dpad.setSlot(DpadDirection::DOWN, "Hack", "skills/electronics.png");
	dpad.setSlot(DpadDirection::LEFT, "Kick", "approach/force.png");
	canvas.calls.clear();
	dpad.render(canvas);
	cyans = 0;
	for (const RecordingCanvas::Call& call : canvas.calls)
		cyans += call.kind == RecordingCanvas::IMAGE && canvas.pathOf(call.texture) == "assets/ui/icons/rings/cyan.png";
	CHECK(cyans == 2);
	CHECK(drew(canvas, "assets/ui/icons/rings/red.png"));
	CHECK_FALSE(says(canvas, L"X: Pick Lock"));
}

TEST_CASE("every line of D-pad text sits on a dark see-through pill drawn just before it")
{
	DpadHud dpad = active();
	dpad.setSelectedDirection(DpadDirection::UP);
	dpad.setActionPrompt("Pick Lock");
	RecordingCanvas canvas;
	dpad.render(canvas);
	int lines = 0;
	for (size_t i = 0; i < canvas.calls.size(); ++i)
	{
		const RecordingCanvas::Call& call = canvas.calls[i];
		if (call.kind != RecordingCanvas::TEXT)
			continue;
		++lines;
		REQUIRE(i >= 3);
		const RecordingCanvas::Call& middle = canvas.calls[i - 3];
		CHECK(middle.kind == RecordingCanvas::RECT);
		CHECK(middle.colour.getRed() == 0);
		CHECK(middle.colour.getAlpha() > 140);
		CHECK(middle.colour.getAlpha() < 200);
		CHECK(middle.area.UpperLeftCorner.X <= call.area.UpperLeftCorner.X);
		CHECK(middle.area.LowerRightCorner.X >= call.area.LowerRightCorner.X);
		CHECK(middle.area.UpperLeftCorner.Y < call.area.UpperLeftCorner.Y);
		CHECK(middle.area.LowerRightCorner.Y > call.area.LowerRightCorner.Y);
	}
	CHECK(lines == 4);
}

TEST_CASE("a long LEFT label fits on screen outward; one that cannot flips to the inward side of its slot")
{
	DpadHud dpad = active();
	dpad.setSlot(DpadDirection::LEFT, "Send Companion", "approach/companion-delegate.png");
	RecordingCanvas canvas;
	canvas.screenDimension = dimension2d<u32>(1920, 1080);
	canvas.textDimension = dimension2d<u32>(125, 16);
	dpad.render(canvas);
	rect<s32> arm = leftArm(canvas);
	const RecordingCanvas::Call* label = text(canvas, L"Send Companion");
	REQUIRE(label != nullptr);
	CHECK(label->area.UpperLeftCorner.X >= 5);
	CHECK(label->area.LowerRightCorner.X <= arm.UpperLeftCorner.X);

	canvas.calls.clear();
	canvas.textDimension = dimension2d<u32>(400, 16);
	dpad.render(canvas);
	label = text(canvas, L"Send Companion");
	REQUIRE(label != nullptr);
	CHECK(label->area.UpperLeftCorner.X >= leftArm(canvas).LowerRightCorner.X);
}

TEST_CASE("selecting an available slot shows the X prompt and rims its ring in glowing circles; a cleared slot shows nothing")
{
	DpadHud dpad = active();
	RecordingCanvas canvas;
	dpad.render(canvas);
	int unselected = count(canvas, RecordingCanvas::LINE);
	REQUIRE(drew(canvas, "assets/ui/icons/rings/cyan.png"));
	dpad.setSelectedDirection(DpadDirection::UP);
	dpad.setActionPrompt("Pick Lock");
	canvas.calls.clear();
	dpad.render(canvas);
	CHECK(says(canvas, L"X: Pick Lock"));
	CHECK(count(canvas, RecordingCanvas::LINE) == unselected + 3 * 32);

	dpad.clearSlot(DpadDirection::UP);
	canvas.calls.clear();
	dpad.render(canvas);
	CHECK_FALSE(says(canvas, L"Pick"));
	CHECK_FALSE(says(canvas, L"X: Pick Lock"));
	CHECK_FALSE(drew(canvas, "assets/ui/icons/skills/lockpicking.png"));
	CHECK_FALSE(drew(canvas, "assets/ui/icons/rings/cyan.png"));
	CHECK(drew(canvas, "assets/ui/icons/rings/green.png"));
}

TEST_CASE("the selected slot pulses: its label between 80% and 100% brightness, its ring swelling a little")
{
	DpadHud dpad = active();
	dpad.setSelectedDirection(DpadDirection::UP);
	u32 lowest = 255, highest = 0;
	s32 smallest = 1000, largest = 0;
	// One canvas for every frame, as in the game: the D-pad keeps the textures it loaded from it.
	RecordingCanvas canvas;
	for (int frame = 0; frame < 30; ++frame)
	{
		canvas.calls.clear();
		dpad.render(canvas);
		const RecordingCanvas::Call* label = text(canvas, L"Pick");
		const RecordingCanvas::Call* ring = image(canvas, "assets/ui/icons/rings/cyan.png");
		REQUIRE(label != nullptr);
		REQUIRE(ring != nullptr);
		lowest = std::min(lowest, label->colour.getBlue());
		highest = std::max(highest, label->colour.getBlue());
		smallest = std::min(smallest, ring->area.getWidth());
		largest = std::max(largest, ring->area.getWidth());
		dpad.update(0.5f / 30);
	}
	CHECK(lowest >= 0xbb * 8 / 10 - 1);
	CHECK(lowest <= 0xbb * 82 / 100);
	CHECK(highest >= 0xbb - 2);
	CHECK(largest > smallest);
	CHECK(largest <= smallest * 110 / 100);
}
