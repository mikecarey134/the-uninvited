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
	const s32 trackX = 136;
	const s32 trackWidth = 120;
	const s32 grooveInset = 6;
	const s32 iconSize = 14;
	const s32 numberX = 264;
	const s32 firstRow = 22;
	const s32 rowStep = 18;
	const SColor seeThrough(200, 255, 255, 255);
	const s32 margin = 12;
	const s32 weaponWidth = 128;
	const s32 weaponHeight = 80;
	const s32 reticleSize = 32;
	const SColor portraitColour(255, 20, 18, 14);
	const SColor opaque(255, 255, 255, 255);
	const char* const labelFont = "assets/fonts/label.png";
	const rect<s32> backingArt(19, 119, 2154, 592);
	const rect<s32> weaponArt(22, 35, 1516, 976);
	const rect<s32> reticleArt(364, 349, 892, 887);
	const rect<s32> frameArt(40, 42, 1215, 1203);
	const rect<s32> trackArt(28, 271, 2144, 487);

	struct Meter
	{
		const char* fill;
		rect<s32> fillArt;
		const char* icon;
		rect<s32> iconArt;
	};

	const Meter meters[3] = {
		{ "assets/hud/fill_hp.png", rect<s32>(33, 290, 2139, 458), "assets/hud/icon_hp.png", rect<s32>(90, 188, 1158, 1113) },
		{ "assets/hud/fill_food.png", rect<s32>(33, 285, 2139, 459), "assets/hud/icon_food.png", rect<s32>(227, 84, 1039, 1181) },
		{ "assets/hud/fill_h2o.png", rect<s32>(46, 227, 2127, 494), "assets/hud/icon_h2o.png", rect<s32>(280, 99, 975, 1152) } };

	std::wstring wide(const std::string& text)
	{
		return std::wstring(text.begin(), text.end());
	}

	void bar(Canvas& canvas, FontHandle font, s32 row, f32 value, f32 most)
	{
		const Meter& meter = meters[row];
		s32 y = firstRow + row * rowStep;
		s32 middle = y + rowStep / 2;
		const rect<s32>& art = meter.iconArt;
		s32 iconWidth = std::max(1, iconSize * art.getWidth() / std::max(art.getWidth(), art.getHeight()));
		s32 iconHeight = std::max(1, iconSize * art.getHeight() / std::max(art.getWidth(), art.getHeight()));
		s32 iconMiddle = trackX - 12;
		canvas.drawImage(canvas.loadTexture(meter.icon), rect<s32>(iconMiddle - iconWidth / 2, middle - iconHeight / 2,
			iconMiddle - iconWidth / 2 + iconWidth, middle - iconHeight / 2 + iconHeight), art, opaque);
		canvas.drawImage(canvas.loadTexture("assets/hud/bar_track.png"), rect<s32>(trackX, middle - 6, trackX + trackWidth, middle + 6),
			trackArt, opaque);
		f32 share = std::min(std::max(value / most, 0.0f), 1.0f);
		s32 groove = trackWidth - 2 * grooveInset;
		s32 fill = (s32)(groove * share);
		if (fill > 0)
		{
			s32 source = (s32)(meter.fillArt.getWidth() * share);
			canvas.drawImage(canvas.loadTexture(meter.fill), rect<s32>(trackX + grooveInset, middle - 3, trackX + grooveInset + fill, middle + 3),
				rect<s32>(meter.fillArt.UpperLeftCorner.X, meter.fillArt.UpperLeftCorner.Y, meter.fillArt.UpperLeftCorner.X + source,
				meter.fillArt.LowerRightCorner.Y), opaque);
		}
		canvas.drawText(font, wide(std::to_string((int)value)).c_str(), rect<s32>(numberX, y, panel.LowerRightCorner.X - 4, y + rowStep),
			theme::text, false, true, false);
	}
}

void drawVitals(Canvas& canvas, const character::CharacterSheet& sheet)
{
	canvas.drawImage(canvas.loadTexture("assets/hud/backing_panel.png"), panel, backingArt, seeThrough);
	canvas.drawRect(portrait, portraitColour);
	canvas.drawRect(rect<s32>(41, 30, 55, 46), theme::dim);
	canvas.drawRect(rect<s32>(31, 50, 65, 75), theme::dim);
	canvas.drawImage(canvas.loadTexture("assets/hud/portrait_frame.png"), portrait, frameArt, opaque);
	FontHandle font = canvas.loadFont(labelFont);
	bar(canvas, font, 0, (f32)sheet.health(), (f32)sheet.maxHealth());
	bar(canvas, font, 1, sheet.hunger(), 100.0f);
	bar(canvas, font, 2, sheet.water(), 100.0f);
	dimension2d<u32> screen = canvas.screenSize();
	s32 right = (s32)screen.Width - margin;
	s32 bottom = (s32)screen.Height - margin;
	canvas.drawImage(canvas.loadTexture("assets/hud/weapon_frame.png"), rect<s32>(right - weaponWidth, bottom - weaponHeight, right, bottom),
		weaponArt, seeThrough);
	position2d<s32> centre((s32)screen.Width / 2, (s32)screen.Height / 2);
	canvas.drawImage(canvas.loadTexture("assets/hud/reticle.png"), rect<s32>(centre.X - reticleSize / 2, centre.Y - reticleSize / 2,
		centre.X + reticleSize / 2, centre.Y + reticleSize / 2), reticleArt, opaque);
}
