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

namespace
{
	bool inside(const rect<s32>& outer, const rect<s32>& inner)
	{
		return outer.isPointInside(inner.UpperLeftCorner) && inner.LowerRightCorner.X <= outer.LowerRightCorner.X &&
			inner.LowerRightCorner.Y <= outer.LowerRightCorner.Y;
	}

	bool ringed(const RecordingCanvas& canvas, const rect<s32>& around)
	{
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::RECT_OUTLINE && call.colour == theme::accent && call.area.UpperLeftCorner.X <= around.UpperLeftCorner.X &&
				call.area.UpperLeftCorner.Y <= around.UpperLeftCorner.Y && call.area.LowerRightCorner.X >= around.LowerRightCorner.X &&
				call.area.LowerRightCorner.Y >= around.LowerRightCorner.Y && call.area.getWidth() <= around.getWidth() + 8)
				return true;
		return false;
	}

	const RecordingCanvas::Call* icon(const RecordingCanvas& canvas, const std::string& file)
	{
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::IMAGE && canvas.pathOf(call.texture) == std::string(backpack::ICON_FOLDER) + file)
				return &call;
		return nullptr;
	}
}

TEST_CASE("two floating panels sit over a dimmed world, the container's icon grid on the left and the player's inventory on the right")
{
	Search search;
	search.near = true;
	search.key(KEY_KEY_E);
	search.draw();
	const RecordingCanvas::Call& dim = search.canvas.calls.front();
	CHECK(dim.area == rect<s32>(0, 0, 1280, 720));
	CHECK(dim.colour.getAlpha() >= 60);
	CHECK(dim.colour.getAlpha() <= 140);

	rect<s32> box = search.screen.containerPanel();
	rect<s32> bag = search.screen.playerPanel();
	rect<s32> screen(0, 0, 1280, 720);
	CHECK(inside(screen, box));
	CHECK(inside(screen, bag));
	CHECK(box.LowerRightCorner.X + 16 <= bag.UpperLeftCorner.X);
	CHECK(box.getWidth() < bag.getWidth());
	CHECK(box.getHeight() < bag.getHeight());
	CHECK(std::abs(box.getCenter().Y - 360) <= 1);
	CHECK(std::abs(bag.getCenter().Y - 360) <= 1);
	CHECK(std::abs(box.UpperLeftCorner.X - (1280 - bag.LowerRightCorner.X)) <= 1);
	REQUIRE(search.text(L"Chest") != nullptr);
	REQUIRE(search.text(L"Inventory") != nullptr);
	CHECK(box.isPointInside(search.text(L"Chest")->area.UpperLeftCorner));
	CHECK(bag.isPointInside(search.text(L"Inventory")->area.UpperLeftCorner));

	CHECK(search.screen.containerColumns() == 4);
	CHECK(search.screen.containerRows() == 2);
	for (int i = 0; i < 8; ++i)
	{
		INFO(i);
		rect<s32> cell = search.screen.containerCell(i);
		CHECK(cell.getWidth() == cell.getHeight());
		CHECK(cell.getWidth() >= 40);
		CHECK(inside(box, cell));
		for (int j = 0; j < i; ++j)
			CHECK_FALSE(cell.isRectCollided(search.screen.containerCell(j)));
		if (i > 0 && i % 4 != 0)
			CHECK(cell.UpperLeftCorner.X > search.screen.containerCell(i - 1).UpperLeftCorner.X);
	}
	CHECK(search.screen.containerCell(4).UpperLeftCorner.Y > search.screen.containerCell(0).UpperLeftCorner.Y);
	const RecordingCanvas::Call* shotgun = icon(search.canvas, "shotgun.png");
	REQUIRE(shotgun != nullptr);
	CHECK(inside(search.screen.containerCell(0), shotgun->area));
	REQUIRE(icon(search.canvas, "pills.png") != nullptr);
	CHECK(inside(search.screen.containerCell(2), icon(search.canvas, "pills.png")->area));
	REQUIRE(search.text(L"4") != nullptr);
	CHECK(inside(search.screen.containerCell(2), search.text(L"4")->area));
	CHECK(search.text(L"1") == nullptr);
	CHECK(search.text(L"Shotgun") == nullptr);
	int emptyCells = 0;
	for (const RecordingCanvas::Call& call : search.canvas.calls)
		for (int i = 3; i < 8; ++i)
			if (call.kind == RecordingCanvas::RECT && call.area == search.screen.containerCell(i))
				++emptyCells;
	CHECK(emptyCells == 5);

	backpack::Layout layout = search.screen.layout();
	CHECK(inside(bag, layout.grid));
	for (const rect<s32>& slot : layout.slots)
	{
		CHECK(inside(bag, slot));
		CHECK(slot.LowerRightCorner.X < layout.grid.UpperLeftCorner.X);
	}
	CHECK(layout.cell >= 32);
	int lines = 0;
	for (const RecordingCanvas::Call& call : search.canvas.calls)
		if (call.kind == RecordingCanvas::LINE && layout.grid.isPointInside(call.from) && layout.grid.isPointInside(call.to))
			++lines;
	CHECK(lines == backpack::COLUMNS + backpack::ROWS - 2);

	search.canvas.screenDimension = dimension2d<u32>(1920, 1080);
	search.draw();
	box = search.screen.containerPanel();
	bag = search.screen.playerPanel();
	CHECK(box.UpperLeftCorner.X > 100);
	CHECK(std::abs(box.UpperLeftCorner.X - (1920 - bag.LowerRightCorner.X)) <= 1);
	CHECK(std::abs(box.getCenter().Y - 540) <= 1);
	CHECK(std::abs(bag.getCenter().Y - 540) <= 1);
}

