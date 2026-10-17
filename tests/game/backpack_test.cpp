#include "doctest.h"
#include "Backpack.h"
#include <algorithm>
#include <set>
#include <string>

using namespace irr;
using namespace core;
using namespace inventory;

namespace
{
	struct Pack
	{
		Pack() : inventory(load(), backpack::COLUMNS, backpack::ROWS), page(inventory, catalog) {}

		const Catalog& load()
		{
			std::string problem;
			loaded = backpack::loadCatalog(catalog, problem);
			INFO(problem);
			REQUIRE(loaded);
			return catalog;
		}

		std::vector<std::string> grid() const
		{
			std::vector<std::string> lines;
			for (int row = 0; row < backpack::ROWS; ++row)
			{
				std::string line;
				for (int column = 0; column < backpack::COLUMNS; ++column)
				{
					const Item* item = inventory.itemAt(Cell{ column, row });
					line += item ? item->type[0] : '.';
				}
				lines.push_back(line);
			}
			return lines;
		}

		int slot(const std::string& id) const
		{
			for (int i = 0; i < catalog.slotCount(); ++i)
				if (catalog.slotId(i) == id)
					return i;
			return -1;
		}

		bool loaded = false;
		Catalog catalog;
		Inventory inventory;
		backpack::Page page;
	};

	const rect<s32> leftPage(200, 100, 560, 560);
	const rect<s32> rightPage(700, 100, 1000, 560);
}

TEST_CASE("the backpack is an eight by five grid loaded from the engine's item files, and a new game starts with a knife, bandages, ammo, body armour and both backpacks")
{
	Pack pack;
	CHECK(pack.catalog.slotCount() == 9);
	REQUIRE(pack.catalog.itemType("body_armor") != nullptr);
	backpack::addStartingItems(pack.inventory);
	std::vector<Placed> contents = pack.inventory.backpackContents();
	std::set<std::string> types;
	for (const Placed& placed : contents)
		types.insert(placed.item.type);
	CHECK(types == std::set<std::string>{ "knife", "bandage", "ammo_box", "body_armor", "small_backpack", "large_backpack" });
	for (const Placed& placed : contents)
	{
		if (placed.item.type == "bandage")
			CHECK(placed.item.count == 3);
		if (placed.item.type == "ammo_box")
			CHECK(placed.item.count == 6);
	}
	CHECK(pack.inventory.rows() == 5);
}

TEST_CASE("clicking an item picks it up where it was grabbed, and clicking empty cells puts it down with the same grab")
{
	Pack pack;
	REQUIRE(pack.inventory.placeAt(Item{ "body_armor", 1 }, Cell{ 0, 0 }).outcome == Outcome::Done);
	pack.page.clickCell(Cell{ 1, 2 });
	REQUIRE(pack.page.isHolding());
	CHECK(pack.page.held().type == "body_armor");
	CHECK(pack.page.grabOffset().column == 1);
	CHECK(pack.page.grabOffset().row == 2);
	CHECK(pack.inventory.itemAt(Cell{ 0, 0 }) == nullptr);
	pack.page.clickCell(Cell{ 5, 4 });
	CHECK_FALSE(pack.page.isHolding());
	CHECK(pack.grid() == std::vector<std::string>{ "........", "........", "....bb..", "....bb..", "....bb.." });
}

TEST_CASE("an item that will not go where it is clicked stays in hand with the reason shown")
{
	Pack pack;
	REQUIRE(pack.inventory.placeAt(Item{ "body_armor", 1 }, Cell{ 0, 0 }).outcome == Outcome::Done);
	REQUIRE(pack.inventory.placeAt(Item{ "key", 1 }, Cell{ 5, 1 }).outcome == Outcome::Done);
	pack.page.clickCell(Cell{ 0, 0 });
	pack.page.clickCell(Cell{ 4, 0 });
	CHECK(pack.page.isHolding());
	CHECK(pack.page.message() == "Something is in the way");
	pack.page.clickCell(Cell{ 7, 3 });
	CHECK(pack.page.isHolding());
	CHECK(pack.page.message() == "It won't fit there");
	pack.page.clickCell(Cell{ 2, 2 });
	CHECK_FALSE(pack.page.isHolding());
	CHECK(pack.page.message().empty());
}

