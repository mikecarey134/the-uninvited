#include "doctest.h"
#include "InventoryScreen.h"
#include "RecordingCanvas.h"
#include "Theme.h"
#include <memory>
#include <string>

using namespace irr;
using namespace core;
using namespace inventory;

namespace
{
	struct Bag
	{
		Bag() : screen([this] { return console; }, [this] { ++opened; })
		{
			canvas.screenDimension = dimension2d<u32>(1280, 720);
			std::string problem;
			REQUIRE(backpack::loadCatalog(catalog, problem));
			pack.reset(new Inventory(catalog, backpack::COLUMNS, backpack::ROWS));
			page.reset(new backpack::Page(*pack, catalog));
			screen.attach(page.get());
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

		bool mouse(EMOUSE_INPUT_EVENT kind, position2d<s32> at)
		{
			SEvent event;
			event.EventType = EET_MOUSE_INPUT_EVENT;
			event.MouseInput.Event = kind;
			event.MouseInput.X = at.X;
			event.MouseInput.Y = at.Y;
			event.MouseInput.Wheel = 0.0f;
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

		const RecordingCanvas::Call* image(const std::string& icon)
		{
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::IMAGE && canvas.pathOf(call.texture) == std::string(backpack::ICON_FOLDER) + icon)
					return &call;
			return nullptr;
		}

		const RecordingCanvas::Call* text(const std::wstring& line)
		{
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::TEXT && call.text == line)
					return &call;
			return nullptr;
		}

		bool console = false;
		int opened = 0;
		RecordingCanvas canvas;
		Catalog catalog;
		std::unique_ptr<Inventory> pack;
		std::unique_ptr<backpack::Page> page;
		InventoryScreen screen;
	};
}

TEST_CASE("I opens and closes the inventory screen, Escape closes it, and the console keeps I for typing")
{
	Bag bag;
	CHECK_FALSE(bag.screen.isOpen());
	CHECK_FALSE(bag.key(KEY_ESCAPE));
	CHECK(bag.key(KEY_KEY_I));
	CHECK(bag.screen.isOpen());
	CHECK(bag.opened == 1);
	CHECK(bag.key(KEY_KEY_I));
	CHECK_FALSE(bag.screen.isOpen());
	CHECK(bag.key(KEY_KEY_I));
	CHECK(bag.key(KEY_ESCAPE));
	CHECK_FALSE(bag.screen.isOpen());
	CHECK(bag.opened == 2);
	bag.console = true;
	CHECK_FALSE(bag.key(KEY_KEY_I));
	CHECK_FALSE(bag.screen.isOpen());
}

TEST_CASE("the open screen swallows movement keys and the mouse, but lets key releases, the console key and C for the notebook through")
{
	Bag bag;
	CHECK_FALSE(bag.key(KEY_KEY_W));
	CHECK_FALSE(bag.mouse(EMIE_LMOUSE_PRESSED_DOWN, position2d<s32>(5, 5)));
	bag.key(KEY_KEY_I);
	CHECK(bag.key(KEY_KEY_W));
	CHECK_FALSE(bag.key(KEY_KEY_W, false));
	CHECK_FALSE(bag.key(KEY_OEM_3));
	CHECK_FALSE(bag.key(KEY_KEY_C));
	CHECK(bag.mouse(EMIE_LMOUSE_PRESSED_DOWN, position2d<s32>(5, 5)));
	CHECK(bag.mouse(EMIE_MOUSE_MOVED, position2d<s32>(9, 9)));
}

