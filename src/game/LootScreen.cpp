#include "LootScreen.h"
#include "BackpackView.h"
#include <algorithm>

using namespace irr;
using namespace core;
using namespace video;
using namespace inventory;

namespace
{
	const SColor rowFill(255, 34, 34, 38);
	const SColor rowEdge(90, 255, 255, 255);

	std::wstring wide(const std::string& line)
	{
		return std::wstring(line.begin(), line.end());
	}
}

const s32 LootScreen::ROW_HEIGHT;

LootScreen::LootScreen(std::function<bool()> blocked, std::function<void()> opened, std::function<loot::Container*()> nearby)
	: blocked_(blocked), opened_(opened), nearby_(nearby)
{
}

void LootScreen::open()
{
	loot::Container* found = nearby_ ? nearby_() : nullptr;
	if (!found)
		return;
	container_ = found;
	container_->searched = true;
	scroll_ = 0;
	message_.clear();
	if (!open_ && opened_)
		opened_();
	open_ = true;
}

void LootScreen::close()
{
	open_ = false;
	container_ = nullptr;
}

rect<s32> LootScreen::panel() const
{
	s32 width = std::min((s32)screen_.Width - 80, 1160);
	s32 height = std::min((s32)screen_.Height - 80, 600);
	s32 left = ((s32)screen_.Width - width) / 2;
	s32 top = ((s32)screen_.Height - height) / 2;
	return rect<s32>(left, top, left + width, top + height);
}

rect<s32> LootScreen::list() const
{
	rect<s32> area = panel();
	return rect<s32>(area.UpperLeftCorner.X + 28, area.UpperLeftCorner.Y + 84, area.UpperLeftCorner.X + 328, area.LowerRightCorner.Y - 56);
}

int LootScreen::visibleRows() const
{
	return std::max(1, list().getHeight() / ROW_HEIGHT);
}

rect<s32> LootScreen::row(int index) const
{
	rect<s32> area = list();
	s32 top = area.UpperLeftCorner.Y + (index - scroll_) * ROW_HEIGHT;
	return rect<s32>(area.UpperLeftCorner.X, top, area.LowerRightCorner.X, top + ROW_HEIGHT - 4);
}

backpack::Layout LootScreen::layout() const
{
	rect<s32> area = panel();
	rect<s32> shelf = list();
	s32 top = area.UpperLeftCorner.Y + 56;
	s32 bottom = area.LowerRightCorner.Y - 20;
	rect<s32> doll(shelf.LowerRightCorner.X + 28, top, shelf.LowerRightCorner.X + 268, bottom);
	rect<s32> grid(doll.LowerRightCorner.X + 20, top, area.LowerRightCorner.X - 28, bottom);
	return backpack::Layout(*catalog_, doll, grid, carried_->columns(), carried_->rows());
}

bool LootScreen::onEvent(const SEvent& event)
{
	if (event.EventType == EET_MOUSE_INPUT_EVENT)
	{
		if (!open_)
			return false;
		position2d<s32> point(event.MouseInput.X, event.MouseInput.Y);
		if (event.MouseInput.Event == EMIE_MOUSE_WHEEL && container_)
		{
			int most = std::max(0, (int)container_->items.size() - visibleRows());
			scroll_ = std::min(most, std::max(0, scroll_ - (event.MouseInput.Wheel > 0.0f ? 1 : -1)));
		}
		else if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN && container_ && carried_ && catalog_)
		{
			Cell cell{ 0, 0 };
			if (list().isPointInside(point))
			{
				int index = scroll_ + (point.Y - list().UpperLeftCorner.Y) / ROW_HEIGHT;
				if (index < (int)container_->items.size())
					message_ = loot::take(*container_, index, *carried_);
				scroll_ = std::min(scroll_, std::max(0, (int)container_->items.size() - visibleRows()));
			}
			else if (layout().cellAt(point, cell))
				message_ = loot::give(*container_, cell, *carried_);
		}
		return true;
	}
	if (event.EventType != EET_KEY_INPUT_EVENT || !event.KeyInput.PressedDown || (blocked_ && blocked_()))
		return false;
	switch (event.KeyInput.Key)
	{
	case KEY_KEY_E:
		if (open_)
		{
			close();
			return true;
		}
		open();
		return open_;
	case KEY_ESCAPE:
		if (!open_)
			return false;
		close();
		return true;
	case KEY_KEY_I:
	case KEY_KEY_C:
	case KEY_OEM_3:
		return false;
	default:
		return open_;
	}
}