TEST_CASE("R turns the item in hand, and right-click puts it back where it came from the way it was")
{
	Pack pack;
	REQUIRE(pack.inventory.placeAt(Item{ "body_armor", 1 }, Cell{ 0, 0 }).outcome == Outcome::Done);
	pack.page.clickCell(Cell{ 1, 2 });
	pack.page.rotate(Cell{ 0, 0 }, false);
	CHECK(pack.page.held().rotated);
	CHECK(pack.page.grabOffset().column == 2);
	CHECK(pack.page.grabOffset().row == 1);
	pack.page.clickCell(Cell{ 4, 1 });
	CHECK(pack.grid() == std::vector<std::string>{ "..bbb...", "..bbb...", "........", "........", "........" });
	CHECK(pack.inventory.itemAt(Cell{ 2, 0 })->rotated);

	pack.page.clickCell(Cell{ 2, 0 });
	pack.page.rotate(Cell{ 0, 0 }, false);
	pack.page.rotate(Cell{ 0, 0 }, false);
	pack.page.rotate(Cell{ 0, 0 }, false);
	pack.page.cancel();
	CHECK_FALSE(pack.page.isHolding());
	CHECK(pack.grid() == std::vector<std::string>{ "..bbb...", "..bbb...", "........", "........", "........" });
	CHECK(pack.inventory.itemAt(Cell{ 2, 0 })->rotated);
	pack.page.cancel();
	CHECK_FALSE(pack.page.isHolding());
}

TEST_CASE("R over an item with nothing in hand turns it where it lies, or says why it cannot")
{
	Pack pack;
	REQUIRE(pack.inventory.placeAt(Item{ "pistol", 1 }, Cell{ 0, 0 }).outcome == Outcome::Done);
	REQUIRE(pack.inventory.placeAt(Item{ "key", 1 }, Cell{ 2, 0 }).outcome == Outcome::Done);
	pack.page.rotate(Cell{ 0, 1 }, true);
	CHECK(pack.page.message() == "Something is in the way");
	CHECK_FALSE(pack.inventory.itemAt(Cell{ 0, 0 })->rotated);
	REQUIRE(pack.inventory.takeAt(Cell{ 2, 0 }, 1).outcome == Outcome::Done);
	pack.page.rotate(Cell{ 0, 1 }, true);
	CHECK(pack.page.message().empty());
	CHECK(pack.grid() == std::vector<std::string>{ "ppp.....", "........", "........", "........", "........" });
	pack.page.rotate(Cell{ 6, 3 }, true);
	pack.page.rotate(Cell{ 0, 0 }, false);
	CHECK(pack.grid()[0] == "ppp.....");
}

TEST_CASE("clicking a slot with an item in hand equips it, says why when it cannot, and clicking an equipped slot takes it off")
{
	Pack pack;
	backpack::addStartingItems(pack.inventory);
	Cell knife{ -1, -1 }, armour{ -1, -1 };
	for (const Placed& placed : pack.inventory.backpackContents())
	{
		if (placed.item.type == "knife")
			knife = placed.anchor;
		if (placed.item.type == "body_armor")
			armour = placed.anchor;
	}
	pack.page.clickCell(armour);
	pack.page.clickSlot(pack.slot("head"));
	CHECK(pack.page.isHolding());
	CHECK(pack.page.message() == "Body Armor doesn't go in the Head slot");
	pack.page.clickSlot(pack.slot("chest"));
	CHECK_FALSE(pack.page.isHolding());
	REQUIRE(pack.inventory.equippedIn("chest") != nullptr);
	CHECK(pack.inventory.equippedIn("chest")->type == "body_armor");
	CHECK(pack.page.slotLine() == "Chest: Body Armor");

	pack.page.clickCell(knife);
	pack.page.clickSlot(pack.slot("main_hand"));
	CHECK(pack.inventory.equippedIn("main_hand") != nullptr);
	pack.page.clickSlot(pack.slot("main_hand"));
	CHECK(pack.inventory.equippedIn("main_hand") == nullptr);
	CHECK(pack.page.slotLine() == "Main Hand: empty");
	bool back = false;
	for (const Placed& placed : pack.inventory.backpackContents())
		back = back || placed.item.type == "knife";
	CHECK(back);
	pack.page.clickSlot(pack.slot("neck"));
	CHECK(pack.page.slotLine() == "Neck: empty");
	CHECK(pack.page.message().empty());
}

