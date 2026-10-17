#include "InventoryScreen.h"
#include <algorithm>
#include <string>

using namespace irr;
using namespace core;
using namespace video;
using namespace inventory;

namespace
{
	const SColor shade(170, 0, 0, 0);
	const SColor panelFill(235, 20, 20, 22);
	const SColor panelEdge(255, 70, 70, 74);
	const SColor text(255, 232, 226, 214);
	const SColor muted(255, 150, 146, 138);
	const SColor warning(255, 226, 104, 84);
	const SColor emptyCells(255, 28, 28, 31);
	const SColor gridLine(45, 255, 255, 255);
	const SColor boneLine(60, 255, 255, 255);
	const SColor slotFill(255, 34, 34, 38);
	const SColor slotEdge(120, 255, 255, 255);
	const SColor itemFill(255, 60, 56, 50);
	const SColor itemEdge(150, 230, 220, 200);
	const SColor heldFill(140, 80, 76, 70);
	const SColor badge(220, 10, 10, 10);
	const SColor icon(255, 238, 234, 226);
	const char* const titleFont = "assets/fonts/button.png";
	const char* const bodyFont = "assets/fonts/body.png";
	const char* const labelFont = "assets/fonts/label.png";

	std::wstring wide(const std::string& line)
	{
		return std::wstring(line.begin(), line.end());
	}

	rect<s32> square(const rect<s32>& area, s32 inset)
	{
		s32 side = std::min(area.getWidth(), area.getHeight()) - 2 * inset;
		position2d<s32> centre = area.getCenter();
		return rect<s32>(centre.X - side / 2, centre.Y - side / 2, centre.X - side / 2 + side, centre.Y - side / 2 + side);
	}
}

InventoryScreen::InventoryScreen(std::function<bool()> blocked, std::function<void()> opened) : blocked_(blocked), opened_(opened)
{
}

void InventoryScreen::open()
{
	if (!open_ && opened_)
		opened_();
	open_ = true;
}

void InventoryScreen::close()
{
	if (page_)
		page_->cancel();
	open_ = false;
}

rect<s32> InventoryScreen::panel() const
{
	s32 width = std::min((s32)screen_.Width - 80, 1040);
	s32 height = std::min((s32)screen_.Height - 80, 600);
	s32 left = ((s32)screen_.Width - width) / 2;
	s32 top = ((s32)screen_.Height - height) / 2;
	return rect<s32>(left, top, left + width, top + height);
}

backpack::Layout InventoryScreen::layout() const
{
	rect<s32> area = panel();
	s32 width = area.getWidth();
	s32 top = area.UpperLeftCorner.Y + 56;
	s32 bottom = area.LowerRightCorner.Y - 20;
	rect<s32> doll(area.UpperLeftCorner.X + 28, top, area.UpperLeftCorner.X + width * 36 / 100, bottom);
	rect<s32> grid(area.UpperLeftCorner.X + width * 40 / 100, top, area.LowerRightCorner.X - 28, bottom);
	return backpack::Layout(page_->catalog(), doll, grid, page_->inventory().columns(), page_->inventory().rows());
}

bool InventoryScreen::onEvent(const SEvent& event)
{
	if (event.EventType == EET_MOUSE_INPUT_EVENT)
	{
		if (!open_)
			return false;
		mouse_ = position2d<s32>(event.MouseInput.X, event.MouseInput.Y);
		if (page_ && event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN)
			page_->click(layout(), mouse_);
		else if (page_ && event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN)
			page_->cancel();
		return true;
	}
	if (event.EventType != EET_KEY_INPUT_EVENT || !event.KeyInput.PressedDown || (blocked_ && blocked_()))
		return false;
	switch (event.KeyInput.Key)
	{
	case KEY_KEY_I:
		if (open_)
			close();
		else
			open();
		return true;
	case KEY_ESCAPE:
		if (!open_)
			return false;
		close();
		return true;
	case KEY_KEY_R:
		if (open_ && page_)
		{
			backpack::Layout grid = layout();
			Cell hovered{ 0, 0 };
			bool over = grid.cellAt(mouse_, hovered);
			page_->rotate(hovered, over);
		}
		return open_;
	case KEY_KEY_C:
	case KEY_OEM_3:
		return false;
	default:
		return open_;
	}
}