void LootScreen::draw(Canvas& canvas)
{
	if (!open_ || !container_ || !carried_ || !catalog_)
		return;
	screen_ = canvas.screenSize();
	canvas.drawRect(rect<s32>(0, 0, (s32)screen_.Width, (s32)screen_.Height), backpack::look::shade);
	rect<s32> area = panel();
	backpack::drawPanel(canvas, area, wide(container_->kind));
	FontHandle label = canvas.loadFont(backpack::look::labelFont);
	FontHandle body = canvas.loadFont(backpack::look::bodyFont);
	rect<s32> shelf = list();
	canvas.drawText(label, L"CONTENTS", rect<s32>(shelf.UpperLeftCorner.X, shelf.UpperLeftCorner.Y - 28, shelf.LowerRightCorner.X,
		shelf.UpperLeftCorner.Y - 4), backpack::look::muted, false, true, false);
	std::string used = std::to_string(container_->items.size()) + " / " + std::to_string(container_->capacity);
	s32 usedWidth = (s32)canvas.textSize(label, wide(used).c_str()).Width;
	canvas.drawText(label, wide(used).c_str(), rect<s32>(shelf.LowerRightCorner.X - usedWidth, shelf.UpperLeftCorner.Y - 28, shelf.LowerRightCorner.X,
		shelf.UpperLeftCorner.Y - 4), backpack::look::muted, false, true, false);
	for (int i = scroll_; i < (int)container_->items.size() && i < scroll_ + visibleRows(); ++i)
	{
		const Item& item = container_->items[i];
		rect<s32> line = row(i);
		canvas.drawRect(line, rowFill);
		canvas.drawRectOutline(line, rowEdge);
		rect<s32> mark(line.UpperLeftCorner.X + 4, line.UpperLeftCorner.Y + 4, line.UpperLeftCorner.X + line.getHeight() - 4,
			line.LowerRightCorner.Y - 4);
		canvas.drawImage(canvas.loadTexture(backpack::iconPath(*catalog_, item.type).c_str()), mark, backpack::look::icon);
		const ItemType* type = catalog_->itemType(item.type);
		canvas.drawText(body, wide(type ? type->name : item.type).c_str(), rect<s32>(mark.LowerRightCorner.X + 10, line.UpperLeftCorner.Y,
			line.LowerRightCorner.X - 50, line.LowerRightCorner.Y), backpack::look::text, false, true, false);
		if (item.count > 1)
			canvas.drawText(label, wide("x" + std::to_string(item.count)).c_str(), rect<s32>(line.LowerRightCorner.X - 48, line.UpperLeftCorner.Y,
				line.LowerRightCorner.X - 8, line.LowerRightCorner.Y), backpack::look::muted, false, true, false);
	}

	backpack::Layout grid = layout();
	canvas.drawText(label, L"EQUIPMENT", rect<s32>(grid.left.UpperLeftCorner.X, grid.left.UpperLeftCorner.Y, grid.left.LowerRightCorner.X,
		grid.left.UpperLeftCorner.Y + 24), backpack::look::muted, false, true, false);
	canvas.drawText(label, L"BACKPACK", rect<s32>(grid.right.UpperLeftCorner.X, grid.right.UpperLeftCorner.Y, grid.right.LowerRightCorner.X,
		grid.right.UpperLeftCorner.Y + 24), backpack::look::muted, false, true, false);
	backpack::drawEquipment(canvas, *carried_, *catalog_, grid, "");
	backpack::drawGrid(canvas, *carried_, *catalog_, grid);
	canvas.drawText(body, wide(message_).c_str(), rect<s32>(area.UpperLeftCorner.X + 28, area.LowerRightCorner.Y - 46, area.LowerRightCorner.X - 28,
		area.LowerRightCorner.Y - 16), backpack::look::warning, false, true, false);
}
