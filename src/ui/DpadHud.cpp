#include "DpadHud.h"
#include "IconArt.h"
#include <algorithm>
#include <cmath>

using namespace irr;
using namespace core;
using namespace video;

namespace
{
	const f32 idleOpacity = 0.35f;
	const f32 fadeSeconds = 0.225f;
	const f32 pulseSeconds = 0.5f;
	const s32 margin = 12;
	const char* const labelFont = "assets/fonts/label.png";
	const std::string iconFolder = "assets/ui/icons/";
	// UP gold, RIGHT red, DOWN cyan, LEFT green.
	const SColor slotColours[4] = { SColor(255, 0xd4, 0xa8, 0x43), SColor(255, 0xc4, 0x40, 0x40), SColor(255, 0x4a, 0x9e, 0xbb),
		SColor(255, 0x6b, 0x9a, 0x5a) };
	const char* const rings[4] = { "rings/gold.png", "rings/red.png", "rings/cyan.png", "rings/green.png" };
	const SColor ghost(255, 140, 140, 140);
	const SColor backing(255, 12, 11, 9);
	const SColor frameLine(255, 190, 185, 170);
	const SColor neutral(255, 215, 210, 200);
	const SColor pill(255, 0, 0, 0);
	const SColor white(255, 255, 255, 255);
	const f32 pillOpacity = 0.65f;

	SColor shade(SColor colour, f32 alpha, f32 brightness = 1.0f)
	{
		return SColor((u32)(std::min(std::max(alpha, 0.0f), 1.0f) * 255.0f), (u32)std::min(colour.getRed() * brightness, 255.0f),
			(u32)std::min(colour.getGreen() * brightness, 255.0f), (u32)std::min(colour.getBlue() * brightness, 255.0f));
	}

	std::wstring wide(const std::string& text)
	{
		return std::wstring(text.begin(), text.end());
	}

	void chevron(Canvas& canvas, const rect<s32>& wedge, int direction, SColor colour)
	{
		position2d<s32> c = wedge.getCenter();
		s32 r = wedge.getWidth() / 6;
		s32 edge = wedge.getWidth() / 2 - r;
		static const s32 ox[4] = { 0, 1, 0, -1 };
		static const s32 oy[4] = { -1, 0, 1, 0 };
		position2d<s32> tip(c.X + ox[direction] * edge, c.Y + oy[direction] * edge);
		position2d<s32> back(tip.X - ox[direction] * r, tip.Y - oy[direction] * r);
		position2d<s32> side(oy[direction] * r, ox[direction] * r);
		canvas.drawLine(back + side, tip, colour);
		canvas.drawLine(back - side, tip, colour);
	}

	void circle(Canvas& canvas, position2d<s32> centre, f32 radius, SColor colour)
	{
		const int segments = 32;
		for (int i = 0; i < segments; ++i)
		{
			f32 a = i * 6.2831853f / segments;
			f32 b = (i + 1) * 6.2831853f / segments;
			canvas.drawLine(position2d<s32>(centre.X + (s32)(std::cos(a) * radius), centre.Y + (s32)(std::sin(a) * radius)),
				position2d<s32>(centre.X + (s32)(std::cos(b) * radius), centre.Y + (s32)(std::sin(b) * radius)), colour);
		}
	}

	// A dark pill behind a line of text, so it reads on bright ground and dark walls alike; corners are clipped to round it off.
	void pillText(Canvas& canvas, FontHandle font, const std::wstring& text, position2d<s32> anchor, int align, s32 line, SColor colour, f32 opacity)
	{
		s32 width = (s32)canvas.textSize(font, text.c_str()).Width;
		s32 left = align < 0 ? anchor.X - width : (align == 0 ? anchor.X - width / 2 : anchor.X);
		rect<s32> area(left, anchor.Y - line / 2, left + width, anchor.Y - line / 2 + line);
		const s32 padX = 5;
		const s32 padY = 2;
		const s32 corner = 2;
		rect<s32> outer(area.UpperLeftCorner.X - padX, area.UpperLeftCorner.Y - padY, area.LowerRightCorner.X + padX, area.LowerRightCorner.Y + padY);
		SColor back = shade(pill, opacity * pillOpacity);
		canvas.drawRect(rect<s32>(outer.UpperLeftCorner.X + corner, outer.UpperLeftCorner.Y, outer.LowerRightCorner.X - corner, outer.LowerRightCorner.Y), back);
		canvas.drawRect(rect<s32>(outer.UpperLeftCorner.X, outer.UpperLeftCorner.Y + corner, outer.UpperLeftCorner.X + corner, outer.LowerRightCorner.Y - corner), back);
		canvas.drawRect(rect<s32>(outer.LowerRightCorner.X - corner, outer.UpperLeftCorner.Y + corner, outer.LowerRightCorner.X, outer.LowerRightCorner.Y - corner), back);
		canvas.drawText(font, text.c_str(), area, colour, true, true, false);
	}
}

