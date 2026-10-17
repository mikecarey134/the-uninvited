#include "doctest.h"
#include "LootScreen.h"
#include "RecordingCanvas.h"
#include "Theme.h"
#include <memory>
#include <string>

using namespace irr;
using namespace core;
using namespace inventory;

namespace
{
	struct Search
	{
		Search() : chest{ "Chest", 0.0f, 0.0f, 8, { Item{ "shotgun", 1 }, Item{ "body_armor", 1 }, Item{ "pills", 4 } } },
			screen([this] { return console; }, [this] { ++opened; }, [this] { return near ? &chest : nullptr; })
		{
			canvas.screenDimension = dimension2d<u32>(1280, 720);
			std::string problem;
			REQUIRE(backpack::loadCatalog(catalog, problem));
			carried.reset(new Inventory(catalog, backpack::COLUMNS, backpack::ROWS));
			screen.attach(carried.get(), &catalog);
		}

		bool key(EKEY_CODE code, bool down = true)
		{
			SEvent event;
			event.EventType = EET_KEY_INPUT_EVENT;
			event.KeyInput.Key = code;
			event.KeyInput.PressedDown = down;
			event.KeyInput.Char = 0;
			event.KeyInput.Shift = false;
			event.KeyInput.Control = false;
			return screen.onEvent(event);
		}

		bool mouse(EMOUSE_INPUT_EVENT kind, position2d<s32> at, f32 wheel = 0.0f)
		{
			SEvent event;
			event.EventType = EET_MOUSE_INPUT_EVENT;
			event.MouseInput.Event = kind;
			event.MouseInput.X = at.X;
			event.MouseInput.Y = at.Y;
			event.MouseInput.Wheel = wheel;
			event.MouseInput.ButtonStates = 0;
			event.MouseInput.Shift = false;
			event.MouseInput.Control = false;
			return screen.onEvent(event);
		}

		void draw()
		{
			canvas.calls.clear();
			screen.draw(canvas);
		}

		const RecordingCanvas::Call* text(const std::wstring& line)
		{
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::TEXT && call.text == line)
					return &call;
			return nullptr;
		}

		bool console = false;
		bool near = false;
		int opened = 0;
		loot::Container chest;
		RecordingCanvas canvas;
		Catalog catalog;
		std::unique_ptr<Inventory> carried;
		LootScreen screen;
	};
}

TEST_CASE("E opens the loot screen only beside a container, marks it searched, and E or Escape closes it")
{
	Search search;
	CHECK_FALSE(search.key(KEY_KEY_E));
	CHECK_FALSE(search.screen.isOpen());
	search.near = true;
	search.console = true;
	CHECK_FALSE(search.key(KEY_KEY_E));
	CHECK_FALSE(search.screen.isOpen());
	search.console = false;
	CHECK(search.key(KEY_KEY_E));
	CHECK(search.screen.isOpen());
	CHECK(search.screen.container() == &search.chest);
	CHECK(search.chest.searched);
	CHECK(search.opened == 1);
	CHECK(search.key(KEY_KEY_E));
	CHECK_FALSE(search.screen.isOpen());
	search.key(KEY_KEY_E);
	CHECK(search.key(KEY_ESCAPE));
	CHECK_FALSE(search.screen.isOpen());
	CHECK_FALSE(search.key(KEY_ESCAPE));
	CHECK(search.opened == 2);
}

TEST_CASE("the open loot screen swallows movement and the mouse but lets I, C and the console key through")
{
	Search search;
	search.near = true;
	search.key(KEY_KEY_E);
	CHECK(search.key(KEY_KEY_W));
	CHECK_FALSE(search.key(KEY_KEY_W, false));
	CHECK_FALSE(search.key(KEY_KEY_I));
	CHECK_FALSE(search.key(KEY_KEY_C));
	CHECK_FALSE(search.key(KEY_OEM_3));
	CHECK(search.mouse(EMIE_MOUSE_MOVED, position2d<s32>(3, 3)));
	search.screen.close();
	CHECK_FALSE(search.mouse(EMIE_LMOUSE_PRESSED_DOWN, position2d<s32>(3, 3)));
}

