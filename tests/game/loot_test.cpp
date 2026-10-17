#include "doctest.h"
#include "Backpack.h"
#include "Loot.h"
#include <string>

using namespace inventory;

namespace
{
	struct Stash
	{
		Stash(int columns = backpack::COLUMNS, int rows = backpack::ROWS) : carried(load(), columns, rows) {}

		const Catalog& load()
		{
			std::string problem;
			REQUIRE(backpack::loadCatalog(catalog, problem));
			return catalog;
		}

		std::vector<std::string> grid() const
		{
			std::vector<std::string> lines;
			for (int row = 0; row < carried.rows(); ++row)
			{
				std::string line;
				for (int column = 0; column < carried.columns(); ++column)
				{
					const Item* item = carried.itemAt(Cell{ column, row });
					line += item ? item->type[0] : '.';
				}
				lines.push_back(line);
			}
			return lines;
		}

		Catalog catalog;
		Inventory carried;
	};

	loot::Container crate(std::vector<Item> items, int capacity = 4)
	{
		return loot::Container{ "Crate", 0.0f, 0.0f, capacity, items };
	}
}

TEST_CASE("taking from a container moves the whole stack into the first free cells and drops it from the list")
{
	Stash stash;
	loot::Container box = crate({ Item{ "pistol", 1 }, Item{ "pills", 4 } });
	CHECK(loot::take(box, 0, stash.carried) == "");
	REQUIRE(box.items.size() == 1);
	CHECK(box.items[0].type == "pills");
	REQUIRE(stash.carried.itemAt(Cell{ 0, 0 }) != nullptr);
	CHECK(stash.carried.itemAt(Cell{ 0, 0 })->type == "pistol");
	CHECK(loot::take(box, 0, stash.carried) == "");
	CHECK(box.items.empty());
	CHECK(stash.carried.itemAt(Cell{ 1, 0 })->count == 4);
	CHECK(loot::take(box, 0, stash.carried) == "");
	CHECK(loot::take(box, -1, stash.carried) == "");
}

TEST_CASE("a stack taken from a container tops up a partial stack in the backpack first")
{
	Stash stash;
	REQUIRE(stash.carried.placeAt(Item{ "bandage", 3 }, Cell{ 2, 2 }).outcome == Outcome::Done);
	loot::Container box = crate({ Item{ "bandage", 4 } });
	CHECK(loot::take(box, 0, stash.carried) == "");
	CHECK(box.items.empty());
	CHECK(stash.carried.itemAt(Cell{ 2, 2 })->count == 5);
	CHECK(stash.carried.itemAt(Cell{ 0, 0 })->count == 2);
}

TEST_CASE("an item that only fits on its side goes in turned")
{
	Stash stash(4, 3);
	loot::Container box = crate({ Item{ "shotgun", 1 } });
	CHECK(loot::take(box, 0, stash.carried) == "");
	CHECK(stash.grid() == std::vector<std::string>{ "ssss", "....", "...." });
	CHECK(stash.carried.itemAt(Cell{ 0, 0 })->rotated);

	Stash tight(3, 3);
	REQUIRE(tight.carried.placeAt(Item{ "key", 1 }, Cell{ 1, 2 }).outcome == Outcome::Done);
	loot::Container armour = crate({ Item{ "body_armor", 1 } });
	CHECK(loot::take(armour, 0, tight.carried) == "");
	CHECK(tight.grid() == std::vector<std::string>{ "bbb", "bbb", ".k." });
	CHECK(tight.carried.itemAt(Cell{ 0, 0 })->rotated);
}