TEST_CASE("the container grid is as wide as its capacity up to four columns, with rows added as needed, and still fits the screen")
{
	Search search;
	search.near = true;
	const struct { int capacity; int columns; int rows; } sizes[] = { { 4, 2, 2 }, { 8, 4, 2 }, { 1, 1, 1 }, { 3, 2, 2 }, { 10, 4, 3 }, { 30, 4, 8 } };
	for (const auto& size : sizes)
	{
		INFO(size.capacity);
		search.chest.capacity = size.capacity;
		search.screen.close();
		search.key(KEY_KEY_E);
		search.draw();
		CHECK(search.screen.containerColumns() == size.columns);
		CHECK(search.screen.containerRows() == size.rows);
		CHECK(inside(search.screen.containerPanel(), search.screen.containerCell(size.capacity - 1)));
		CHECK(inside(rect<s32>(0, 0, 1280, 720), search.screen.containerPanel()));
		CHECK(search.screen.containerPanel().LowerRightCorner.X + 16 <= search.screen.playerPanel().UpperLeftCorner.X);
	}
}

TEST_CASE("clicking a container icon takes it into the grid, clicking a grid item puts it in the container, and refusals show under the grid")
{
	Search search;
	search.near = true;
	search.key(KEY_KEY_E);
	search.draw();
	search.mouse(EMIE_LMOUSE_PRESSED_DOWN, search.screen.containerCell(1).getCenter());
	REQUIRE(search.chest.items.size() == 2);
	CHECK(search.chest.items[1].type == "pills");
	REQUIRE(search.carried->itemAt(Cell{ 0, 0 }) != nullptr);
	CHECK(search.carried->itemAt(Cell{ 0, 0 })->type == "body_armor");
	search.mouse(EMIE_LMOUSE_PRESSED_DOWN, search.screen.containerCell(6).getCenter());
	CHECK(search.chest.items.size() == 2);

	backpack::Layout layout = search.screen.layout();
	search.mouse(EMIE_LMOUSE_PRESSED_DOWN, layout.cellArea(Cell{ 1, 2 }).getCenter());
	CHECK(search.carried->itemAt(Cell{ 0, 0 }) == nullptr);
	REQUIRE(search.chest.items.size() == 3);
	CHECK(search.chest.items[2].type == "body_armor");

	REQUIRE(search.carried->placeAt(Item{ "pills", 3 }, Cell{ 6, 0 }).outcome == Outcome::Done);
	search.mouse(EMIE_LMOUSE_PRESSED_DOWN, search.screen.containerCell(1).getCenter());
	REQUIRE(search.carried->itemAt(Cell{ 6, 0 }) != nullptr);
	CHECK(search.carried->itemAt(Cell{ 6, 0 })->count == 7);
	CHECK(search.chest.items.size() == 2);

	search.chest.capacity = 2;
	backpack::Layout narrower = search.screen.layout();
	REQUIRE(search.carried->placeAt(Item{ "key", 1 }, Cell{ 4, 4 }).outcome == Outcome::Done);
	search.mouse(EMIE_LMOUSE_PRESSED_DOWN, narrower.cellArea(Cell{ 4, 4 }).getCenter());
	CHECK(search.screen.message() == "Container full.");
	search.draw();
	const RecordingCanvas::Call* full = search.text(L"Container full.");
	REQUIRE(full != nullptr);
	CHECK(full->area.UpperLeftCorner.Y >= narrower.grid.LowerRightCorner.Y);
	CHECK(inside(search.screen.playerPanel(), full->area));

	for (int column = 0; column < backpack::COLUMNS; ++column)
		for (int row = 0; row < backpack::ROWS; ++row)
			search.carried->placeAt(Item{ "key", 1 }, Cell{ column, row });
	search.mouse(EMIE_LMOUSE_PRESSED_DOWN, search.screen.containerCell(0).getCenter());
	CHECK(search.screen.message() == "Inventory full.");
	CHECK(search.chest.items.size() == 2);
}

