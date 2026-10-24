#include "doctest.h"
#include "DpadHud.h"
#include "RecordingCanvas.h"
#include <algorithm>
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

	bool says(const RecordingCanvas& canvas, const std::wstring& text)
	{
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::TEXT && call.text == text)
				return true;
		return false;
	}

	bool drew(const RecordingCanvas& canvas, const std::string& path)
	{
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::IMAGE && canvas.pathOf(call.texture) == path)
				return true;
		return false;
	}

	void stock(DpadHud& dpad)
	{
		dpad.setTarget("Locked Door", "interactables/locked-door.png");
		dpad.setSlot(DpadDirection::UP, "Pick", "skills/lockpicking.png");
		dpad.setSlot(DpadDirection::RIGHT, "Force", "approach/force.png");
	}
}

TEST_CASE("an idle D-pad draws only ghosted wedges at 20% opacity: no text and no icons")
{
	DpadHud dpad;
	stock(dpad);
	RecordingCanvas canvas;
	canvas.screenDimension = dimension2d<u32>(1920, 1080);
	dpad.render(canvas);
	CHECK(count(canvas, RecordingCanvas::TEXT) == 0);
	CHECK(count(canvas, RecordingCanvas::IMAGE) == 0);
	CHECK(count(canvas, RecordingCanvas::RECT) == 5);
	CHECK(dpad.opacity() == doctest::Approx(0.2f));
}

TEST_CASE("the D-pad sits in the bottom-left corner and is about 120px square at 1080p")
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
	CHECK(right - left == 120);
	CHECK(bottom - top == 120);
	CHECK(left < 200);
	CHECK(bottom > 1080 - 60);
}

TEST_CASE("an active D-pad fades in over about a quarter second and shows labels, slot icons and the target")
{
	DpadHud dpad;
	stock(dpad);
	dpad.setState(DpadState::ACTIVE);
	dpad.update(0.1f);
	CHECK(dpad.opacity() > 0.2f);
	CHECK(dpad.opacity() < 1.0f);
	dpad.update(0.15f);
	CHECK(dpad.opacity() == doctest::Approx(1.0f));
	RecordingCanvas canvas;
	dpad.render(canvas);
	CHECK(says(canvas, L"Pick"));
	CHECK(says(canvas, L"Force"));
	CHECK(says(canvas, L"Locked Door"));
	CHECK(drew(canvas, "assets/ui/icons/skills/lockpicking.png"));
	CHECK(drew(canvas, "assets/ui/icons/approach/force.png"));
	CHECK(drew(canvas, "assets/ui/icons/interactables/locked-door.png"));
	CHECK_FALSE(says(canvas, L"X: Pick Lock"));
}

TEST_CASE("selecting an available slot shows the X prompt and outlines that wedge; a cleared slot shows nothing")
{
	DpadHud dpad;
	stock(dpad);
	dpad.setState(DpadState::ACTIVE);
	dpad.update(1.0f);
	dpad.setSelectedDirection(DpadDirection::UP);
	dpad.setActionPrompt("Pick Lock");
	RecordingCanvas canvas;
	dpad.render(canvas);
	CHECK(says(canvas, L"X: Pick Lock"));
	CHECK(count(canvas, RecordingCanvas::RECT_OUTLINE) == 1);

	dpad.clearSlot(DpadDirection::UP);
	RecordingCanvas cleared;
	dpad.render(cleared);
	CHECK_FALSE(says(cleared, L"Pick"));
	CHECK_FALSE(says(cleared, L"X: Pick Lock"));
	CHECK_FALSE(drew(cleared, "assets/ui/icons/skills/lockpicking.png"));
}

TEST_CASE("the selected slot pulses between 80% and 100% brightness")
{
	DpadHud dpad;
	stock(dpad);
	dpad.setState(DpadState::ACTIVE);
	dpad.update(1.0f);
	dpad.setSelectedDirection(DpadDirection::UP);
	u32 lowest = 255, highest = 0;
	for (int frame = 0; frame < 30; ++frame)
	{
		RecordingCanvas canvas;
		dpad.render(canvas);
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::RECT_OUTLINE)
			{
				lowest = std::min(lowest, call.colour.getRed());
				highest = std::max(highest, call.colour.getRed());
			}
		dpad.update(0.5f / 30);
	}
	CHECK(lowest >= 255 * 8 / 10 - 1);
	CHECK(lowest <= 255 * 82 / 100);
	CHECK(highest >= 253);
}