void DpadHud::setState(DpadState state)
{
	state_ = state;
}

void DpadHud::setTarget(const std::string& name, const std::string& iconPath)
{
	targetName_ = name;
	if (iconPath != targetIconPath_)
		targetIcon_ = nullptr;
	targetIconPath_ = iconPath;
}

void DpadHud::setSlot(DpadDirection dir, const std::string& label, const std::string& iconPath)
{
	if (dir == DpadDirection::NONE)
		return;
	Slot& slot = slots_[(int)dir];
	slot.available = true;
	slot.label = label;
	if (iconPath != slot.iconPath)
		slot.icon = nullptr;
	slot.iconPath = iconPath;
}

void DpadHud::clearSlot(DpadDirection dir)
{
	if (dir != DpadDirection::NONE)
		slots_[(int)dir] = Slot();
}

void DpadHud::setSelectedDirection(DpadDirection dir)
{
	if (dir != selected_)
		pulseClock_ = 0.0f;
	selected_ = dir;
}

void DpadHud::clearSelection()
{
	selected_ = DpadDirection::NONE;
}

void DpadHud::setActionPrompt(const std::string& text)
{
	prompt_ = text;
}

void DpadHud::update(f32 deltaTime)
{
	f32 goal = state_ == DpadState::IDLE ? idleOpacity : 1.0f;
	f32 step = deltaTime * (1.0f - idleOpacity) / fadeSeconds;
	opacity_ = opacity_ < goal ? std::min(opacity_ + step, goal) : std::max(opacity_ - step, goal);
	pulseClock_ = std::fmod(pulseClock_ + deltaTime, pulseSeconds);
}

TextureHandle DpadHud::texture(Canvas& canvas, const std::string& iconPath, TextureHandle& cached)
{
	if (!cached && !iconPath.empty())
		cached = canvas.loadTexture((iconFolder + iconPath).c_str());
	return cached;
}

