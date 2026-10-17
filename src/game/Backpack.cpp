#include "Backpack.h"
#include <algorithm>
#include <fstream>
#include <sstream>

using namespace irr;
using namespace core;
using namespace inventory;

namespace
{
	const s32 heading = 44;
	const s32 footer = 40;

	std::string readText(const char* path)
	{
		std::ifstream in(path, std::ios::binary);
		std::ostringstream text;
		text << in.rdbuf();
		return text.str();
	}

}

namespace backpack
{
	bool loadCatalog(Catalog& catalog, std::string& problem)
	{
		LoadError error;
		if (inventory::loadCatalog(readText(SLOTS_FILE), readText(ITEMS_FILE), catalog, error))
			return true;
		problem = error.file + " line " + std::to_string(error.line) + ": " + error.message;
		return false;
	}

	void addStartingItems(Inventory& inventory)
	{
		inventory.pickUp(Item{ "knife", 1 });
		inventory.pickUp(Item{ "bandage", 3 });
		inventory.pickUp(Item{ "ammo_box", 6 });
		inventory.pickUp(Item{ "body_armor", 1 });
		inventory.pickUp(Item{ "small_backpack", 1 });
		inventory.pickUp(Item{ "large_backpack", 1 });
	}

	Layout::Layout(const Catalog& catalog, const rect<s32>& left, const rect<s32>& right, int columns, int rows)
		: catalog(catalog), left(left), right(right), columns(columns), rows(rows)
	{
		cell = std::min(right.getWidth() / columns, (right.getHeight() - heading - footer) / rows);
		position2d<s32> corner(right.UpperLeftCorner.X, right.UpperLeftCorner.Y + heading);
		grid = rect<s32>(corner.X, corner.Y, corner.X + cell * columns, corner.Y + cell * rows);
		s32 columnWidth = left.getWidth() / 3;
		s32 rowHeight = (left.getHeight() - heading - footer) / 4;
		s32 box = std::min(columnWidth, rowHeight) * 5 / 8;
		for (int i = 0; i < catalog.slotCount(); ++i)
		{
			const Slot& slot = catalog.slot(i);
			s32 x = left.UpperLeftCorner.X + columnWidth * slot.dollColumn + columnWidth / 2;
			s32 y = left.UpperLeftCorner.Y + heading + rowHeight * slot.dollRow + rowHeight / 2 - 8;
			slots.push_back(rect<s32>(x - box / 2, y - box / 2, x - box / 2 + box, y - box / 2 + box));
		}
	}

	rect<s32> Layout::cellArea(Cell at) const
	{
		s32 x = grid.UpperLeftCorner.X + at.column * cell;
		s32 y = grid.UpperLeftCorner.Y + at.row * cell;
		return rect<s32>(x, y, x + cell, y + cell);
	}

	rect<s32> Layout::footprint(const Placed& placed) const
	{
		const ItemType* type = catalog.itemType(placed.item.type);
		s32 across = placed.item.rotated ? type->height : type->width;
		s32 down = placed.item.rotated ? type->width : type->height;
		rect<s32> corner = cellArea(placed.anchor);
		return rect<s32>(corner.UpperLeftCorner.X, corner.UpperLeftCorner.Y, corner.UpperLeftCorner.X + across * cell,
			corner.UpperLeftCorner.Y + down * cell);
	}

	bool Layout::cellAt(position2d<s32> point, Cell& at) const
	{
		if (point.X < grid.UpperLeftCorner.X || point.Y < grid.UpperLeftCorner.Y || point.X >= grid.LowerRightCorner.X ||
			point.Y >= grid.LowerRightCorner.Y)
			return false;
		at = Cell{ (point.X - grid.UpperLeftCorner.X) / cell, (point.Y - grid.UpperLeftCorner.Y) / cell };
		return true;
	}

	int Layout::slotAt(position2d<s32> point) const
	{
		for (size_t i = 0; i < slots.size(); ++i)
			if (slots[i].isPointInside(point))
				return (int)i;
		return -1;
	}

	Page::Page(Inventory& inventory, const Catalog& catalog) : inventory_(inventory), catalog_(catalog)
	{
	}

	std::string Page::itemName(const std::string& type) const
	{
		const ItemType* found = catalog_.itemType(type);
		return found ? found->name : type;
	}