TEST_CASE("the screen dims the game and draws one dark panel, the doll on the left and the grid on the right, without the notebook")
{
	Bag bag;
	bag.draw();
	CHECK(bag.canvas.calls.empty());
	bag.key(KEY_KEY_I);
	bag.draw();
	REQUIRE_FALSE(bag.canvas.calls.empty());
	const RecordingCanvas::Call& dim = bag.canvas.calls.front();
	CHECK(dim.kind == RecordingCanvas::RECT);
	CHECK(dim.area == rect<s32>(0, 0, 1280, 720));
	CHECK(dim.colour.getAlpha() >= 120);
	CHECK(dim.colour.getRed() < 30);
	rect<s32> panel = bag.screen.panel();
	CHECK(panel.UpperLeftCorner.X >= 16);
	CHECK(panel.LowerRightCorner.X <= 1280 - 16);
	CHECK(panel.UpperLeftCorner.Y >= 16);
	CHECK(panel.LowerRightCorner.Y <= 720 - 16);
	CHECK(panel.getWidth() >= 800);
	bool filled = false;
	for (const RecordingCanvas::Call& call : bag.canvas.calls)
		filled = filled || (call.kind == RecordingCanvas::RECT && call.area == panel && call.colour.getRed() < 40 && call.colour.getAlpha() > 200);
	CHECK(filled);
	REQUIRE(bag.text(L"INVENTORY") != nullptr);
	CHECK(bag.text(L"EQUIPMENT") != nullptr);
	CHECK(bag.text(L"BACKPACK") != nullptr);
	for (const RecordingCanvas::Call& call : bag.canvas.calls)
		if (call.kind == RecordingCanvas::IMAGE)
			CHECK(bag.canvas.pathOf(call.texture).find("notebook") == std::string::npos);

	backpack::Layout layout = bag.screen.layout();
	CHECK(layout.cell >= 48);
	CHECK(panel.isRectCollided(layout.grid));
	CHECK(layout.grid.LowerRightCorner.X <= panel.LowerRightCorner.X);
	CHECK(layout.grid.LowerRightCorner.Y <= panel.LowerRightCorner.Y);
	for (const rect<s32>& slot : layout.slots)
	{
		CHECK(slot.LowerRightCorner.X < layout.grid.UpperLeftCorner.X);
		CHECK(slot.UpperLeftCorner.X >= panel.UpperLeftCorner.X);
	}
}

TEST_CASE("empty cells are faintly lined, occupied cells stand out, icons sit inside their items, stacks show a count and the item in hand follows the mouse")
{
	Bag bag;
	REQUIRE(bag.pack->placeAt(Item{ "body_armor", 1, true }, Cell{ 0, 0 }).outcome == Outcome::Done);
	REQUIRE(bag.pack->placeAt(Item{ "bandage", 3 }, Cell{ 6, 4 }).outcome == Outcome::Done);
	REQUIRE(bag.pack->placeAt(Item{ "helmet", 1 }, Cell{ 4, 0 }).outcome == Outcome::Done);
	REQUIRE(bag.pack->equipFrom(Cell{ 4, 0 }, "head").outcome == Outcome::Done);
	bag.key(KEY_KEY_I);
	bag.draw();
	backpack::Layout layout = bag.screen.layout();

	int lines = 0;
	for (const RecordingCanvas::Call& call : bag.canvas.calls)
		if (call.kind == RecordingCanvas::LINE && layout.grid.isPointInside(call.from) && layout.grid.isPointInside(call.to))
		{
			++lines;
			CHECK(call.colour.getAlpha() <= 90);
		}
	CHECK(lines >= backpack::COLUMNS + backpack::ROWS - 2);

	rect<s32> armourCells = layout.footprint(Placed{ Item{ "body_armor", 1, true }, Cell{ 0, 0 } });
	const RecordingCanvas::Call* emptyFill = nullptr;
	const RecordingCanvas::Call* armourFill = nullptr;
	for (const RecordingCanvas::Call& call : bag.canvas.calls)
		if (call.kind == RecordingCanvas::RECT)
		{
			if (call.area == layout.grid)
				emptyFill = &call;
			if (call.area == armourCells)
				armourFill = &call;
		}
	REQUIRE(emptyFill != nullptr);
	REQUIRE(armourFill != nullptr);
	CHECK(armourFill->colour.color != emptyFill->colour.color);
	CHECK(armourFill->colour.getRed() > emptyFill->colour.getRed() + 15);

	const RecordingCanvas::Call* armour = bag.image("body_armor.png");
	REQUIRE(armour != nullptr);
	CHECK(armour->area.UpperLeftCorner.X >= armourCells.UpperLeftCorner.X);
	CHECK(armour->area.LowerRightCorner.X <= armourCells.LowerRightCorner.X);
	CHECK(armour->area.LowerRightCorner.Y <= armourCells.LowerRightCorner.Y);
	CHECK(armour->area.getWidth() == armour->area.getHeight());
	CHECK(armour->colour.getRed() > 200);
	const RecordingCanvas::Call* helmet = bag.image("helmet.png");
	REQUIRE(helmet != nullptr);
	CHECK(layout.slots[0].isRectCollided(helmet->area));
	const RecordingCanvas::Call* count = bag.text(L"3");
	REQUIRE(count != nullptr);
	CHECK(layout.cellArea(Cell{ 6, 4 }).isRectCollided(count->area));
	CHECK(bag.text(L"Main Hand") != nullptr);

	position2d<s32> bandage = layout.cellArea(Cell{ 6, 4 }).getCenter();
	bag.mouse(EMIE_LMOUSE_PRESSED_DOWN, bandage);
	REQUIRE(bag.page->isHolding());
	position2d<s32> elsewhere(layout.grid.UpperLeftCorner.X + 130, layout.grid.UpperLeftCorner.Y + 170);
	bag.mouse(EMIE_MOUSE_MOVED, elsewhere);
	bag.draw();
	REQUIRE(bag.image("bandage.png") != nullptr);
	CHECK(bag.image("bandage.png")->area.isPointInside(elsewhere));
}