void DpadHud::render(Canvas& canvas)
{
	dimension2d<u32> screen = canvas.screenSize();
	f32 scale = screen.Height / 1080.0f;
	// About 126px square at 1080p, scaled with the screen height, with room on the left for the LEFT slot's label.
	s32 side = std::max((s32)(126.0f * scale + 0.5f), 60);
	s32 cell = side / 3;
	s32 labelRoom = (s32)(150.0f * scale + 0.5f);
	FontHandle font = canvas.loadFont(labelFont);
	s32 line = (s32)canvas.textSize(font, L"Ag").Height;
	rect<s32> box(margin + labelRoom, (s32)screen.Height - margin - line - 6 - side, margin + labelRoom + side, (s32)screen.Height - margin - line - 6);
	position2d<s32> origin = box.UpperLeftCorner;
	const rect<s32> wedges[4] = { rect<s32>(origin.X + cell, origin.Y, origin.X + 2 * cell, origin.Y + cell),
		rect<s32>(origin.X + 2 * cell, origin.Y + cell, origin.X + 3 * cell, origin.Y + 2 * cell),
		rect<s32>(origin.X + cell, origin.Y + 2 * cell, origin.X + 2 * cell, origin.Y + 3 * cell),
		rect<s32>(origin.X, origin.Y + cell, origin.X + cell, origin.Y + 2 * cell) };
	rect<s32> centre(origin.X + cell, origin.Y + cell, origin.X + 2 * cell, origin.Y + 2 * cell);
	bool live = state_ != DpadState::IDLE;
	f32 wave = std::sin(pulseClock_ / pulseSeconds * 6.2831853f);
	f32 pulse = 0.9f + 0.1f * wave;
	// Sizes are of the visible art, not the padded PNG canvas: the ring as seen, and the icon filling about 70% of it.
	s32 ringSize = cell * 21 / 20;
	s32 iconSize = ringSize * 68 / 100;

	// The plus-shaped frame: a dark see-through cross with a thin outline.
	SColor back = shade(backing, opacity_ * 0.45f);
	canvas.drawRect(rect<s32>(origin.X + cell, origin.Y, origin.X + 2 * cell, origin.Y + 3 * cell), back);
	canvas.drawRect(wedges[3], back);
	canvas.drawRect(wedges[1], back);
	SColor edge = shade(frameLine, opacity_ * 0.5f);
	const position2d<s32> outline[12] = { position2d<s32>(origin.X + cell, origin.Y), position2d<s32>(origin.X + 2 * cell, origin.Y),
		position2d<s32>(origin.X + 2 * cell, origin.Y + cell), position2d<s32>(origin.X + 3 * cell, origin.Y + cell),
		position2d<s32>(origin.X + 3 * cell, origin.Y + 2 * cell), position2d<s32>(origin.X + 2 * cell, origin.Y + 2 * cell),
		position2d<s32>(origin.X + 2 * cell, origin.Y + 3 * cell), position2d<s32>(origin.X + cell, origin.Y + 3 * cell),
		position2d<s32>(origin.X + cell, origin.Y + 2 * cell), position2d<s32>(origin.X, origin.Y + 2 * cell),
		position2d<s32>(origin.X, origin.Y + cell), position2d<s32>(origin.X + cell, origin.Y + cell) };
	for (int i = 0; i < 12; ++i)
		canvas.drawLine(outline[i], outline[(i + 1) % 12], edge);
	circle(canvas, centre.getCenter(), cell * 0.42f, shade(neutral, opacity_ * (live ? 0.9f : 0.5f)));

	for (int i = 0; i < 4; ++i)
	{
		const Slot& slot = slots_[i];
		bool shown = live && slot.available;
		bool chosen = shown && (int)selected_ == i;
		position2d<s32> middle = wedges[i].getCenter();
		// Idle and unavailable slots are just the frame's arrow: faint rings there read as muddy discs.
		if (!shown)
		{
			chevron(canvas, wedges[i], i, shade(ghost, opacity_ * (live ? 0.6f : 1.0f)));
			continue;
		}
		f32 swell = (wave + 1.0f) / 2.0f;
		s32 size = chosen ? ringSize + (s32)(ringSize * 0.08f * swell) : ringSize;
		canvas.drawImage(texture(canvas, rings[i], ringTextures_[i]), artRect(rings[i], middle, size), shade(white, opacity_));
		// The selected ring glows: bright rims in its colour that pulse just outside it.
		if (chosen)
			for (int rim = 0; rim < 3; ++rim)
				circle(canvas, middle, size / 2.0f + 1.0f + rim, shade(slotColours[i], opacity_ * (0.5f + 0.5f * swell) * (1.0f - rim * 0.3f), 1.3f));
		if (TextureHandle icon = texture(canvas, slot.iconPath, slots_[i].icon))
			canvas.drawImage(icon, artRect(slot.iconPath, middle, iconSize), shade(white, opacity_));
		if (slot.label.empty())
			continue;
		const rect<s32>& w = wedges[i];
		std::wstring label = wide(slot.label);
		s32 width = (s32)canvas.textSize(font, label.c_str()).Width;
		position2d<s32> anchor;
		int align = 0;
		if (i == (int)DpadDirection::UP)
			anchor = position2d<s32>(middle.X, w.UpperLeftCorner.Y - line / 2 - 4);
		else if (i == (int)DpadDirection::DOWN)
			anchor = position2d<s32>(middle.X, w.LowerRightCorner.Y + line / 2 + 4);
		else if (i == (int)DpadDirection::RIGHT)
		{
			anchor = position2d<s32>(w.LowerRightCorner.X + 8, middle.Y);
			align = 1;
		}
		else
		{
			anchor = position2d<s32>(w.UpperLeftCorner.X - 8, middle.Y);
			align = -1;
		}
		// A label that would leave the screen flips to the inward side of its slot.
		if (align == -1 && anchor.X - width - 5 < 0)
		{
			anchor.X = w.LowerRightCorner.X + 8;
			align = 1;
		}
		else if (align == 1 && anchor.X + width + 5 > (s32)screen.Width)
		{
			anchor.X = w.UpperLeftCorner.X - 8;
			align = -1;
		}
		pillText(canvas, font, label, anchor, align, line, shade(slotColours[i], opacity_, chosen ? pulse : 1.0f), opacity_);
	}
	if (!live)
		return;

	if (TextureHandle icon = texture(canvas, targetIconPath_, targetIcon_))
		canvas.drawImage(icon, artRect(targetIconPath_, centre.getCenter(), (s32)(cell * 0.58f)), shade(white, opacity_));
	s32 above = wedges[0].UpperLeftCorner.Y - line - 10;
	if (!targetName_.empty())
	{
		above -= line / 2 + 4;
		pillText(canvas, font, wide(targetName_), position2d<s32>(box.getCenter().X, above), 0, line, shade(white, opacity_), opacity_);
	}
	if (selected_ != DpadDirection::NONE && slots_[(int)selected_].available && !prompt_.empty())
	{
		above -= line + 8;
		pillText(canvas, font, wide("X: " + prompt_), position2d<s32>(box.getCenter().X, above), 0, line, shade(white, opacity_), opacity_);
	}
}
