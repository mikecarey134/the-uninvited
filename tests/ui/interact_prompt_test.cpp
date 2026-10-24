#include "doctest.h"
#include "InteractPrompt.h"
#include "RecordingCanvas.h"
#include <algorithm>
#include <cstdlib>
#include <string>

using namespace irr;
using namespace core;

namespace
{
	const RecordingCanvas::Call* text(const RecordingCanvas& canvas, const std::wstring& line)
	{
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::TEXT && call.text == line)
				return &call;
		return nullptr;
	}
}

TEST_CASE("the interact prompt is the button's glyph image, then the verb, on a dark pill centred under the anchor")
{
	RecordingCanvas canvas;
	canvas.textDimension = dimension2d<u32>(60, 16);
	drawInteractPrompt(canvas, position2d<s32>(500, 300), "assets/textures/glyphs/kb/key_e.png", "Search");
	const RecordingCanvas::Call* verb = text(canvas, L"Search");
	REQUIRE(verb != nullptr);
	CHECK(text(canvas, L"E") == nullptr);
	const RecordingCanvas::Call* glyph = nullptr;
	for (const RecordingCanvas::Call& call : canvas.calls)
		if (call.kind == RecordingCanvas::IMAGE)
			glyph = &call;
	REQUIRE(glyph != nullptr);
	CHECK(canvas.pathOf(glyph->texture) == "assets/textures/glyphs/kb/key_e.png");
	CHECK(glyph->area.getWidth() == glyph->area.getHeight());
	CHECK(glyph->area.getWidth() >= 24);
	CHECK(glyph->area.getWidth() <= 32);
	CHECK(glyph->area.LowerRightCorner.X <= verb->area.UpperLeftCorner.X);
	CHECK(verb->colour.getRed() > 200);

	REQUIRE(canvas.calls.front().kind == RecordingCanvas::RECT);
	const RecordingCanvas::Call& pill = canvas.calls.front();
	CHECK(pill.colour.getAlpha() < 255);
	CHECK(pill.colour.getRed() == 0);
	CHECK(pill.area.isPointInside(glyph->area.getCenter()));
	CHECK(pill.area.isPointInside(verb->area.getCenter()));
	CHECK(std::abs(pill.area.getCenter().X - 500) <= 2);
}

TEST_CASE("a different glyph is just a different image in the same place")
{
	RecordingCanvas keyboard, pad;
	drawInteractPrompt(keyboard, position2d<s32>(500, 300), "assets/textures/glyphs/kb/key_e.png", "Open");
	drawInteractPrompt(pad, position2d<s32>(500, 300), "assets/textures/glyphs/xbox/x.png", "Open");
	REQUIRE(keyboard.calls.size() == pad.calls.size());
	for (size_t i = 0; i < pad.calls.size(); ++i)
		if (pad.calls[i].kind == RecordingCanvas::IMAGE)
		{
			CHECK(pad.pathOf(pad.calls[i].texture) == "assets/textures/glyphs/xbox/x.png");
			CHECK(pad.calls[i].area == keyboard.calls[i].area);
		}
}

TEST_CASE("with nothing to do there is no prompt at all")
{
	RecordingCanvas canvas;
	drawInteractPrompt(canvas, position2d<s32>(500, 300), "E", "");
	CHECK(canvas.calls.empty());
}