TEST_CASE("with no room the item stays in the container and the player is told the inventory is full")
{
	Stash stash(2, 2);
	REQUIRE(stash.carried.placeAt(Item{ "key", 1 }, Cell{ 0, 0 }).outcome == Outcome::Done);
	loot::Container box = crate({ Item{ "body_armor", 1 }, Item{ "pills", 10 } });
	CHECK(loot::take(box, 0, stash.carried) == "Inventory full.");
	REQUIRE(box.items.size() == 2);
	CHECK(box.items[0].type == "body_armor");
	REQUIRE(stash.carried.placeAt(Item{ "key", 1 }, Cell{ 1, 0 }).outcome == Outcome::Done);
	REQUIRE(stash.carried.placeAt(Item{ "key", 1 }, Cell{ 0, 1 }).outcome == Outcome::Done);
	REQUIRE(stash.carried.placeAt(Item{ "pills", 7 }, Cell{ 1, 1 }).outcome == Outcome::Done);
	CHECK(loot::take(box, 1, stash.carried) == "Inventory full.");
	CHECK(stash.carried.itemAt(Cell{ 1, 1 })->count == 10);
	REQUIRE(box.items.size() == 2);
	CHECK(box.items[1].count == 7);
}

TEST_CASE("giving puts the clicked item's whole stack in the container upright, until the container is full")
{
	Stash stash;
	REQUIRE(stash.carried.placeAt(Item{ "body_armor", 1, true }, Cell{ 0, 0 }).outcome == Outcome::Done);
	REQUIRE(stash.carried.placeAt(Item{ "bandage", 3 }, Cell{ 5, 4 }).outcome == Outcome::Done);
	loot::Container box = crate({ Item{ "key", 1 } }, 2);
	CHECK(loot::give(box, Cell{ 2, 1 }, stash.carried) == "");
	CHECK(stash.carried.itemAt(Cell{ 0, 0 }) == nullptr);
	REQUIRE(box.items.size() == 2);
	CHECK(box.items[1].type == "body_armor");
	CHECK_FALSE(box.items[1].rotated);
	CHECK(loot::give(box, Cell{ 5, 4 }, stash.carried) == "Container full.");
	CHECK(stash.carried.itemAt(Cell{ 5, 4 })->count == 3);
	CHECK(box.items.size() == 2);
	CHECK(loot::give(box, Cell{ 7, 0 }, stash.carried) == "");
	CHECK(box.items.size() == 2);
}

TEST_CASE("the closest container within two and a half metres is the one in reach")
{
	std::vector<loot::Container> boxes{ loot::Container{ "Chest", 0.0f, 0.0f, 8, {} }, loot::Container{ "Crate", 3.0f, 0.0f, 4, {} } };
	CHECK(loot::closest(boxes, 0.5f, 0.0f) == 0);
	CHECK(loot::closest(boxes, 2.0f, 0.0f) == 1);
	CHECK(loot::closest(boxes, 1.4f, 0.0f) == 0);
	CHECK(loot::closest(boxes, 0.0f, 2.4f) == 0);
	CHECK(loot::closest(boxes, 0.0f, 2.6f) == -1);
	CHECK(loot::closest(boxes, -10.0f, 5.0f) == -1);
	CHECK(loot::closest({}, 0.0f, 0.0f) == -1);
}

TEST_CASE("the flat world hides two chests of eight stacks and two crates of four, stocked with catalog items including big ones")
{
	Stash stash;
	std::vector<loot::Container> boxes = loot::flatWorldContainers();
	int chests = 0, crates = 0;
	bool armour = false, shotgun = false;
	for (const loot::Container& box : boxes)
	{
		INFO(box.kind);
		if (box.kind == "Chest")
		{
			++chests;
			CHECK(box.capacity == 8);
		}
		else
		{
			CHECK(box.kind == "Crate");
			++crates;
			CHECK(box.capacity == 4);
		}
		CHECK_FALSE(box.items.empty());
		CHECK((int)box.items.size() < box.capacity);
		CHECK_FALSE(box.searched);
		for (const Item& item : box.items)
		{
			INFO(item.type);
			const ItemType* type = stash.catalog.itemType(item.type);
			REQUIRE(type != nullptr);
			CHECK(item.count >= 1);
			CHECK(item.count <= type->maxStack);
			armour = armour || item.type == "body_armor";
			shotgun = shotgun || item.type == "shotgun";
		}
	}
	CHECK(chests == 2);
	CHECK(crates == 2);
	CHECK(armour);
	CHECK(shotgun);
}
