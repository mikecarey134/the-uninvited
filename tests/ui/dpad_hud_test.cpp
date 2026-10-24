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
	dpad.update(0.2f); // past the pick's pop
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
	CHECK(largest * 100 <= smallest * 111);
}

namespace
{
	u32 alphaOf(const RecordingCanvas& canvas, const std::string& path)
	{
		const RecordingCanvas::Call* call = image(canvas, path);
		return call ? call->colour.getAlpha() : 0;
	}

	s32 widthOf(const RecordingCanvas& canvas, const std::string& path)
	{
		const RecordingCanvas::Call* call = image(canvas, path);
		return call ? call->area.getWidth() : 0;
	}

	void stockAll(DpadHud& dpad)
	{
		dpad.setTarget("Locked Door", "interactables/locked-door.png");
		dpad.setSlot(DpadDirection::UP, "Pick", "skills/lockpicking.png");
		dpad.setSlot(DpadDirection::RIGHT, "Send", "approach/companion-delegate.png");
		dpad.setSlot(DpadDirection::DOWN, "Hack", "skills/electronics.png");
		dpad.setSlot(DpadDirection::LEFT, "Kick", "approach/force.png");
	}
}

TEST_CASE("idle to active: the frame brightens first, then each slot pops in from 80% size, up first and left last, all within 300ms, easing out")
{
	DpadHud dpad;
	stockAll(dpad);
	dpad.setState(DpadState::ACTIVE);
	RecordingCanvas canvas;
	dpad.update(0.03f);
	dpad.render(canvas);
	CHECK(dpad.opacity() > 0.35f);
	CHECK(count(canvas, RecordingCanvas::IMAGE) == 0);
	f32 early = dpad.opacity();

	dpad.update(0.04f);
	canvas.calls.clear();
	dpad.render(canvas);
	u32 up = alphaOf(canvas, "assets/ui/icons/skills/lockpicking.png");
	u32 left = alphaOf(canvas, "assets/ui/icons/approach/force.png");
	CHECK(up > 0);
	CHECK(up > left);
	CHECK(up < 255);
	s32 popping = widthOf(canvas, "assets/ui/icons/rings/cyan.png");
	// Ease-out: the first 30ms did more than 30/200 of the frame's brightening.
	CHECK((early - 0.35f) / 0.65f > 0.3f);

	dpad.update(0.23f); // 300ms in
	canvas.calls.clear();
	dpad.render(canvas);
	CHECK(dpad.opacity() == doctest::Approx(1.0f));
	CHECK(alphaOf(canvas, "assets/ui/icons/skills/lockpicking.png") == 255);
	CHECK(alphaOf(canvas, "assets/ui/icons/approach/force.png") == 255);
	s32 full = widthOf(canvas, "assets/ui/icons/rings/cyan.png");
	CHECK(popping < full);
	CHECK(popping >= full * 8 / 10);
}

TEST_CASE("active to idle: icons and rings fade out before the frame dims, and it is all idle again within about 200ms")
{
	DpadHud dpad;
	stockAll(dpad);
	dpad.setState(DpadState::ACTIVE);
	dpad.update(1.0f);
	dpad.setState(DpadState::IDLE);
	dpad.update(0.04f);
	RecordingCanvas canvas;
	dpad.render(canvas);
	CHECK(dpad.opacity() > 0.95f);
	CHECK(alphaOf(canvas, "assets/ui/icons/approach/force.png") < 255);
	dpad.update(0.17f);
	canvas.calls.clear();
	dpad.render(canvas);
	CHECK(dpad.opacity() == doctest::Approx(0.35f));
	CHECK(count(canvas, RecordingCanvas::IMAGE) == 0);
	CHECK(count(canvas, RecordingCanvas::TEXT) == 0);
}

