#include "InventoryScreen.h"
#include "BackpackView.h"
#include <algorithm>
#include <string>

using namespace irr;
using namespace core;
using namespace video;
using namespace inventory;

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
	canvas.drawRect(rect<s32>(0, 0, (s32)screen_.Width, (s32)screen_.Height), backpack::look::shade);
	rect<s32> area = panel();
	backpack::drawPanel(canvas, area, L"INVENTORY");
	if (!page_)
	{
		canvas.drawText(canvas.loadFont(backpack::look::bodyFont), L"Inventory unavailable", area, backpack::look::muted, true, true, false);
		return;
	}
	backpack::Layout grid = layout();
	FontHandle label = canvas.loadFont(backpack::look::labelFont);
	FontHandle body = canvas.loadFont(backpack::look::bodyFont);
	canvas.drawText(label, L"EQUIPMENT", rect<s32>(grid.left.UpperLeftCorner.X, grid.left.UpperLeftCorner.Y, grid.left.LowerRightCorner.X,
		grid.left.UpperLeftCorner.Y + 24), backpack::look::muted, false, true, false);
	canvas.drawText(label, L"BACKPACK", rect<s32>(grid.right.UpperLeftCorner.X, grid.right.UpperLeftCorner.Y, grid.right.LowerRightCorner.X,
		grid.right.UpperLeftCorner.Y + 24), backpack::look::muted, false, true, false);
	backpack::drawEquipment(canvas, page_->inventory(), page_->catalog(), grid, page_->slotLine());
	backpack::drawGrid(canvas, page_->inventory(), page_->catalog(), grid);
	canvas.drawText(body, std::wstring(page_->message().begin(), page_->message().end()).c_str(), rect<s32>(grid.right.UpperLeftCorner.X,
		grid.grid.LowerRightCorner.Y + 8, grid.right.LowerRightCorner.X, grid.grid.LowerRightCorner.Y + 34), backpack::look::warning, false, true, false);
	if (page_->isHolding())
	{
		rect<s32> footprint = grid.footprint(Placed{ page_->held(), Cell{ 0, 0 } });
		position2d<s32> corner(mouse_.X - page_->grabOffset().column * grid.cell - grid.cell / 2,
			mouse_.Y - page_->grabOffset().row * grid.cell - grid.cell / 2);
		footprint += corner - footprint.UpperLeftCorner;
		backpack::drawItem(canvas, page_->catalog(), page_->held(), footprint, true);
	}
}
