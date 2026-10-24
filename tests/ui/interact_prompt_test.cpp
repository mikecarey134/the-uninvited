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

TEST_CASE("the interact prompt is a light keycap holding the key, then the verb, on a dark pill centred under the anchor")
{
	RecordingCanvas canvas;
	canvas.textDimension = dimension2d<u32>(60, 16);
	drawInteractPrompt(canvas, position2d<s32>(500, 300), "E", "Search");
	const RecordingCanvas::Call* key = text(canvas, L"E");
	const RecordingCanvas::Call* verb = text(canvas, L"Search");
	REQUIRE(key != nullptr);
	REQUIRE(verb != nullptr);
	CHECK(key->area.LowerRightCorner.X <= verb->area.UpperLeftCorner.X);
	CHECK(key->colour.getRed() < 60);
	CHECK(verb->colour.getRed() > 200);

	REQUIRE(canvas.calls.front().kind == RecordingCanvas::RECT);
	const RecordingCanvas::Call& pill = canvas.calls.front();
	CHECK(pill.colour.getAlpha() < 255);
	CHECK(pill.colour.getRed() == 0);
	CHECK(pill.area.isPointInside(key->area.getCenter()));
	CHECK(pill.area.isPointInside(verb->area.getCenter()));
	CHECK(std::abs(pill.area.getCenter().X - 500) <= 2);
	CHECK(pill.area.UpperLeftCorner.Y < 300);

	// The keycap: a light face that reaches the cap's top edge, sitting on a darker lip that shows below it.
	s32 capTop = 10000, faceTop = 10000, faceBottom = 0, lipBottom = 0;
	for (const RecordingCanvas::Call& call : canvas.calls)
	{
		if (call.kind != RecordingCanvas::RECT || &call == &pill || !call.area.isPointInside(key->area.getCenter()))
			continue;
		capTop = std::min(capTop, call.area.UpperLeftCorner.Y);
		if (call.colour.getRed() > 200)
		{
			faceTop = std::min(faceTop, call.area.UpperLeftCorner.Y);
			faceBottom = std::max(faceBottom, call.area.LowerRightCorner.Y);
		}
		else
			lipBottom = std::max(lipBottom, call.area.LowerRightCorner.Y);
	}
	CHECK(faceTop == capTop);
	CHECK(lipBottom > faceBottom);
}

TEST_CASE("with nothing to do there is no prompt at all")
{
	RecordingCanvas canvas;
	drawInteractPrompt(canvas, position2d<s32>(500, 300), "E", "");
	CHECK(canvas.calls.empty());
}
