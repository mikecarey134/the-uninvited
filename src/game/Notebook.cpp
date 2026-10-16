#include "Notebook.h"
#include <algorithm>
#include <string>

using namespace irr;
using namespace core;
using namespace video;

namespace
{
	const s32 margin = 24;
	const f32 tabOut = 0.09f;
	const f32 tabHeight = 0.11f;
	const s32 pushIn = 14;
	const SColor shade(160, 0, 0, 0);
	const SColor ink(255, 62, 44, 28);
	const SColor faded(255, 150, 150, 150);
	const SColor bright(255, 255, 255, 255);
	const char* const spreadFile = "assets/hud/notebook_spread.png";
	const char* const tabsFile = "assets/hud/bookmark_tabs.png";
	const char* const titleFont = "assets/fonts/button.png";
	const char* const bodyFont = "assets/fonts/body.png";
	const rect<s32> spreadArt(14, 9, 1647, 917);
	const rect<s32> tabArt[Notebook::TABS] = { rect<s32>(444, 48, 1030, 277), rect<s32>(445, 314, 1028, 525),
		rect<s32>(446, 563, 1045, 792), rect<s32>(453, 810, 1034, 1031) };

	std::wstring wide(const std::string& text)
	{
		return std::wstring(text.begin(), text.end());
	}

	rect<s32> spreadArea(dimension2d<u32> screen)
	{
		f32 shape = (f32)spreadArt.getWidth() / spreadArt.getHeight();
		s32 height = (s32)screen.Height - 2 * margin;
		s32 width = (s32)(height * shape);
		s32 room = (s32)screen.Width - 2 * margin;
		s32 out = (s32)(width * tabOut);
		if (width + out > room)
		{
			width = (s32)(room / (1.0f + tabOut));
			height = (s32)(width / shape);
			out = (s32)(width * tabOut);
		}
		s32 left = ((s32)screen.Width - width - out) / 2;
		s32 top = ((s32)screen.Height - height) / 2;
		return rect<s32>(left, top, left + width, top + height);
	}

	rect<s32> pageArea(const rect<s32>& spread, f32 from, f32 to)
	{
		return rect<s32>(spread.UpperLeftCorner.X + (s32)(spread.getWidth() * from), spread.UpperLeftCorner.Y + (s32)(spread.getHeight() * 0.14f),
			spread.UpperLeftCorner.X + (s32)(spread.getWidth() * to), spread.UpperLeftCorner.Y + (s32)(spread.getHeight() * 0.84f));
	}

	void write(Canvas& canvas, FontHandle font, const std::string& text, const rect<s32>& area, bool centred = false)
	{
		canvas.drawText(font, wide(text).c_str(), area, ink, centred, true, false);
	}

	void row(Canvas& canvas, FontHandle font, const rect<s32>& area, s32 y, const std::string& name, int value)
	{
		write(canvas, font, name, rect<s32>(area.UpperLeftCorner.X, y, area.LowerRightCorner.X - 60, y + 26));
		std::wstring number = std::to_wstring(value);
		s32 width = (s32)canvas.textSize(font, number.c_str()).Width;
		canvas.drawText(font, number.c_str(), rect<s32>(area.LowerRightCorner.X - width, y, area.LowerRightCorner.X, y + 26), ink, false, true, false);
	}
}

const int Notebook::CHARACTER;
const int Notebook::INVENTORY;
const int Notebook::MAP;
const int Notebook::NOTES;
const int Notebook::TABS;

Notebook::Notebook(std::function<bool()> blocked, std::function<void()> opened)
	: blocked_(blocked), opened_(opened), open_(false), page_(CHARACTER), screen_(1280, 720)
{
}

void Notebook::open(int page)
{
	page_ = page;
	if (!open_ && opened_)
		opened_();
	open_ = true;
}

void Notebook::toggle(int page)
{
	if (open_ && page_ == page)
		close();
	else
		open(page);
}