TEST_CASE("on the screen a click picks up and puts down, R turns, right-click puts back, a refusal is shown, and closing returns what is in hand")
{
	Bag bag;
	REQUIRE(bag.pack->placeAt(Item{ "body_armor", 1 }, Cell{ 0, 0 }).outcome == Outcome::Done);
	REQUIRE(bag.pack->placeAt(Item{ "key", 1 }, Cell{ 5, 1 }).outcome == Outcome::Done);
	bag.key(KEY_KEY_I);
	bag.draw();
	backpack::Layout layout = bag.screen.layout();
	bag.mouse(EMIE_LMOUSE_PRESSED_DOWN, layout.cellArea(Cell{ 0, 0 }).getCenter());
	REQUIRE(bag.page->isHolding());
	CHECK(bag.key(KEY_KEY_R));
	CHECK(bag.page->held().rotated);
	bag.mouse(EMIE_LMOUSE_PRESSED_DOWN, layout.cellArea(Cell{ 4, 0 }).getCenter());
	CHECK(bag.page->isHolding());
	bag.draw();
	const RecordingCanvas::Call* reason = bag.text(L"Something is in the way");
	REQUIRE(reason != nullptr);
	CHECK(reason->area.UpperLeftCorner.Y >= layout.grid.LowerRightCorner.Y);
	CHECK(bag.mouse(EMIE_RMOUSE_PRESSED_DOWN, layout.cellArea(Cell{ 4, 0 }).getCenter()));
	CHECK_FALSE(bag.page->isHolding());
	CHECK_FALSE(bag.pack->itemAt(Cell{ 0, 0 })->rotated);

	bag.mouse(EMIE_MOUSE_MOVED, layout.cellArea(Cell{ 1, 1 }).getCenter());
	bag.key(KEY_KEY_R);
	CHECK(bag.pack->itemAt(Cell{ 0, 0 })->rotated);

	bag.mouse(EMIE_LMOUSE_PRESSED_DOWN, layout.cellArea(Cell{ 0, 0 }).getCenter());
	REQUIRE(bag.page->isHolding());
	bag.key(KEY_KEY_I);
	CHECK_FALSE(bag.screen.isOpen());
	CHECK_FALSE(bag.page->isHolding());
	CHECK(bag.pack->itemAt(Cell{ 0, 0 }) != nullptr);
}

TEST_CASE("without a backpack the screen still opens and says there is nothing to show")
{
	InventoryScreen screen([] { return false; }, [] {});
	RecordingCanvas canvas;
	canvas.screenDimension = dimension2d<u32>(1280, 720);
	screen.open();
	screen.draw(canvas);
	bool said = false;
	for (const RecordingCanvas::Call& call : canvas.calls)
		said = said || (call.kind == RecordingCanvas::TEXT && call.text == L"Inventory unavailable");
	CHECK(said);
}

TEST_CASE("after a backpack is put on, the screen draws the taller grid in the same panel")
{
	Bag bag;
	REQUIRE(bag.pack->placeAt(Item{ "large_backpack", 1 }, Cell{ 0, 0 }).outcome == Outcome::Done);
	REQUIRE(bag.pack->equipFrom(Cell{ 0, 0 }, "back").outcome == Outcome::Done);
	bag.key(KEY_KEY_I);
	bag.draw();
	backpack::Layout layout = bag.screen.layout();
	CHECK(layout.grid.getHeight() == 9 * layout.cell);
	CHECK(layout.cell >= 40);
	CHECK(layout.grid.LowerRightCorner.Y <= bag.screen.panel().LowerRightCorner.Y);
	int across = 0;
	for (const RecordingCanvas::Call& call : bag.canvas.calls)
		if (call.kind == RecordingCanvas::LINE && call.from.Y == call.to.Y && layout.grid.isPointInside(call.from))
			++across;
	CHECK(across == 8);
	REQUIRE(bag.image("large_backpack.png") != nullptr);
	CHECK(layout.slots[8].isRectCollided(bag.image("large_backpack.png")->area));
}

namespace
{
	const RecordingCanvas::Call* ring(const RecordingCanvas& canvas, const rect<s32>& around)
	{
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::RECT_OUTLINE && call.colour == theme::accent && call.area.UpperLeftCorner.X <= around.UpperLeftCorner.X &&
				call.area.UpperLeftCorner.Y <= around.UpperLeftCorner.Y && call.area.LowerRightCorner.X >= around.LowerRightCorner.X &&
				call.area.LowerRightCorner.Y >= around.LowerRightCorner.Y && call.area.getWidth() <= around.getWidth() + 8)
				return &call;
		return nullptr;
	}
}