TEST_CASE("the grid sits on the right page and the eight slots on the left, and points map back to cells and slots")
{
	Pack pack;
	backpack::Layout layout(pack.catalog, leftPage, rightPage);
	CHECK(layout.cell >= 20);
	CHECK(rightPage.isRectCollided(layout.grid));
	CHECK(layout.grid.UpperLeftCorner.X >= rightPage.UpperLeftCorner.X);
	CHECK(layout.grid.LowerRightCorner.X <= rightPage.LowerRightCorner.X);
	CHECK(layout.grid.LowerRightCorner.Y <= rightPage.LowerRightCorner.Y);
	CHECK(layout.grid.getWidth() == layout.cell * backpack::COLUMNS);
	CHECK(layout.grid.getHeight() == layout.cell * backpack::ROWS);
	Cell cell{ -1, -1 };
	REQUIRE(layout.cellAt(layout.cellArea(Cell{ 6, 3 }).getCenter(), cell));
	CHECK(cell.column == 6);
	CHECK(cell.row == 3);
	CHECK_FALSE(layout.cellAt(position2d<s32>(layout.grid.LowerRightCorner.X + 2, layout.grid.UpperLeftCorner.Y + 2), cell));
	CHECK_FALSE(layout.cellAt(position2d<s32>(layout.grid.LowerRightCorner.X, layout.grid.UpperLeftCorner.Y + 2), cell));
	CHECK_FALSE(layout.cellAt(position2d<s32>(layout.grid.UpperLeftCorner.X + 2, layout.grid.LowerRightCorner.Y), cell));
	REQUIRE(layout.slots.size() == 9);
	for (size_t i = 0; i < layout.slots.size(); ++i)
	{
		INFO(pack.catalog.slotId((int)i));
		CHECK(leftPage.isRectCollided(layout.slots[i]));
		CHECK(layout.slots[i].UpperLeftCorner.X >= leftPage.UpperLeftCorner.X);
		CHECK(layout.slots[i].LowerRightCorner.X <= leftPage.LowerRightCorner.X);
		CHECK(layout.slotAt(layout.slots[i].getCenter()) == (int)i);
		for (size_t j = 0; j < i; ++j)
			CHECK_FALSE(layout.slots[i].isRectCollided(layout.slots[j]));
	}
	CHECK(layout.slots[pack.slot("head")].UpperLeftCorner.Y < layout.slots[pack.slot("boots")].UpperLeftCorner.Y);
	CHECK(layout.slots[pack.slot("gloves")].UpperLeftCorner.X < layout.slots[pack.slot("off_hand")].UpperLeftCorner.X);
	CHECK(layout.slotAt(position2d<s32>(rightPage.getCenter())) == -1);
	Placed armour{ Item{ "body_armor", 1, true }, Cell{ 1, 1 } };
	CHECK(layout.footprint(armour).getWidth() == 3 * layout.cell);
	CHECK(layout.footprint(armour).getHeight() == 2 * layout.cell);
}

TEST_CASE("clicks on the page reach the right cell or slot")
{
	Pack pack;
	backpack::Layout layout(pack.catalog, leftPage, rightPage);
	REQUIRE(pack.inventory.placeAt(Item{ "knife", 1 }, Cell{ 3, 1 }).outcome == Outcome::Done);
	pack.page.click(layout, layout.cellArea(Cell{ 3, 2 }).getCenter());
	REQUIRE(pack.page.isHolding());
	pack.page.click(layout, layout.slots[pack.slot("off_hand")].getCenter());
	REQUIRE(pack.inventory.equippedIn("off_hand") != nullptr);
	pack.page.click(layout, position2d<s32>(5, 5));
	CHECK_FALSE(pack.page.isHolding());
}

namespace
{
	Cell anchorOf(const Inventory& inventory, const std::string& type)
	{
		for (const Placed& placed : inventory.backpackContents())
			if (placed.item.type == type)
				return placed.anchor;
		return Cell{ -1, -1 };
	}
}

TEST_CASE("wearing a backpack grows the grid, and an item in the extra rows keeps it on with a message until it is moved")
{
	Pack pack;
	REQUIRE(pack.inventory.placeAt(Item{ "small_backpack", 1 }, Cell{ 0, 0 }).outcome == Outcome::Done);
	pack.page.clickCell(Cell{ 0, 0 });
	pack.page.clickSlot(pack.slot("back"));
	CHECK_FALSE(pack.page.isHolding());
	CHECK(pack.inventory.rows() == 7);
	CHECK(pack.inventory.columns() == 8);
	REQUIRE(pack.inventory.placeAt(Item{ "key", 1 }, Cell{ 3, 6 }).outcome == Outcome::Done);
	pack.page.clickSlot(pack.slot("back"));
	CHECK(pack.page.message() == "Clear items from extended slots first.");
	CHECK(pack.inventory.equippedIn("back") != nullptr);
	CHECK(pack.inventory.rows() == 7);
	pack.page.clickCell(Cell{ 3, 6 });
	pack.page.clickCell(Cell{ 3, 0 });
	pack.page.clickSlot(pack.slot("back"));
	CHECK(pack.page.message().empty());
	CHECK(pack.inventory.equippedIn("back") == nullptr);
	CHECK(pack.inventory.rows() == 5);
}

