#include "BackpackView.h"
#include <algorithm>

using namespace irr;
using namespace core;
using namespace video;
using namespace inventory;

namespace
{
	const SColor emptyCells(255, 28, 28, 31);
	const SColor gridLine(45, 255, 255, 255);
	const SColor boneLine(60, 255, 255, 255);
	const SColor slotFill(255, 34, 34, 38);
	const SColor slotEdge(120, 255, 255, 255);
	const SColor itemFill(255, 60, 56, 50);
	const SColor itemEdge(150, 230, 220, 200);
	const SColor heldFill(140, 80, 76, 70);
	const SColor badge(220, 10, 10, 10);

	std::wstring wide(const std::string& line)
	{
		return std::wstring(line.begin(), line.end());
	}
}

namespace backpack
{
	std::string iconPath(const Catalog& catalog, const std::string& type)
	{
		const ItemType* found = catalog.itemType(type);
		return std::string(ICON_FOLDER) + (found ? found->icon : "");
	}

	rect<s32> square(const rect<s32>& area, s32 inset)
	{
		s32 side = std::min(area.getWidth(), area.getHeight()) - 2 * inset;
		position2d<s32> centre = area.getCenter();
		return rect<s32>(centre.X - side / 2, centre.Y - side / 2, centre.X - side / 2 + side, centre.Y - side / 2 + side);
	}

	void drawPanel(Canvas& canvas, const rect<s32>& panel, const std::wstring& title)
	{
		canvas.drawRect(panel, look::panelFill);
		canvas.drawRectOutline(panel, look::panelEdge);
		canvas.drawText(canvas.loadFont(look::titleFont), title.c_str(), rect<s32>(panel.UpperLeftCorner.X + 28, panel.UpperLeftCorner.Y + 14,
			panel.LowerRightCorner.X, panel.UpperLeftCorner.Y + 46), look::text, false, true, false);
	}

	void drawEquipment(Canvas& canvas, const Inventory& carried, const Catalog& catalog, const Layout& layout, const std::string& slotLine)
	{
		FontHandle label = canvas.loadFont(look::labelFont);
		auto centre = [&](const std::string& id) {
			for (int i = 0; i < catalog.slotCount(); ++i)
				if (catalog.slotId(i) == id)
					return layout.slots[i].getCenter();
			return layout.left.getCenter();
		};
		const char* const bones[][2] = { { "head", "chest" }, { "neck", "chest" }, { "chest", "belt" }, { "belt", "boots" },
			{ "chest", "gloves" }, { "chest", "main_hand" }, { "chest", "off_hand" }, { "chest", "back" } };
		for (const auto& bone : bones)
			canvas.drawLine(centre(bone[0]), centre(bone[1]), boneLine);
		for (int i = 0; i < catalog.slotCount(); ++i)
		{
			const rect<s32>& box = layout.slots[i];
			canvas.drawRect(box, slotFill);
			canvas.drawRectOutline(box, slotEdge);
			if (const Item* worn = carried.equippedIn(catalog.slotId(i)))
				canvas.drawImage(canvas.loadTexture(iconPath(catalog, worn->type).c_str()), square(box, 6), look::icon);
			canvas.drawText(label, wide(catalog.slotName(i)).c_str(), rect<s32>(box.UpperLeftCorner.X - 40, box.LowerRightCorner.Y + 3,
				box.LowerRightCorner.X + 40, box.LowerRightCorner.Y + 19), look::muted, true, false, false);
		}
		canvas.drawText(canvas.loadFont(look::bodyFont), wide(slotLine).c_str(), rect<s32>(layout.left.UpperLeftCorner.X,
			layout.left.LowerRightCorner.Y - 28, layout.left.LowerRightCorner.X, layout.left.LowerRightCorner.Y), look::text, false, true, false);
	}

	void drawItem(Canvas& canvas, const Catalog& catalog, const Item& item, const rect<s32>& footprint, bool held)
	{
		canvas.drawRect(footprint, held ? heldFill : itemFill);
		canvas.drawRectOutline(footprint, itemEdge);
		canvas.drawImage(canvas.loadTexture(iconPath(catalog, item.type).c_str()), square(footprint, 6), look::icon);
		if (item.count > 1)
		{
			FontHandle label = canvas.loadFont(look::labelFont);
			std::wstring count = std::to_wstring(item.count);
			dimension2d<u32> size = canvas.textSize(label, count.c_str());
			rect<s32> spot(footprint.LowerRightCorner.X - (s32)size.Width - 6, footprint.LowerRightCorner.Y - (s32)size.Height - 3,
				footprint.LowerRightCorner.X - 1, footprint.LowerRightCorner.Y - 1);
			canvas.drawRect(spot, badge);
			canvas.drawText(label, count.c_str(), rect<s32>(spot.UpperLeftCorner.X + 3, spot.UpperLeftCorner.Y + 1, spot.LowerRightCorner.X,
				spot.LowerRightCorner.Y), look::text, false, false, false);
		}
	}

	void drawGrid(Canvas& canvas, const Inventory& carried, const Catalog& catalog, const Layout& layout)
	{
		const rect<s32>& grid = layout.grid;
		canvas.drawRect(grid, emptyCells);
		for (int column = 1; column < layout.columns; ++column)
		{
			s32 x = grid.UpperLeftCorner.X + column * layout.cell;
			canvas.drawLine(position2d<s32>(x, grid.UpperLeftCorner.Y), position2d<s32>(x, grid.LowerRightCorner.Y), gridLine);
		}
		for (int row = 1; row < layout.rows; ++row)
		{
			s32 y = grid.UpperLeftCorner.Y + row * layout.cell;
			canvas.drawLine(position2d<s32>(grid.UpperLeftCorner.X, y), position2d<s32>(grid.LowerRightCorner.X, y), gridLine);
		}
		canvas.drawRectOutline(grid, gridLine);
		for (const Placed& placed : carried.backpackContents())
			drawItem(canvas, catalog, placed.item, layout.footprint(placed), false);
	}
}