TEST_CASE("picking a slot pops its ring about 10% bigger and back within 150ms; the previously picked slot settles without a pop")
{
	DpadHud dpad = active();
	RecordingCanvas canvas;
	dpad.render(canvas);
	s32 base = widthOf(canvas, "assets/ui/icons/rings/cyan.png");
	s32 baseRed = widthOf(canvas, "assets/ui/icons/rings/green.png");
	dpad.setSelectedDirection(DpadDirection::UP);
	dpad.update(0.05f);
	canvas.calls.clear();
	dpad.render(canvas);
	// The pop on top of the ordinary pulse: well over the ~5% the pulse alone gives at this moment, and no more than 20%.
	CHECK(widthOf(canvas, "assets/ui/icons/rings/cyan.png") * 100 >= base * 109);
	CHECK(widthOf(canvas, "assets/ui/icons/rings/cyan.png") * 100 <= base * 120);
	dpad.update(0.12f);
	dpad.setSelectedDirection(DpadDirection::RIGHT);
	canvas.calls.clear();
	dpad.render(canvas);
	CHECK(widthOf(canvas, "assets/ui/icons/rings/cyan.png") == base);
	dpad.update(0.05f);
	canvas.calls.clear();
	dpad.render(canvas);
	CHECK(widthOf(canvas, "assets/ui/icons/rings/green.png") > baseRed);
	CHECK(widthOf(canvas, "assets/ui/icons/rings/cyan.png") == base);
}

TEST_CASE("switching straight to another target keeps the frame bright and crossfades the old content out and the new in within 150ms")
{
	DpadHud dpad = active();
	dpad.setTarget("Supply Crate", "interactables/supply-crate.png");
	dpad.clearSlot(DpadDirection::UP);
	dpad.clearSlot(DpadDirection::RIGHT);
	dpad.setSlot(DpadDirection::LEFT, "Smash", "approach/force.png");
	RecordingCanvas canvas;
	dpad.update(0.03f);
	dpad.render(canvas);
	CHECK(dpad.opacity() == doctest::Approx(1.0f));
	const RecordingCanvas::Call* old = text(canvas, L"Locked Door");
	REQUIRE(old != nullptr);
	CHECK(old->colour.getAlpha() < 255);
	CHECK(text(canvas, L"Supply Crate") == nullptr);
	dpad.update(0.13f);
	canvas.calls.clear();
	dpad.render(canvas);
	CHECK(dpad.opacity() == doctest::Approx(1.0f));
	CHECK(text(canvas, L"Locked Door") == nullptr);
	CHECK(text(canvas, L"Pick") == nullptr);
	REQUIRE(text(canvas, L"Supply Crate") != nullptr);
	CHECK(text(canvas, L"Supply Crate")->colour.getAlpha() == 255);
	CHECK(text(canvas, L"Smash") != nullptr);
}

TEST_CASE("a target with a single slot gives it one gentle glow just after it pops in, without picking it; two slots get none")
{
	DpadHud dpad;
	dpad.setTarget("Supply Crate", "interactables/supply-crate.png");
	dpad.setSlot(DpadDirection::LEFT, "Smash", "approach/force.png");
	dpad.setState(DpadState::ACTIVE);
	RecordingCanvas canvas;
	dpad.update(1.0f);
	dpad.render(canvas);
	int settled = count(canvas, RecordingCanvas::LINE);
	DpadHud fresh;
	fresh.setTarget("Supply Crate", "interactables/supply-crate.png");
	fresh.setSlot(DpadDirection::LEFT, "Smash", "approach/force.png");
	fresh.setState(DpadState::ACTIVE);
	int most = 0;
	for (int frame = 0; frame < 40; ++frame)
	{
		fresh.update(0.025f);
		canvas.calls.clear();
		fresh.render(canvas);
		most = std::max(most, count(canvas, RecordingCanvas::LINE));
	}
	CHECK(most >= settled + 3 * 32);

	DpadHud pair = active();
	int pairMost = 0, pairBase = 0;
	pair.setTarget("Other", "interactables/supply-crate.png");
	for (int frame = 0; frame < 40; ++frame)
	{
		pair.update(0.025f);
		canvas.calls.clear();
		pair.render(canvas);
		pairMost = std::max(pairMost, count(canvas, RecordingCanvas::LINE));
	}
	canvas.calls.clear();
	pair.render(canvas);
	pairBase = count(canvas, RecordingCanvas::LINE);
	CHECK(pairMost < pairBase + 3 * 32);
}
