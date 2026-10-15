#include "Hud.h"
#include "Theme.h"
#include <algorithm>
#include <string>

using namespace irr;
using namespace core;
using namespace video;

namespace
{
	const rect<s32> panel(12, 12, 300, 84);
	const rect<s32> portrait(20, 20, 76, 76);
	const s32 labelX = 86;
	const s32 trackX = 136;
	const s32 trackWidth = 120;
	const s32 numberX = 264;
	const s32 firstRow = 22;
	const s32 rowStep = 18;
	const SColor panelColour(170, 10, 9, 7);
	const SColor portraitColour(255, 20, 18, 14);
	const SColor trackColour(255, 32, 29, 24);
	const char* const labelFont = "assets/fonts/label.png";

	std::wstring wide(const std::string& text)
	{
		return std::wstring(text.begin(), text.end());
	}

	void bar(Canvas& canvas, FontHandle font, s32 row, const char* label, f32 value, f32 most, SColor colour)
	{
		s32 y = firstRow + row * rowStep;
		canvas.drawText(font, wide(label).c_str(), rect<s32>(labelX, y, trackX - 4, y + rowStep), theme::muted, false, true, false);
		rect<s32> track(trackX, y + 7, trackX + trackWidth, y + 12);
		canvas.drawRect(track, trackColour);
		s32 fill = (s32)(trackWidth * std::min(std::max(value / most, 0.0f), 1.0f));
		if (fill > 0)
			canvas.drawRect(rect<s32>(trackX, track.UpperLeftCorner.Y, trackX + fill, track.LowerRightCorner.Y), colour);
		canvas.drawText(font, wide(std::to_string((int)value)).c_str(), rect<s32>(numberX, y, panel.LowerRightCorner.X - 4, y + rowStep),
			theme::text, false, true, false);
	}
}

void drawVitals(Canvas& canvas, const character::CharacterSheet& sheet)
{
	canvas.drawRect(panel, panelColour);
	canvas.drawRect(portrait, portraitColour);
	canvas.drawRect(rect<s32>(41, 30, 55, 46), theme::dim);
	canvas.drawRect(rect<s32>(31, 50, 65, 75), theme::dim);
	canvas.drawRectOutline(portrait, theme::accent);
	FontHandle font = canvas.loadFont(labelFont);
	bar(canvas, font, 0, "HP", (f32)sheet.health(), (f32)sheet.maxHealth(), hudHealth);
	bar(canvas, font, 1, "FOOD", sheet.hunger(), 100.0f, hudFood);
	bar(canvas, font, 2, "H2O", sheet.water(), 100.0f, hudWater);
}