TEST_CASE("the large backpack grows the grid to nine rows, and trading down to the small one needs its extra rows clear")
{
	Pack pack;
	REQUIRE(pack.inventory.placeAt(Item{ "large_backpack", 1 }, Cell{ 0, 0 }).outcome == Outcome::Done);
	REQUIRE(pack.inventory.placeAt(Item{ "small_backpack", 1 }, Cell{ 4, 0 }).outcome == Outcome::Done);
	pack.page.clickCell(Cell{ 0, 0 });
	pack.page.clickSlot(pack.slot("back"));
	CHECK(pack.inventory.rows() == 9);
	REQUIRE(pack.inventory.placeAt(Item{ "key", 1 }, Cell{ 7, 8 }).outcome == Outcome::Done);
	pack.page.clickCell(anchorOf(pack.inventory, "small_backpack"));
	pack.page.clickSlot(pack.slot("back"));
	CHECK(pack.page.isHolding());
	CHECK(pack.page.message() == "Not enough room to downgrade backpack.");
	CHECK(pack.inventory.equippedIn("back")->type == "large_backpack");
	CHECK(pack.inventory.rows() == 9);
	pack.page.cancel();
	pack.page.clickCell(Cell{ 7, 8 });
	pack.page.clickCell(Cell{ 7, 0 });
	pack.page.clickCell(anchorOf(pack.inventory, "small_backpack"));
	pack.page.clickSlot(pack.slot("back"));
	CHECK(pack.inventory.equippedIn("back")->type == "small_backpack");
	CHECK(pack.inventory.rows() == 7);
	CHECK(anchorOf(pack.inventory, "large_backpack").row >= 0);
}

TEST_CASE("a nine-row grid still fits the page with usable cells, and the back slot sits beside the chest")
{
	Pack pack;
	backpack::Layout layout(pack.catalog, leftPage, rightPage, 8, 9);
	CHECK(layout.cell >= 32);
	CHECK(layout.grid.getHeight() == 9 * layout.cell);
	CHECK(layout.grid.getWidth() == 8 * layout.cell);
	CHECK(layout.grid.LowerRightCorner.Y <= rightPage.LowerRightCorner.Y);
	Cell cell{ -1, -1 };
	REQUIRE(layout.cellAt(layout.cellArea(Cell{ 2, 8 }).getCenter(), cell));
	CHECK(cell.row == 8);
	REQUIRE(layout.slots.size() == 9);
	const rect<s32>& back = layout.slots[pack.slot("back")];
	const rect<s32>& chest = layout.slots[pack.slot("chest")];
	CHECK(back.UpperLeftCorner.Y == chest.UpperLeftCorner.Y);
	CHECK(back.UpperLeftCorner.X > chest.LowerRightCorner.X);
}

namespace
{
	std::vector<Placed> sorted(std::vector<Placed> contents)
	{
		std::sort(contents.begin(), contents.end(), [](const Placed& a, const Placed& b) {
			return a.anchor.row != b.anchor.row ? a.anchor.row < b.anchor.row : a.anchor.column < b.anchor.column;
		});
		return contents;
	}
}