TEST_CASE("the loot screen is titled with the container's kind and lists each item with its icon, name and count beside the player's doll and grid")
{
	Search search;
	search.near = true;
	search.key(KEY_KEY_E);
	search.draw();
	const RecordingCanvas::Call& dim = search.canvas.calls.front();
	CHECK(dim.area == rect<s32>(0, 0, 1280, 720));
	CHECK(dim.colour.getAlpha() >= 120);
	REQUIRE(search.text(L"Chest") != nullptr);
	REQUIRE(search.text(L"3 / 8") != nullptr);
	REQUIRE(search.text(L"CONTENTS") != nullptr);
	CHECK(search.text(L"3 / 8")->area.UpperLeftCorner.X > search.text(L"CONTENTS")->area.UpperLeftCorner.X + 100);
	const RecordingCanvas::Call* shotgun = search.text(L"Shotgun");
	const RecordingCanvas::Call* armour = search.text(L"Body Armor");
	const RecordingCanvas::Call* pills = search.text(L"Pills");
	REQUIRE(shotgun != nullptr);
	REQUIRE(armour != nullptr);
	REQUIRE(pills != nullptr);
	CHECK(search.screen.row(0).isRectCollided(shotgun->area));
	CHECK(search.screen.row(1).isRectCollided(armour->area));
	REQUIRE(search.text(L"x4") != nullptr);
	CHECK(search.screen.row(2).isRectCollided(search.text(L"x4")->area));
	CHECK(search.text(L"x1") == nullptr);
	bool icon = false;
	for (const RecordingCanvas::Call& call : search.canvas.calls)
		icon = icon || (call.kind == RecordingCanvas::IMAGE && search.canvas.pathOf(call.texture) == std::string(backpack::ICON_FOLDER) + "shotgun.png" &&
			search.screen.row(0).isRectCollided(call.area));
	CHECK(icon);

	backpack::Layout layout = search.screen.layout();
	rect<s32> panel = search.screen.panel();
	CHECK(search.screen.list().UpperLeftCorner.X >= panel.UpperLeftCorner.X);
	for (const rect<s32>& slot : layout.slots)
		CHECK(slot.UpperLeftCorner.X > search.screen.list().LowerRightCorner.X);
	CHECK(layout.grid.UpperLeftCorner.X > layout.slots[0].LowerRightCorner.X);
	CHECK(layout.grid.LowerRightCorner.X <= panel.LowerRightCorner.X);
	CHECK(layout.grid.LowerRightCorner.Y <= panel.LowerRightCorner.Y);
	CHECK(layout.cell >= 32);
	int lines = 0;
	for (const RecordingCanvas::Call& call : search.canvas.calls)
		if (call.kind == RecordingCanvas::LINE && layout.grid.isPointInside(call.from) && layout.grid.isPointInside(call.to))
			++lines;
	CHECK(lines == backpack::COLUMNS + backpack::ROWS - 2);
}

TEST_CASE("clicking a row takes it into the grid, clicking a grid item puts it in the container, and refusals show in the status line")
{
	Search search;
	search.near = true;
	search.key(KEY_KEY_E);
	search.draw();
	search.mouse(EMIE_LMOUSE_PRESSED_DOWN, search.screen.row(1).getCenter());
	REQUIRE(search.chest.items.size() == 2);
	CHECK(search.chest.items[1].type == "pills");
	REQUIRE(search.carried->itemAt(Cell{ 0, 0 }) != nullptr);
	CHECK(search.carried->itemAt(Cell{ 0, 0 })->type == "body_armor");

	backpack::Layout layout = search.screen.layout();
	search.mouse(EMIE_LMOUSE_PRESSED_DOWN, layout.cellArea(Cell{ 1, 2 }).getCenter());
	CHECK(search.carried->itemAt(Cell{ 0, 0 }) == nullptr);
	REQUIRE(search.chest.items.size() == 3);
	CHECK(search.chest.items[2].type == "body_armor");

	search.chest.capacity = 3;
	REQUIRE(search.carried->placeAt(Item{ "key", 1 }, Cell{ 4, 4 }).outcome == Outcome::Done);
	search.mouse(EMIE_LMOUSE_PRESSED_DOWN, layout.cellArea(Cell{ 4, 4 }).getCenter());
	CHECK(search.screen.message() == "Container full.");
	search.draw();
	const RecordingCanvas::Call* full = search.text(L"Container full.");
	REQUIRE(full != nullptr);
	CHECK(full->area.UpperLeftCorner.Y >= search.screen.list().UpperLeftCorner.Y);

	for (int column = 0; column < backpack::COLUMNS; ++column)
		for (int row = 0; row < backpack::ROWS; ++row)
			search.carried->placeAt(Item{ "key", 1 }, Cell{ column, row });
	search.mouse(EMIE_LMOUSE_PRESSED_DOWN, search.screen.row(0).getCenter());
	CHECK(search.screen.message() == "Inventory full.");
	CHECK(search.chest.items.size() == 3);
	search.mouse(EMIE_LMOUSE_PRESSED_DOWN, position2d<s32>(search.screen.panel().UpperLeftCorner.X + 5, search.screen.panel().UpperLeftCorner.Y + 5));
	CHECK(search.chest.items.size() == 3);
}

