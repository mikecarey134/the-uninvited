#include "doctest.h"
#include "ContextPanel.h"
#include "IconArt.h"
#include "RecordingCanvas.h"
#include <string>

using namespace irr;
using namespace core;

namespace
{
	const std::vector<ContextRow> doorRows = { { 1, "modes/normal.png", "Try Handle" }, { 2, "modes/force.png", "Kick Down" },
		{ 3, "modes/skill.png", "Pick Lock" }, { 4, "modes/guile.png", "Send Companion" } };

	int texts(const RecordingCanvas& canvas, const std::wstring& text)
	{
		int count = 0;
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::TEXT && call.text == text)
				++count;
		return count;
	}

	bool drew(const RecordingCanvas& canvas, const std::string& path)
	{
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::IMAGE && canvas.pathOf(call.texture) == path)
				return true;
		return false;
	}
}

TEST_CASE("a closed panel draws nothing")
{
	ContextPanel panel;
	RecordingCanvas canvas;
	panel.render(canvas);
	CHECK(canvas.calls.empty());
	panel.open(doorRows);
	panel.close();
	panel.render(canvas);
	CHECK(canvas.calls.empty());
}

TEST_CASE("each row shows its mode icon, its label and its number key, on a see-through dark panel just below the screen centre")
{
	ContextPanel panel;
	panel.open(doorRows);
	RecordingCanvas canvas;
	canvas.screenDimension = dimension2d<u32>(1920, 1080);
	panel.render(canvas);
	REQUIRE_FALSE(canvas.calls.empty());
	const RecordingCanvas::Call& back = canvas.calls.front();
	REQUIRE(back.kind == RecordingCanvas::RECT);
	CHECK(back.colour.getAlpha() < 255);
	CHECK(back.colour.getAlpha() > 100);
	CHECK(back.area.getCenter().X == 960);
	CHECK(back.area.UpperLeftCorner.Y > 540);
	CHECK(back.area.UpperLeftCorner.Y < 640);
	for (const ContextRow& row : doorRows)
	{
		CHECK(texts(canvas, std::wstring(row.label.begin(), row.label.end())) == 1);
		CHECK(texts(canvas, std::to_wstring(row.number)) == 1);
		CHECK(drew(canvas, "assets/ui/icons/" + row.iconPath));
	}
}

TEST_CASE("the row under the cursor is found and highlighted; off the rows nothing is")
{
	ContextPanel panel;
	panel.open(doorRows);
	RecordingCanvas sized;
	sized.screenDimension = dimension2d<u32>(1920, 1080);
	panel.render(sized);
	position2d<s32> second = panel.rowArea(1).getCenter();
	CHECK(panel.rowAt(second) == 1);
	CHECK(panel.rowAt(position2d<s32>(10, 10)) == -1);
	CHECK(panel.rowAt(panel.rowArea(4).getCenter()) == -1);
	panel.hover(second);
	CHECK(panel.hovered() == 1);
	RecordingCanvas canvas;
	canvas.screenDimension = dimension2d<u32>(1920, 1080);
	panel.render(canvas);
	int highlights = 0;
	for (const RecordingCanvas::Call& call : canvas.calls)
		if (call.kind == RecordingCanvas::RECT && call.area == panel.rowArea(1))
			++highlights;
	CHECK(highlights == 1);
	panel.hover(position2d<s32>(10, 10));
	CHECK(panel.hovered() == -1);
	panel.close();
	CHECK(panel.rowAt(second) == -1);
}

TEST_CASE("each row's mode glyph is drawn about 24px across as seen, whatever padding its PNG carries")
{
	ContextPanel panel;
	panel.open(doorRows);
	RecordingCanvas canvas;
	canvas.screenDimension = dimension2d<u32>(1920, 1080);
	panel.render(canvas);
	int glyphs = 0;
	for (const RecordingCanvas::Call& call : canvas.calls)
		if (call.kind == RecordingCanvas::IMAGE)
		{
			std::string path = canvas.pathOf(call.texture).substr(std::string("assets/ui/icons/").size());
			CAPTURE(path);
			CHECK(call.area.getWidth() * artBox(path).fill == doctest::Approx(24.0f).epsilon(0.08));
			++glyphs;
		}
	CHECK(glyphs == 4);
}
