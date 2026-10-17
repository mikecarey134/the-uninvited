#ifndef LOOT_H
#define LOOT_H

#include <string>
#include <vector>
#include <irrlicht.h>
#include "Catalog.h"
#include "Inventory.h"

namespace loot
{
	const irr::f32 REACH = 2.5f;

	struct Container
	{
		std::string kind;
		irr::f32 x;
		irr::f32 z;
		int capacity;
		std::vector<inventory::Item> items;
		bool searched = false;
	};

	std::vector<Container> flatWorldContainers();
	int closest(const std::vector<Container>& containers, irr::f32 x, irr::f32 z);
	std::string take(Container& container, int index, inventory::Inventory& carried);
	std::string give(Container& container, inventory::Cell cell, inventory::Inventory& carried);
}

#endif
