#include "ContextPanel.h"

using namespace irr;
using namespace core;
using namespace video;

namespace
{
	const s32 width = 300;
	const s32 rowHeight = 40;
	const s32 padding = 8;
	const s32 belowCrosshair = 32;
	const s32 iconSize = 28;
	const char* const labelFont = "assets/fonts/label.png";
	const SColor backing(190, 12, 11, 9);
	const SColor highlight(110, 0xd4, 0xa8, 0x43);
	const SColor text(255, 235, 230, 220);
	const SColor hint(255, 150, 145, 135);

	std::wstring wide(const std::string& value)
	{
		return std::wstring(value.begin(), value.end());
	}
}

void ContextPanel::open(const std::vector<ContextRow>& rows)
{
	rows_ = rows;
	open_ = true;
	hovered_ = -1;
}

void ContextPanel::close()
{
	open_ = false;
	hovered_ = -1;
}

void ContextPanel::hover(position2d<s32> at)
{
	hovered_ = rowAt(at);
}

rect<s32> ContextPanel::rowArea(int index) const
{
	s32 left = (s32)screen_.Width / 2 - width / 2;
	s32 top = (s32)screen_.Height / 2 + belowCrosshair + padding + index * rowHeight;
	return rect<s32>(left + padding, top, left + width - padding, top + rowHeight);
}

int ContextPanel::rowAt(position2d<s32> at) const
{
	if (!open_)
		return -1;
	for (int i = 0; i < (int)rows_.size(); ++i)
		if (rowArea(i).isPointInside(at))
			return i;
	return -1;
}

void ContextPanel::render(Canvas& canvas)
{
	screen_ = canvas.screenSize();
	if (!open_ || rows_.empty())
		return;
	rect<s32> first = rowArea(0);
	rect<s32> last = rowArea((int)rows_.size() - 1);
	canvas.drawRect(rect<s32>(first.UpperLeftCorner.X - padding, first.UpperLeftCorner.Y - padding, last.LowerRightCorner.X + padding,
		last.LowerRightCorner.Y + padding), backing);
	FontHandle font = canvas.loadFont(labelFont);
	for (int i = 0; i < (int)rows_.size(); ++i)
	{
		const ContextRow& row = rows_[i];
		rect<s32> area = rowArea(i);
		if (i == hovered_)
			canvas.drawRect(area, highlight);
		position2d<s32> centre = area.getCenter();
		s32 iconLeft = area.UpperLeftCorner.X + 6;
		if (!row.iconPath.empty())
			canvas.drawImage(canvas.loadTexture(("assets/ui/icons/" + row.iconPath).c_str()),
				rect<s32>(iconLeft, centre.Y - iconSize / 2, iconLeft + iconSize, centre.Y + iconSize / 2));
		s32 labelLeft = iconLeft + iconSize + 10;
		canvas.drawText(font, wide(row.label).c_str(), rect<s32>(labelLeft, area.UpperLeftCorner.Y, area.LowerRightCorner.X - 36, area.LowerRightCorner.Y),
			text, false, true, false);
		canvas.drawText(font, std::to_wstring(row.number).c_str(), rect<s32>(area.LowerRightCorner.X - 32, area.UpperLeftCorner.Y,
			area.LowerRightCorner.X - 6, area.LowerRightCorner.Y), hint, true, true, false);
	}
}