TEST_CASE("a long container list scrolls with the wheel, and clicks follow the scroll")
{
	Search search;
	search.chest.capacity = 30;
	search.chest.items.clear();
	for (int i = 0; i < 20; ++i)
		search.chest.items.push_back(Item{ i == 15 ? "shotgun" : "key", 1 });
	search.near = true;
	search.key(KEY_KEY_E);
	search.draw();
	CHECK(search.text(L"Shotgun") == nullptr);
	for (int i = 0; i < 10; ++i)
		search.mouse(EMIE_MOUSE_WHEEL, search.screen.list().getCenter(), -1.0f);
	search.draw();
	const RecordingCanvas::Call* shotgun = search.text(L"Shotgun");
	REQUIRE(shotgun != nullptr);
	int shown = -1;
	for (int i = 0; i < 20; ++i)
		if (search.screen.row(i).isRectCollided(shotgun->area))
			shown = i;
	REQUIRE(shown >= 0);
	CHECK(search.screen.list().isPointInside(search.screen.row(shown).UpperLeftCorner));
	CHECK(search.screen.list().isPointInside(search.screen.row(shown).LowerRightCorner));
	search.mouse(EMIE_LMOUSE_PRESSED_DOWN, search.screen.row(shown).getCenter());
	CHECK(search.chest.items.size() == 19);
	bool taken = false;
	for (const Placed& placed : search.carried->backpackContents())
		taken = taken || placed.item.type == "shotgun";
	CHECK(taken);
	for (int i = 0; i < 40; ++i)
		search.mouse(EMIE_MOUSE_WHEEL, search.screen.list().getCenter(), 1.0f);
	search.draw();
	CHECK(search.screen.list().isPointInside(search.screen.row(0).UpperLeftCorner));
	CHECK(search.screen.list().isPointInside(search.screen.row(0).LowerRightCorner));
}

namespace
{
	bool ringed(const RecordingCanvas& canvas, const rect<s32>& around)
	{
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::RECT_OUTLINE && call.colour == theme::accent && call.area.UpperLeftCorner.X <= around.UpperLeftCorner.X &&
				call.area.UpperLeftCorner.Y <= around.UpperLeftCorner.Y && call.area.LowerRightCorner.X >= around.LowerRightCorner.X &&
				call.area.LowerRightCorner.Y >= around.LowerRightCorner.Y && call.area.getWidth() <= around.getWidth() + 8)
				return true;
		return false;
	}
}

TEST_CASE("with the keyboard alone the list and the grid take turns with Tab or the arrows, and Enter moves the highlighted stack either way")
{
	Search search;
	search.near = true;
	search.key(KEY_KEY_E);
	search.draw();
	CHECK(ringed(search.canvas, search.screen.row(0)));
	CHECK(search.key(KEY_DOWN));
	search.draw();
	CHECK(ringed(search.canvas, search.screen.row(1)));
	CHECK_FALSE(ringed(search.canvas, search.screen.row(0)));
	CHECK(search.key(KEY_RETURN));
	REQUIRE(search.chest.items.size() == 2);
	CHECK(search.chest.items[1].type == "pills");
	CHECK(search.carried->itemAt(Cell{ 0, 0 }) != nullptr);
	search.key(KEY_DOWN);
	search.key(KEY_DOWN);
	search.draw();
	CHECK(ringed(search.canvas, search.screen.row(1)));

	CHECK(search.key(KEY_TAB));
	search.draw();
	backpack::Layout layout = search.screen.layout();
	CHECK(ringed(search.canvas, layout.cellArea(Cell{ 0, 0 })));
	CHECK_FALSE(ringed(search.canvas, search.screen.row(1)));
	CHECK(search.key(KEY_SPACE));
	CHECK(search.chest.items.size() == 3);
	CHECK(search.carried->itemAt(Cell{ 0, 0 }) == nullptr);
	search.key(KEY_RIGHT);
	search.key(KEY_DOWN);
	search.key(KEY_UP);
	search.key(KEY_UP);
	search.draw();
	CHECK(ringed(search.canvas, layout.cellArea(Cell{ 1, 0 })));
	search.key(KEY_LEFT);
	search.key(KEY_LEFT);
	search.draw();
	CHECK(ringed(search.canvas, search.screen.row(1)));
	search.key(KEY_RIGHT);
	search.draw();
	CHECK(ringed(search.canvas, layout.cellArea(Cell{ 0, 0 })));
	search.key(KEY_TAB);
	search.draw();
	CHECK(ringed(search.canvas, search.screen.row(1)));

	search.mouse(EMIE_MOUSE_MOVED, search.screen.list().getCenter());
	search.draw();
	CHECK_FALSE(ringed(search.canvas, search.screen.row(1)));
}

TEST_CASE("walking the highlight down a long list scrolls it so the highlighted row stays in view")
{
	Search search;
	search.chest.capacity = 30;
	search.chest.items.clear();
	for (int i = 0; i < 20; ++i)
		search.chest.items.push_back(Item{ i == 15 ? "shotgun" : "key", 1 });
	search.near = true;
	search.key(KEY_KEY_E);
	for (int i = 0; i < 15; ++i)
		search.key(KEY_DOWN);
	search.draw();
	CHECK(search.text(L"Shotgun") != nullptr);
	CHECK(ringed(search.canvas, search.screen.row(15)));
	CHECK(search.screen.list().isPointInside(search.screen.row(15).LowerRightCorner));
	search.key(KEY_RETURN);
	CHECK(search.chest.items.size() == 19);
	for (int i = 0; i < 30; ++i)
		search.key(KEY_UP);
	search.draw();
	CHECK(ringed(search.canvas, search.screen.row(0)));
	CHECK(search.screen.list().isPointInside(search.screen.row(0).UpperLeftCorner));
}