TEST_CASE("what the player carries and wears is captured for the character file and restored exactly, backpack size included")
{
	Pack pack;
	REQUIRE(pack.inventory.placeAt(Item{ "small_backpack", 1 }, Cell{ 0, 0 }).outcome == Outcome::Done);
	REQUIRE(pack.inventory.equipFrom(Cell{ 0, 0 }, "back").outcome == Outcome::Done);
	REQUIRE(pack.inventory.placeAt(Item{ "pistol", 1 }, Cell{ 0, 0 }).outcome == Outcome::Done);
	REQUIRE(pack.inventory.equipFrom(Cell{ 0, 0 }, "main_hand").outcome == Outcome::Done);
	REQUIRE(pack.inventory.placeAt(Item{ "body_armor", 1, true }, Cell{ 5, 5 }).outcome == Outcome::Done);
	REQUIRE(pack.inventory.placeAt(Item{ "bandage", 4 }, Cell{ 1, 6 }).outcome == Outcome::Done);
	REQUIRE(pack.inventory.placeAt(Item{ "knife", 1 }, Cell{ 0, 0 }).outcome == Outcome::Done);
	character::Carried carried = backpack::capture(pack.inventory, pack.catalog);
	CHECK(carried.columns == 8);
	CHECK(carried.rows == 7);
	CHECK(carried.items.size() == 3);
	CHECK(carried.worn.size() == 2);

	Inventory again(pack.catalog, backpack::COLUMNS, backpack::ROWS);
	std::vector<std::string> warnings = backpack::restore(carried, again, pack.catalog);
	CHECK(warnings.empty());
	CHECK(again.columns() == 8);
	CHECK(again.rows() == 7);
	REQUIRE(again.equippedIn("back") != nullptr);
	CHECK(again.equippedIn("back")->type == "small_backpack");
	REQUIRE(again.equippedIn("main_hand") != nullptr);
	CHECK(again.equippedIn("main_hand")->type == "pistol");
	std::vector<Placed> before = sorted(pack.inventory.backpackContents()), after = sorted(again.backpackContents());
	REQUIRE(after.size() == before.size());
	for (size_t i = 0; i < before.size(); ++i)
	{
		INFO(before[i].item.type);
		CHECK(after[i].item.type == before[i].item.type);
		CHECK(after[i].item.count == before[i].item.count);
		CHECK(after[i].item.rotated == before[i].item.rotated);
		CHECK(after[i].anchor.column == before[i].anchor.column);
		CHECK(after[i].anchor.row == before[i].anchor.row);
	}
}

TEST_CASE("an unknown item, a slot that does not exist, an item that no longer fits and garbled lines are skipped with a warning each")
{
	Pack pack;
	character::Carried carried;
	carried.columns = 8;
	carried.rows = 9;
	carried.worn = { character::WornItem{ "back", "large_backpack" }, character::WornItem{ "tail", "knife" }, character::WornItem{ "head", "wizard_hat" } };
	carried.items = { character::CarriedItem{ "knife", 0, 7, false, 1 }, character::CarriedItem{ "laser_rifle", 2, 0, false, 1 },
		character::CarriedItem{ "pistol", 7, 7, false, 1 }, character::CarriedItem{ "key", 0, 8, false, 1 }, character::CarriedItem{ "bandage", 3, 3, false, 9 } };
	carried.problems = { "unreadable inv.item.9 = junk" };
	Inventory again(pack.catalog, backpack::COLUMNS, backpack::ROWS);
	std::vector<std::string> warnings = backpack::restore(carried, again, pack.catalog);
	CHECK(again.rows() == 9);
	REQUIRE(again.itemAt(Cell{ 0, 8 }) != nullptr);
	CHECK(again.itemAt(Cell{ 0, 8 })->type == "knife");
	CHECK(again.backpackContents().size() == 2);
	REQUIRE(again.itemAt(Cell{ 3, 3 }) != nullptr);
	CHECK(again.itemAt(Cell{ 3, 3 })->count == 5);
	REQUIRE(warnings.size() == 7);
	auto mentions = [&warnings](const std::string& word) {
		for (const std::string& line : warnings)
			if (line.find(word) != std::string::npos)
				return true;
		return false;
	};
	CHECK(mentions("laser_rifle"));
	CHECK(mentions("tail"));
	CHECK(mentions("wizard_hat"));
	CHECK(mentions("pistol"));
	CHECK(mentions("key"));
	CHECK(mentions("bandage"));
	CHECK(mentions("inv.item.9"));
}

TEST_CASE("a character with nothing saved starts with an empty eight by five backpack and no warnings")
{
	Pack pack;
	Inventory again(pack.catalog, backpack::COLUMNS, backpack::ROWS);
	CHECK(backpack::restore(character::Carried(), again, pack.catalog).empty());
	CHECK(again.backpackContents().empty());
	CHECK(again.columns() == 8);
	CHECK(again.rows() == 5);
	for (int i = 0; i < pack.catalog.slotCount(); ++i)
		CHECK(again.equippedIn(pack.catalog.slotId(i)) == nullptr);
	character::Carried empty = backpack::capture(again, pack.catalog);
	CHECK(empty.items.empty());
	CHECK(empty.worn.empty());
}