	void Page::explain(Outcome outcome, const std::string& slot)
	{
		switch (outcome)
		{
		case Outcome::OutOfBounds: message_ = "It won't fit there"; break;
		case Outcome::Overlaps: message_ = "Something is in the way"; break;
		case Outcome::NoRoom: message_ = "No room in the backpack"; break;
		case Outcome::NotAllowed: message_ = itemName(held_.type) + " doesn't go in the " + slot + " slot"; break;
		case Outcome::Blocked: message_ = "The " + slot + " is blocked by a two-handed weapon"; break;
		case Outcome::ItemsOutside: message_ = "Not enough room to downgrade backpack."; break;
		default: message_ = "That can't be done"; break;
		}
	}

	void Page::clickCell(Cell cell)
	{
		if (!holding_)
		{
			for (const Placed& placed : inventory_.backpackContents())
			{
				const ItemType* type = catalog_.itemType(placed.item.type);
				s32 across = placed.item.rotated ? type->height : type->width;
				s32 down = placed.item.rotated ? type->width : type->height;
				if (cell.column < placed.anchor.column || cell.row < placed.anchor.row || cell.column >= placed.anchor.column + across ||
					cell.row >= placed.anchor.row + down)
					continue;
				Taken taken = inventory_.takeAt(cell, placed.item.count);
				held_ = taken.item;
				from_ = placed.anchor;
				fromRotated_ = held_.rotated;
				grabOffset_ = Cell{ cell.column - placed.anchor.column, cell.row - placed.anchor.row };
				holding_ = true;
				message_.clear();
				return;
			}
			return;
		}
		Result result = inventory_.placeAt(held_, Cell{ cell.column - grabOffset_.column, cell.row - grabOffset_.row });
		if (result.outcome == Outcome::Done)
		{
			holding_ = false;
			message_.clear();
		}
		else if (result.outcome == Outcome::Partial)
			held_.count = result.leftover;
		else
			explain(result.outcome, "");
	}

	void Page::cancel()
	{
		if (!holding_)
			return;
		held_.rotated = fromRotated_;
		inventory_.placeAt(held_, from_);
		holding_ = false;
		message_.clear();
	}

	void Page::rotate(Cell hovered, bool overGrid)
	{
		if (holding_)
		{
			held_.rotated = !held_.rotated;
			grabOffset_ = Cell{ grabOffset_.row, grabOffset_.column };
			return;
		}
		if (!overGrid)
			return;
		Result result = inventory_.rotate(hovered);
		if (result.outcome == Outcome::Done)
			message_.clear();
		else if (result.outcome != Outcome::EmptyCell)
			explain(result.outcome, "");
	}

	void Page::clickSlot(int slot)
	{
		if (slot < 0 || slot >= catalog_.slotCount())
			return;
		selected_ = slot;
		const std::string& id = catalog_.slotId(slot);
		if (holding_)
		{
			bool turned = held_.rotated;
			held_.rotated = fromRotated_;
			inventory_.placeAt(held_, from_);
			Result result = inventory_.equipFrom(from_, id);
			if (result.outcome == Outcome::Done)
			{
				holding_ = false;
				message_.clear();
				return;
			}
			held_ = inventory_.takeAt(from_, held_.count).item;
			held_.rotated = turned;
			explain(result.outcome, catalog_.slotName(slot));
			return;
		}
		message_.clear();
		if (!inventory_.equippedIn(id))
			return;
		Outcome outcome = inventory_.unequip(id).outcome;
		if (outcome == Outcome::ItemsOutside)
			message_ = "Clear items from extended slots first.";
		else if (outcome != Outcome::Done)
			explain(outcome, "");
	}

	std::string Page::slotLine() const
	{
		if (selected_ < 0)
			return "";
		const Item* worn = inventory_.equippedIn(catalog_.slotId(selected_));
		return catalog_.slotName(selected_) + ": " + (worn ? itemName(worn->type) : "empty");
	}

	void Page::click(const Layout& layout, position2d<s32> point)
	{
		Cell cell{ 0, 0 };
		if (layout.cellAt(point, cell))
			clickCell(cell);
		else
			clickSlot(layout.slotAt(point));
	}

	std::string Page::iconPath(const std::string& type) const
	{
		const ItemType* found = catalog_.itemType(type);
		return std::string(ICON_FOLDER) + (found ? found->icon : "");
	}

}
