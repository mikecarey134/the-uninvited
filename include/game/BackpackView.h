#ifndef BACKPACKVIEW_H
#define BACKPACKVIEW_H

#include <string>
#include <vector>
#include "Backpack.h"
#include "Canvas.h"

namespace backpack
{
	namespace look
	{
		const irr::video::SColor shade(170, 0, 0, 0);
		const irr::video::SColor panelFill(235, 20, 20, 22);
		const irr::video::SColor panelEdge(255, 70, 70, 74);
		const irr::video::SColor text(255, 232, 226, 214);
		const irr::video::SColor muted(255, 150, 146, 138);
		const irr::video::SColor warning(255, 226, 104, 84);
		const irr::video::SColor icon(255, 238, 234, 226);
		const char* const titleFont = "assets/fonts/button.png";
		const char* const bodyFont = "assets/fonts/body.png";
		const char* const labelFont = "assets/fonts/label.png";
	}

	std::string iconPath(const inventory::Catalog& catalog, const std::string& type);
	irr::core::rect<irr::s32> square(const irr::core::rect<irr::s32>& area, irr::s32 inset);
	void drawPanel(Canvas& canvas, const irr::core::rect<irr::s32>& panel, const std::wstring& title);
	void drawEquipment(Canvas& canvas, const inventory::Inventory& carried, const inventory::Catalog& catalog, const Layout& layout,
		const std::string& slotLine);
	void drawCells(Canvas& canvas, const inventory::Catalog& catalog, const std::vector<inventory::Item>& items,
		const std::vector<irr::core::rect<irr::s32> >& cells);
	void drawGrid(Canvas& canvas, const inventory::Inventory& carried, const inventory::Catalog& catalog, const Layout& layout);
	void drawItem(Canvas& canvas, const inventory::Catalog& catalog, const inventory::Item& item, const irr::core::rect<irr::s32>& footprint,
		bool held);
}

#endif
