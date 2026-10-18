#include "LootScreen.h"
#include "BackpackView.h"
#include "Theme.h"
#include <algorithm>

using namespace irr;
using namespace core;
using namespace video;
using namespace inventory;

namespace
{
	const SColor dim(110, 0, 0, 0);
	const s32 cellSize = 64;
	const s32 padding = 24;
	const s32 titleHeight = 56;
	const s32 gap = 24;
	const s32 bagWidth = 920;
	const s32 bagHeight = 600;

	rect<s32> ring(const rect<s32>& area)
	{
		return rect<s32>(area.UpperLeftCorner.X - 2, area.UpperLeftCorner.Y - 2, area.LowerRightCorner.X + 2, area.LowerRightCorner.Y + 2);
	}
}

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
	message_.clear();
	if (!open_)
	{
		inContainer_ = true;
		slot_ = 0;
		cell_ = Cell{ 0, 0 };
		keyboard_ = true;
		if (opened_)
			opened_();
	}
	open_ = true;
}

void LootScreen::close()
{
	open_ = false;
	container_ = nullptr;
}

int LootScreen::containerColumns() const
{
	if (!container_ || container_->capacity <= 1)
		return 1;
	return container_->capacity <= 4 ? 2 : 4;
}

int LootScreen::containerRows() const
{
	return container_ ? std::max(1, (container_->capacity + containerColumns() - 1) / containerColumns()) : 1;
}

s32 LootScreen::left() const
{
	s32 boxWidth = containerColumns() * cellSize + 2 * padding;
	s32 bag = std::min(bagWidth, (s32)screen_.Width - 48 - boxWidth - gap);
	return ((s32)screen_.Width - boxWidth - gap - bag) / 2;
}

rect<s32> LootScreen::containerPanel() const
{
	s32 width = containerColumns() * cellSize + 2 * padding;
	s32 height = titleHeight + containerRows() * cellSize + padding;
	s32 top = ((s32)screen_.Height - height) / 2;
	return rect<s32>(left(), top, left() + width, top + height);
}

rect<s32> LootScreen::playerPanel() const
{
	s32 start = containerPanel().LowerRightCorner.X + gap;
	s32 width = std::min(bagWidth, (s32)screen_.Width - 48 - containerPanel().getWidth() - gap);
	s32 height = std::min(bagHeight, (s32)screen_.Height - 48);
	s32 top = ((s32)screen_.Height - height) / 2;
	return rect<s32>(start, top, start + width, top + height);
}

rect<s32> LootScreen::containerCell(int index) const
{
	rect<s32> box = containerPanel();
	s32 x = box.UpperLeftCorner.X + padding + (index % containerColumns()) * cellSize;
	s32 y = box.UpperLeftCorner.Y + titleHeight + (index / containerColumns()) * cellSize;
	return rect<s32>(x + 2, y + 2, x + cellSize - 2, y + cellSize - 2);
}

backpack::Layout LootScreen::layout() const
{
	rect<s32> area = playerPanel();
	s32 width = area.getWidth();
	s32 top = area.UpperLeftCorner.Y + 56;
	s32 bottom = area.LowerRightCorner.Y - 20;
	rect<s32> doll(area.UpperLeftCorner.X + 28, top, area.UpperLeftCorner.X + width * 36 / 100, bottom);
	rect<s32> grid(area.UpperLeftCorner.X + width * 40 / 100, top, area.LowerRightCorner.X - 28, bottom);
	return backpack::Layout(*catalog_, doll, grid, carried_->columns(), carried_->rows());
}

bool LootScreen::onEvent(const SEvent& event)
{
	if (event.EventType == EET_MOUSE_INPUT_EVENT)
	{
		if (!open_)
			return false;
		keyboard_ = false;
		if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN && container_ && carried_ && catalog_)
		{
			position2d<s32> point(event.MouseInput.X, event.MouseInput.Y);
			Cell cell{ 0, 0 };
			for (int i = 0; i < container_->capacity; ++i)
				if (containerCell(i).isPointInside(point))
					message_ = loot::take(*container_, i, *carried_);
			if (layout().cellAt(point, cell))
				message_ = loot::give(*container_, cell, *carried_);
		}
		return true;
	}
	if (event.EventType != EET_KEY_INPUT_EVENT || !event.KeyInput.PressedDown || (blocked_ && blocked_()))
		return false;
	EKEY_CODE key = event.KeyInput.Key;
	switch (key)
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
	case KEY_TAB:
	case KEY_UP:
	case KEY_DOWN:
	case KEY_LEFT:
	case KEY_RIGHT:
	{
		if (!open_ || !container_ || !carried_)
			return open_;
		keyboard_ = true;
		int columns = containerColumns();
		bool leaves = key == KEY_TAB || (inContainer_ && key == KEY_RIGHT && (slot_ % columns == columns - 1 || slot_ == container_->capacity - 1)) ||
			(!inContainer_ && key == KEY_LEFT && cell_.column == 0);
		if (leaves)
			inContainer_ = !inContainer_;
		else if (inContainer_)
		{
			int moved = slot_ + (key == KEY_LEFT ? -1 : key == KEY_RIGHT ? 1 : key == KEY_UP ? -columns : columns);
			bool sameRow = key == KEY_UP || key == KEY_DOWN || moved / columns == slot_ / columns;
			if (moved >= 0 && moved < container_->capacity && sameRow)
				slot_ = moved;
		}
		else
		{
			cell_.column = std::max(0, std::min(cell_.column + (key == KEY_LEFT ? -1 : key == KEY_RIGHT ? 1 : 0), carried_->columns() - 1));
			cell_.row = std::max(0, std::min(cell_.row + (key == KEY_UP ? -1 : key == KEY_DOWN ? 1 : 0), carried_->rows() - 1));
		}
		return true;
	}
	case KEY_RETURN:
	case KEY_SPACE:
		if (!open_ || !container_ || !carried_)
			return open_;
		keyboard_ = true;
		message_ = inContainer_ ? loot::take(*container_, slot_, *carried_) : loot::give(*container_, cell_, *carried_);
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
	canvas.drawRect(rect<s32>(0, 0, (s32)screen_.Width, (s32)screen_.Height), dim);
	rect<s32> box = containerPanel();
	backpack::drawPanel(canvas, box, std::wstring(container_->kind.begin(), container_->kind.end()));
	std::vector<rect<s32> > cells;
	for (int i = 0; i < container_->capacity; ++i)
		cells.push_back(containerCell(i));
	backpack::drawCells(canvas, *catalog_, container_->items, cells);

	rect<s32> bag = playerPanel();
	backpack::drawPanel(canvas, bag, L"Inventory");
	backpack::Layout grid = layout();
	backpack::drawEquipment(canvas, *carried_, *catalog_, grid, "");
	backpack::drawGrid(canvas, *carried_, *catalog_, grid);
	if (keyboard_)
		canvas.drawRectOutline(ring(inContainer_ ? containerCell(slot_) : grid.cellArea(cell_)), theme::accent);
	canvas.drawText(canvas.loadFont(backpack::look::bodyFont), std::wstring(message_.begin(), message_.end()).c_str(),
		rect<s32>(grid.right.UpperLeftCorner.X, grid.grid.LowerRightCorner.Y + 8, grid.right.LowerRightCorner.X, grid.grid.LowerRightCorner.Y + 34),
		backpack::look::warning, false, true, false);
}