TEST_CASE("with the keyboard alone the two grids take turns with Tab or across their edges, arrows walk the cells, and Enter moves the stack either way")
{
	Search search;
	search.near = true;
	search.key(KEY_KEY_E);
	search.draw();
	CHECK(ringed(search.canvas, search.screen.containerCell(0)));
	CHECK(search.key(KEY_RIGHT));
	search.key(KEY_DOWN);
	search.draw();
	CHECK(ringed(search.canvas, search.screen.containerCell(5)));
	CHECK_FALSE(ringed(search.canvas, search.screen.containerCell(0)));
	CHECK(search.key(KEY_RETURN));
	CHECK(search.chest.items.size() == 3);
	search.key(KEY_DOWN);
	search.draw();
	CHECK(ringed(search.canvas, search.screen.containerCell(5)));
	search.key(KEY_LEFT);
	search.key(KEY_LEFT);
	search.draw();
	CHECK(ringed(search.canvas, search.screen.containerCell(4)));
	search.key(KEY_RIGHT);
	search.key(KEY_UP);
	search.key(KEY_UP);
	search.draw();
	CHECK(ringed(search.canvas, search.screen.containerCell(1)));
	search.key(KEY_LEFT);
	search.key(KEY_LEFT);
	search.draw();
	CHECK(ringed(search.canvas, search.screen.containerCell(0)));
	search.key(KEY_RIGHT);
	CHECK(search.key(KEY_SPACE));
	REQUIRE(search.chest.items.size() == 2);
	CHECK(search.carried->itemAt(Cell{ 0, 0 }) != nullptr);

	CHECK(search.key(KEY_TAB));
	search.draw();
	backpack::Layout layout = search.screen.layout();
	CHECK(ringed(search.canvas, layout.cellArea(Cell{ 0, 0 })));
	CHECK_FALSE(ringed(search.canvas, search.screen.containerCell(1)));
	CHECK(search.key(KEY_RETURN));
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
	CHECK(ringed(search.canvas, search.screen.containerCell(1)));
	search.key(KEY_RIGHT);
	search.key(KEY_RIGHT);
	search.key(KEY_RIGHT);
	search.draw();
	CHECK(ringed(search.canvas, layout.cellArea(Cell{ 0, 0 })));
	search.key(KEY_TAB);
	search.draw();
	CHECK(ringed(search.canvas, search.screen.containerCell(3)));

	search.mouse(EMIE_MOUSE_MOVED, search.screen.containerCell(6).getCenter());
	search.draw();
	CHECK_FALSE(ringed(search.canvas, search.screen.containerCell(3)));
	search.key(KEY_KEY_E);
	search.key(KEY_KEY_E);
	search.draw();
	CHECK(ringed(search.canvas, search.screen.containerCell(0)));
}

TEST_CASE("the loot screen can open on one given container even when no container is nearest, and marks it searched")
{
	Search search;
	loot::Container crate{ "Crate", 40.0f, 40.0f, 4, {} };
	search.screen.open(nullptr);
	CHECK_FALSE(search.screen.isOpen());
	search.screen.open(&crate);
	CHECK(search.screen.isOpen());
	CHECK(search.screen.container() == &crate);
	CHECK(crate.searched);
	CHECK(search.opened == 1);
}