TEST_CASE("with the keyboard alone the grid cursor picks up with Enter, the item in hand rides the cursor, Enter puts it down, and the doll equips it")
{
	Bag bag;
	REQUIRE(bag.pack->placeAt(Item{ "knife", 1 }, Cell{ 2, 1 }).outcome == Outcome::Done);
	bag.key(KEY_KEY_I);
	bag.draw();
	backpack::Layout layout = bag.screen.layout();
	CHECK(ring(bag.canvas, layout.cellArea(Cell{ 0, 0 })) != nullptr);

	CHECK(bag.key(KEY_RIGHT));
	bag.key(KEY_RIGHT);
	bag.key(KEY_DOWN);
	bag.key(KEY_DOWN);
	bag.draw();
	CHECK(ring(bag.canvas, layout.cellArea(Cell{ 2, 2 })) != nullptr);
	CHECK(ring(bag.canvas, layout.cellArea(Cell{ 0, 0 })) == nullptr);
	CHECK(bag.key(KEY_RETURN));
	REQUIRE(bag.page->isHolding());
	CHECK(bag.page->grabOffset().row == 1);

	bag.key(KEY_RIGHT);
	bag.key(KEY_RIGHT);
	bag.draw();
	const RecordingCanvas::Call* ghost = bag.image("knife.png");
	REQUIRE(ghost != nullptr);
	rect<s32> spot = layout.footprint(Placed{ Item{ "knife", 1 }, Cell{ 4, 1 } });
	CHECK(spot.isPointInside(ghost->area.UpperLeftCorner));
	CHECK(ghost->area.LowerRightCorner.X <= spot.LowerRightCorner.X);
	CHECK(ghost->area.LowerRightCorner.Y <= spot.LowerRightCorner.Y);
	CHECK(bag.key(KEY_KEY_R));
	CHECK(bag.page->held().rotated);
	bag.key(KEY_KEY_R);
	CHECK(bag.key(KEY_SPACE));
	CHECK_FALSE(bag.page->isHolding());
	REQUIRE(bag.pack->itemAt(Cell{ 4, 1 }) != nullptr);
	CHECK(bag.pack->itemAt(Cell{ 4, 1 })->type == "knife");
	CHECK(bag.pack->itemAt(Cell{ 2, 1 }) == nullptr);
	CHECK(bag.key(KEY_KEY_R));
	REQUIRE(bag.pack->itemAt(Cell{ 4, 1 }) != nullptr);
	CHECK(bag.pack->itemAt(Cell{ 4, 1 })->rotated);
	bag.key(KEY_UP);
	bag.key(KEY_KEY_R);
	CHECK_FALSE(bag.pack->itemAt(Cell{ 4, 1 })->rotated);

	bag.key(KEY_RETURN);
	REQUIRE(bag.page->isHolding());
	bag.key(KEY_LEFT);
	bag.key(KEY_LEFT);
	bag.key(KEY_LEFT);
	bag.key(KEY_LEFT);
	bag.key(KEY_LEFT);
	bag.draw();
	int mainHand = -1;
	for (int i = 0; i < bag.catalog.slotCount(); ++i)
		if (bag.catalog.slotId(i) == "main_hand")
			mainHand = i;
	CHECK(ring(bag.canvas, layout.slots[mainHand]) != nullptr);
	bag.key(KEY_RETURN);
	CHECK_FALSE(bag.page->isHolding());
	REQUIRE(bag.pack->equippedIn("main_hand") != nullptr);
	CHECK(bag.pack->equippedIn("main_hand")->type == "knife");

	bag.key(KEY_RETURN);
	CHECK(bag.pack->equippedIn("main_hand") == nullptr);
	bag.key(KEY_RIGHT);
	bag.draw();
	CHECK(ring(bag.canvas, layout.cellArea(Cell{ 0, 0 })) != nullptr);

	bag.key(KEY_DOWN);
	bag.mouse(EMIE_MOUSE_MOVED, layout.cellArea(Cell{ 5, 3 }).getCenter());
	bag.draw();
	CHECK(ring(bag.canvas, layout.cellArea(Cell{ 0, 1 })) == nullptr);
	bag.key(KEY_KEY_I);
	bag.key(KEY_KEY_I);
	bag.draw();
	CHECK(ring(bag.canvas, layout.cellArea(Cell{ 0, 0 })) != nullptr);
}
