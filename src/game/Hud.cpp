#include "Hud.h"
#include "Theme.h"
#include <algorithm>
#include <string>

using namespace irr;
using namespace core;
using namespace video;

namespace
{
	const f32 scale = 1.75f;
	const s32 margin = 12;
	const s32 reticleSize = 32;
	const SColor seeThrough(200, 255, 255, 255);
	const SColor portraitColour(255, 20, 18, 14);
	const SColor opaque(255, 255, 255, 255);
	const char* const labelFont = "assets/fonts/label.png";
	const char* const fistFile = "engine/gamecorev0.1.1/gamecorev0.1.1/bill/icons/hand.jpg";
	const rect<s32> backingArt(19, 119, 2154, 592);
	const rect<s32> pouchArt(20, 159, 1389, 957);
	const s32 pouchWidth = 280;
	const s32 pouchHeight = 163;
	const s32 slotWidth = 140;
	const s32 slotHeight = 84;
	const rect<s32> fistArt(260, 165, 1093, 1603);
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

	s32 scaled(s32 length)
	{
		return (s32)(length * scale + 0.5f);
	}

	rect<s32> cluster(s32 left, s32 top, s32 right, s32 bottom)
	{
		return rect<s32>(margin + scaled(left), margin + scaled(top), margin + scaled(right), margin + scaled(bottom));
	}

	std::wstring wide(const std::string& text)
	{
		return std::wstring(text.begin(), text.end());
	}

	rect<s32> fitted(const rect<s32>& art, const rect<s32>& room)
	{
		f32 fit = std::min((f32)room.getWidth() / art.getWidth(), (f32)room.getHeight() / art.getHeight());
		s32 width = (s32)(art.getWidth() * fit);
		s32 height = (s32)(art.getHeight() * fit);
		position2d<s32> centre = room.getCenter();
		return rect<s32>(centre.X - width / 2, centre.Y - height / 2, centre.X - width / 2 + width, centre.Y - height / 2 + height);
	}

	void bar(Canvas& canvas, FontHandle font, s32 row, f32 value, f32 most)
	{
		const Meter& meter = meters[row];
		const s32 top = 10 + row * 18;
		canvas.drawImage(canvas.loadTexture(meter.icon), fitted(meter.iconArt, cluster(105, top + 2, 119, top + 16)), meter.iconArt, opaque);
		canvas.drawImage(canvas.loadTexture("assets/hud/bar_track.png"), cluster(124, top + 3, 244, top + 15), trackArt, opaque);
		f32 share = std::min(std::max(value / most, 0.0f), 1.0f);
		rect<s32> groove = cluster(130, top + 6, 238, top + 12);
		s32 fill = (s32)(groove.getWidth() * share);
		if (fill > 0)
		{
			s32 source = (s32)(meter.fillArt.getWidth() * share);
			canvas.drawImage(canvas.loadTexture(meter.fill), rect<s32>(groove.UpperLeftCorner.X, groove.UpperLeftCorner.Y,
				groove.UpperLeftCorner.X + fill, groove.LowerRightCorner.Y), rect<s32>(meter.fillArt.UpperLeftCorner.X,
				meter.fillArt.UpperLeftCorner.Y, meter.fillArt.UpperLeftCorner.X + source, meter.fillArt.LowerRightCorner.Y), opaque);
		}
		canvas.drawText(font, wide(std::to_string((int)value)).c_str(), cluster(252, top, 284, top + 18), theme::text, false, true, false);
	}
}

void drawVitals(Canvas& canvas, const character::CharacterSheet& sheet)
{
	canvas.drawImage(canvas.loadTexture("assets/hud/backing_panel.png"), cluster(0, 0, 288, 72), backingArt, seeThrough);
	rect<s32> portrait = cluster(8, 8, 64, 64);
	canvas.drawRect(portrait, portraitColour);
	canvas.drawRect(cluster(29, 18, 43, 34), theme::dim);
	canvas.drawRect(cluster(19, 38, 53, 63), theme::dim);
	canvas.drawImage(canvas.loadTexture("assets/hud/portrait_frame.png"), portrait, frameArt, opaque);
	FontHandle font = canvas.loadFont(labelFont);
	bar(canvas, font, 0, (f32)sheet.health(), (f32)sheet.maxHealth());
	bar(canvas, font, 1, sheet.hunger(), 100.0f);
	bar(canvas, font, 2, sheet.water(), 100.0f);

	dimension2d<u32> screen = canvas.screenSize();
	s32 right = (s32)screen.Width - margin;
	s32 bottom = (s32)screen.Height - margin;
	rect<s32> pouch(right - pouchWidth, bottom - pouchHeight, right, bottom);
	canvas.drawImage(canvas.loadTexture("assets/hud/weapon_panel.png"), pouch, pouchArt, seeThrough);
	rect<s32> inside(pouch.UpperLeftCorner.X + pouchWidth * 58 / 1000, pouch.UpperLeftCorner.Y + pouchHeight * 164 / 1000,
		pouch.UpperLeftCorner.X + pouchWidth * 833 / 1000, pouch.UpperLeftCorner.Y + pouchHeight * 903 / 1000);
	s32 middle = inside.getCenter().X;
	rect<s32> slot(middle - slotWidth / 2, inside.UpperLeftCorner.Y + 6, middle - slotWidth / 2 + slotWidth,
		inside.UpperLeftCorner.Y + 6 + slotHeight);
	rect<s32> fist = fitted(fistArt, slot);
	canvas.drawImage(canvas.loadTexture(fistFile), fist, fistArt, opaque);
	const wchar_t* ammo = L"--/--";
	s32 textHeight = (s32)canvas.textSize(font, ammo).Height;
	canvas.drawText(font, ammo, rect<s32>(middle - 60, fist.LowerRightCorner.Y + 4, middle + 60, fist.LowerRightCorner.Y + 6 + textHeight),
		theme::text, true, true, false);

	position2d<s32> centre((s32)screen.Width / 2, (s32)screen.Height / 2);
	canvas.drawImage(canvas.loadTexture("assets/hud/reticle.png"), rect<s32>(centre.X - reticleSize / 2, centre.Y - reticleSize / 2,
		centre.X + reticleSize / 2, centre.Y + reticleSize / 2), reticleArt, opaque);
}