bool Notebook::onEvent(const SEvent& event)
{
	if (event.EventType == EET_MOUSE_INPUT_EVENT)
	{
		if (!open_)
			return false;
		if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN)
			for (int tab = 0; tab < TABS; ++tab)
				if (tabArea(tab, screen_).isPointInside(position2d<s32>(event.MouseInput.X, event.MouseInput.Y)))
					page_ = tab;
		return true;
	}
	if (event.EventType != EET_KEY_INPUT_EVENT || !event.KeyInput.PressedDown || (blocked_ && blocked_()))
		return false;
	switch (event.KeyInput.Key)
	{
	case KEY_KEY_C:
		toggle(CHARACTER);
		return true;
	case KEY_KEY_I:
		toggle(INVENTORY);
		return true;
	case KEY_ESCAPE:
		if (!open_)
			return false;
		close();
		return true;
	case KEY_OEM_3:
		return false;
	default:
		return open_;
	}
}

rect<s32> Notebook::tabArea(int tab, dimension2d<u32> screen) const
{
	rect<s32> spread = spreadArea(screen);
	s32 height = (s32)(spread.getHeight() * tabHeight);
	s32 width = height * tabArt[tab].getWidth() / tabArt[tab].getHeight();
	s32 top = spread.UpperLeftCorner.Y + (s32)(spread.getHeight() * 0.1f) + tab * (height + height / 3);
	s32 left = spread.LowerRightCorner.X + (s32)(spread.getWidth() * tabOut) - width;
	if (tab == page_)
		left -= pushIn;
	return rect<s32>(left, top, left + width, top + height);
}

void Notebook::draw(Canvas& canvas, const character::CharacterSheet& sheet)
{
	if (!open_)
		return;
	screen_ = canvas.screenSize();
	canvas.drawRect(rect<s32>(0, 0, (s32)screen_.Width, (s32)screen_.Height), shade);
	TextureHandle tabs = canvas.loadTexture(tabsFile);
	for (int tab = 0; tab < TABS; ++tab)
		if (tab != page_)
			canvas.drawImage(tabs, tabArea(tab, screen_), tabArt[tab], faded);
	rect<s32> spread = spreadArea(screen_);
	canvas.drawImage(canvas.loadTexture(spreadFile), spread, spreadArt, bright);
	canvas.drawImage(tabs, tabArea(page_, screen_), tabArt[page_], bright);

	rect<s32> left = pageArea(spread, 0.15f, 0.46f);
	rect<s32> right = pageArea(spread, 0.56f, 0.82f);
	FontHandle title = canvas.loadFont(titleFont);
	FontHandle body = canvas.loadFont(bodyFont);
	rect<s32> heading(left.UpperLeftCorner.X, left.UpperLeftCorner.Y, left.LowerRightCorner.X, left.UpperLeftCorner.Y + 32);
	rect<s32> middle(left.UpperLeftCorner.X, left.getCenter().Y - 16, left.LowerRightCorner.X, left.getCenter().Y + 16);
	switch (page_)
	{
	case CHARACTER:
	{
		write(canvas, title, "CHARACTER SHEET", heading);
		s32 y = heading.LowerRightCorner.Y + 16;
		write(canvas, body, sheet.profession().name, rect<s32>(left.UpperLeftCorner.X, y, left.LowerRightCorner.X, y + 26));
		y += 44;
		const int values[] = { sheet.getAgility(), sheet.getEndurance(), sheet.getGuile(), sheet.getIntelligence(), sheet.getStrength() };
		for (int i = 0; i < character::ATTRIBUTE_COUNT; ++i, y += 30)
			row(canvas, body, left, y, character::attributeName(i), values[i]);
		const int skills[] = { sheet.getSmithing(), sheet.getChemistry(), sheet.getMedicine(), sheet.getCooking(), sheet.getSurvivalism(),
			sheet.getLockpicking(), sheet.getElectronics(), sheet.getEngineering(), sheet.getSpeech(), sheet.getConstruction() };
		y = heading.LowerRightCorner.Y + 16;
		for (int i = 0; i < character::SKILL_COUNT; ++i, y += 28)
			row(canvas, body, right, y, character::skillName(i), skills[i]);
		if (sheet.hasBloodline())
			write(canvas, body, "Bloodline: " + sheet.bloodlineName() + " (" + character::attributeName(sheet.bloodlineAttribute()) + ")",
				rect<s32>(right.UpperLeftCorner.X, y + 14, right.LowerRightCorner.X, y + 40));
		break;
	}
	case INVENTORY:
		write(canvas, title, "INVENTORY", heading);
		write(canvas, body, "Empty", middle, true);
		break;
	case MAP:
		write(canvas, title, "MAP - Coming Soon", middle, true);
		break;
	default:
		write(canvas, title, "NOTES - Coming Soon", middle, true);
		break;
	}
}
