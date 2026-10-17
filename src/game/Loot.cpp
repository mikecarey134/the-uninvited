#include "Loot.h"
#include <cmath>

using namespace inventory;

namespace loot
{
	std::vector<Container> flatWorldContainers()
	{
		return {
			Container{ "Crate", 4.0f, 9.0f, 4, { Item{ "bandage", 2 }, Item{ "lockpick", 1 }, Item{ "ammo_box", 3 } } },
			Container{ "Chest", 7.0f, -5.0f, 8, { Item{ "shotgun", 1 }, Item{ "body_armor", 1 }, Item{ "pills", 4 }, Item{ "ammo_box", 5 },
				Item{ "helmet", 1 }, Item{ "small_backpack", 1 } } },
			Container{ "Crate", -12.0f, -14.0f, 4, { Item{ "key", 1 }, Item{ "pills", 2 } } },
			Container{ "Chest", 20.0f, 4.0f, 8, { Item{ "pistol", 1 }, Item{ "boots", 1 }, Item{ "gloves", 1 }, Item{ "bandage", 5 },
				Item{ "utility_belt", 1 }, Item{ "large_backpack", 1 } } },
		};
	}

	int closest(const std::vector<Container>& containers, irr::f32 x, irr::f32 z)
	{
		int best = -1;
		irr::f32 nearest = REACH;
		for (size_t i = 0; i < containers.size(); ++i)
		{
			irr::f32 distance = std::sqrt((containers[i].x - x) * (containers[i].x - x) + (containers[i].z - z) * (containers[i].z - z));
			if (distance <= nearest)
			{
				nearest = distance;
				best = (int)i;
			}
		}
		return best;
	}

	std::string take(Container& container, int index, Inventory& carried)
	{
		if (index < 0 || index >= (int)container.items.size())
			return "";
		Item& item = container.items[index];
		Result upright = carried.pickUp(Item{ item.type, item.count, false });
		int left = upright.leftover;
		if (left > 0)
			left = carried.pickUp(Item{ item.type, left, true }).leftover;
		if (left == 0)
		{
			container.items.erase(container.items.begin() + index);
			return "";
		}
		item.count = left;
		return "Inventory full.";
	}

	std::string give(Container& container, Cell cell, Inventory& carried)
	{
		const Item* item = carried.itemAt(cell);
		if (!item)
			return "";
		if ((int)container.items.size() >= container.capacity)
			return "Container full.";
		Taken taken = carried.takeAt(cell, item->count);
		taken.item.rotated = false;
		container.items.push_back(taken.item);
		return "";
	}
}
