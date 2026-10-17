#ifndef BACKPACK_H
#define BACKPACK_H

#include <string>
#include <vector>
#include <irrlicht.h>
#include "Catalog.h"
#include "Inventory.h"
#include "CharacterSheet.h"

namespace backpack
{
	const int COLUMNS = 8;
	const int ROWS = 5;
	const char* const SLOTS_FILE = "engine/gamecorev0.1.1/gamecorev0.1.1/inventory/data/slots.csv";
	const char* const ITEMS_FILE = "engine/gamecorev0.1.1/gamecorev0.1.1/inventory/data/items.csv";
	const char* const ICON_FOLDER = "engine/gamecorev0.1.1/gamecorev0.1.1/inventory/icons/";

	bool loadCatalog(inventory::Catalog& catalog, std::string& problem);
	void addStartingItems(inventory::Inventory& inventory);
	character::Carried capture(const inventory::Inventory& carried, const inventory::Catalog& catalog);
	std::vector<std::string> restore(const character::Carried& saved, inventory::Inventory& carried, const inventory::Catalog& catalog);

	struct Layout
	{
		Layout(const inventory::Catalog& catalog, const irr::core::rect<irr::s32>& left, const irr::core::rect<irr::s32>& right,
			int columns = COLUMNS, int rows = ROWS);

		irr::core::rect<irr::s32> cellArea(inventory::Cell cell) const;
		irr::core::rect<irr::s32> footprint(const inventory::Placed& placed) const;
		bool cellAt(irr::core::position2d<irr::s32> point, inventory::Cell& cell) const;
		int slotAt(irr::core::position2d<irr::s32> point) const;

		const inventory::Catalog& catalog;
		irr::core::rect<irr::s32> left;
		irr::core::rect<irr::s32> right;
		int columns;
		int rows;
		irr::core::rect<irr::s32> grid;
		irr::s32 cell;
		std::vector<irr::core::rect<irr::s32> > slots;
	};

	class Page
	{
	public:
		Page(inventory::Inventory& inventory, const inventory::Catalog& catalog);

		void clickCell(inventory::Cell cell);
		void clickSlot(int slot);
		void cancel();
		void rotate(inventory::Cell hovered, bool overGrid);

		bool isHolding() const { return holding_; }
		const inventory::Item& held() const { return held_; }
		inventory::Cell grabOffset() const { return grabOffset_; }
		const std::string& message() const { return message_; }
		int selectedSlot() const { return selected_; }
		std::string slotLine() const;

		void click(const Layout& layout, irr::core::position2d<irr::s32> point);
		const inventory::Inventory& inventory() const { return inventory_; }
		const inventory::Catalog& catalog() const { return catalog_; }
		std::string iconPath(const std::string& type) const;

	private:
		void explain(inventory::Outcome outcome, const std::string& slot);
		std::string itemName(const std::string& type) const;

		inventory::Inventory& inventory_;
		const inventory::Catalog& catalog_;
		bool holding_ = false;
		inventory::Item held_;
		inventory::Cell from_{ 0, 0 };
		bool fromRotated_ = false;
		inventory::Cell grabOffset_{ 0, 0 };
		std::string message_;
		int selected_ = -1;
	};
}

#endif