void InventoryScreen::draw(Canvas& canvas)
{
	if (!open_)
		return;
	screen_ = canvas.screenSize();
	canvas.drawRect(rect<s32>(0, 0, (s32)screen_.Width, (s32)screen_.Height), shade);
	rect<s32> area = panel();
	canvas.drawRect(area, panelFill);
	canvas.drawRectOutline(area, panelEdge);
	FontHandle title = canvas.loadFont(titleFont);
	FontHandle body = canvas.loadFont(bodyFont);
	FontHandle label = canvas.loadFont(labelFont);
	canvas.drawText(title, L"INVENTORY", rect<s32>(area.UpperLeftCorner.X + 28, area.UpperLeftCorner.Y + 14, area.LowerRightCorner.X,
		area.UpperLeftCorner.Y + 46), text, false, true, false);
	if (!page_)
	{
		canvas.drawText(body, L"Inventory unavailable", area, muted, true, true, false);
		return;
	}

	backpack::Layout grid = layout();
	const Catalog& catalog = page_->catalog();
	const Inventory& carried = page_->inventory();
	canvas.drawText(label, L"EQUIPMENT", rect<s32>(grid.left.UpperLeftCorner.X, grid.left.UpperLeftCorner.Y, grid.left.LowerRightCorner.X,
		grid.left.UpperLeftCorner.Y + 24), muted, false, true, false);
	canvas.drawText(label, L"BACKPACK", rect<s32>(grid.right.UpperLeftCorner.X, grid.right.UpperLeftCorner.Y, grid.right.LowerRightCorner.X,
		grid.right.UpperLeftCorner.Y + 24), muted, false, true, false);

	auto centre = [&](const std::string& id) {
		for (int i = 0; i < catalog.slotCount(); ++i)
			if (catalog.slotId(i) == id)
				return grid.slots[i].getCenter();
		return grid.left.getCenter();
	};
	const char* const bones[][2] = { { "head", "chest" }, { "neck", "chest" }, { "chest", "belt" }, { "belt", "boots" },
		{ "chest", "gloves" }, { "chest", "main_hand" }, { "chest", "off_hand" }, { "chest", "back" } };
	for (const auto& bone : bones)
		canvas.drawLine(centre(bone[0]), centre(bone[1]), boneLine);
	for (int i = 0; i < catalog.slotCount(); ++i)
	{
		const rect<s32>& box = grid.slots[i];
		canvas.drawRect(box, slotFill);
		canvas.drawRectOutline(box, slotEdge);
		if (const Item* worn = carried.equippedIn(catalog.slotId(i)))
			canvas.drawImage(canvas.loadTexture(page_->iconPath(worn->type).c_str()), square(box, 6), icon);
		canvas.drawText(label, wide(catalog.slotName(i)).c_str(), rect<s32>(box.UpperLeftCorner.X - 40, box.LowerRightCorner.Y + 3,
			box.LowerRightCorner.X + 40, box.LowerRightCorner.Y + 19), muted, true, false, false);
	}
	canvas.drawText(body, wide(page_->slotLine()).c_str(), rect<s32>(grid.left.UpperLeftCorner.X, grid.left.LowerRightCorner.Y - 28,
		grid.left.LowerRightCorner.X, grid.left.LowerRightCorner.Y), text, false, true, false);

	canvas.drawRect(grid.grid, emptyCells);
	for (int column = 1; column < grid.columns; ++column)
	{
		s32 x = grid.grid.UpperLeftCorner.X + column * grid.cell;
		canvas.drawLine(position2d<s32>(x, grid.grid.UpperLeftCorner.Y), position2d<s32>(x, grid.grid.LowerRightCorner.Y), gridLine);
	}
	for (int row = 1; row < grid.rows; ++row)
	{
		s32 y = grid.grid.UpperLeftCorner.Y + row * grid.cell;
		canvas.drawLine(position2d<s32>(grid.grid.UpperLeftCorner.X, y), position2d<s32>(grid.grid.LowerRightCorner.X, y), gridLine);
	}
	canvas.drawRectOutline(grid.grid, gridLine);

	auto drawItem = [&](const Item& item, const rect<s32>& footprint, SColor fill) {
		canvas.drawRect(footprint, fill);
		canvas.drawRectOutline(footprint, itemEdge);
		canvas.drawImage(canvas.loadTexture(page_->iconPath(item.type).c_str()), square(footprint, 6), icon);
		if (item.count > 1)
		{
			std::wstring count = std::to_wstring(item.count);
			dimension2d<u32> size = canvas.textSize(label, count.c_str());
			rect<s32> spot(footprint.LowerRightCorner.X - (s32)size.Width - 6, footprint.LowerRightCorner.Y - (s32)size.Height - 3,
				footprint.LowerRightCorner.X - 1, footprint.LowerRightCorner.Y - 1);
			canvas.drawRect(spot, badge);
			canvas.drawText(label, count.c_str(), rect<s32>(spot.UpperLeftCorner.X + 3, spot.UpperLeftCorner.Y + 1, spot.LowerRightCorner.X,
				spot.LowerRightCorner.Y), text, false, false, false);
		}
	};
	for (const Placed& placed : carried.backpackContents())
		drawItem(placed.item, grid.footprint(placed), itemFill);
	canvas.drawText(body, wide(page_->message()).c_str(), rect<s32>(grid.right.UpperLeftCorner.X, grid.grid.LowerRightCorner.Y + 8,
		grid.right.LowerRightCorner.X, grid.grid.LowerRightCorner.Y + 34), warning, false, true, false);

	if (page_->isHolding())
	{
		rect<s32> footprint = grid.footprint(Placed{ page_->held(), Cell{ 0, 0 } });
		position2d<s32> corner(mouse_.X - page_->grabOffset().column * grid.cell - grid.cell / 2,
			mouse_.Y - page_->grabOffset().row * grid.cell - grid.cell / 2);
		footprint += corner - footprint.UpperLeftCorner;
		drawItem(page_->held(), footprint, heldFill);
	}
}
