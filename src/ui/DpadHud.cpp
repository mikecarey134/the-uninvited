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
	const f32 pulseSeconds = 0.5f;
	// One entrance timeline: the frame brightens over its first 200ms, and each slot's ring, icon and label pop in over 130ms,
	// starting 40ms in and 35ms apart (up, right, down, left), so the whole entrance is 275ms. Leaving runs it backwards and faster
	// (about 200ms), so content goes first and the frame dims last. Ease-out on the way in reads as ease-in on the way out.
	const f32 frameSeconds = 0.2f;
	const f32 contentStart = 0.04f;
	const f32 slotStagger = 0.035f;
	const f32 slotSeconds = 0.13f;
	const f32 revealSeconds = contentStart + 3 * slotStagger + slotSeconds;
	const f32 exitSpeed = revealSeconds / 0.2f;
	const f32 popSeconds = 0.15f;    // a picked slot swells 10% and settles
	const f32 popGrowth = 0.1f;
	const f32 switchSeconds = 0.15f; // the old target's content fades out over the first half, the new one in over the second
	const f32 attentionDelay = 0.25f; // a lone slot glows once, gently, just after it has popped in
	const f32 attentionSeconds = 0.6f;
	const f32 entranceScale = 0.8f;
	const s32 margin = 12;
	const char* const labelFont = "assets/fonts/label.png";
	const std::string iconFolder = "assets/ui/icons/";
	// By slot: UP cyan (Skill), RIGHT green (Guile), DOWN cyan (second Skill), LEFT red (Force). Normal is X, not a slot.
	const SColor slotColours[4] = { SColor(255, 0x4a, 0x9e, 0xbb), SColor(255, 0x6b, 0x9a, 0x5a), SColor(255, 0x4a, 0x9e, 0xbb),
		SColor(255, 0xc4, 0x40, 0x40) };
	const char* const rings[4] = { "rings/cyan.png", "rings/green.png", "rings/cyan.png", "rings/red.png" };
	const SColor ghost(255, 140, 140, 140);
	const SColor backing(255, 12, 11, 9);
	const SColor frameLine(255, 190, 185, 170);
	const SColor neutral(255, 215, 210, 200);
	const SColor pill(255, 0, 0, 0);
	const SColor white(255, 255, 255, 255);
	const f32 pillOpacity = 0.65f;

	f32 unit(f32 value)
	{
		return std::min(std::max(value, 0.0f), 1.0f);
	}

	f32 easeOut(f32 progress)
	{
		f32 rest = 1.0f - unit(progress);
		return 1.0f - rest * rest * rest;
	}

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
	// Switching from one target straight to another keeps the frame up and crossfades the content.
	if (state_ != DpadState::IDLE && timeline_ > 0.0f && (name != targetName_ || iconPath != targetIconPath_))
	{
		for (int i = 0; i < 4; ++i)
			oldSlots_[i] = slots_[i];
		oldName_ = targetName_;
		oldIconPath_ = targetIconPath_;
		oldIcon_ = targetIcon_;
		switchClock_ = 0.0f;
	}
	attentionClock_ = 0.0f;
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
	{
		pulseClock_ = 0.0f;
		popClock_ = 0.0f;
	}
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
	if (state_ == DpadState::IDLE)
		timeline_ = std::max(timeline_ - deltaTime * exitSpeed, 0.0f);
	else
		timeline_ = std::min(timeline_ + deltaTime, revealSeconds);
	opacity_ = idleOpacity + (1.0f - idleOpacity) * easeOut(timeline_ / frameSeconds);
	pulseClock_ = std::fmod(pulseClock_ + deltaTime, pulseSeconds);
	popClock_ = std::min(popClock_ + deltaTime, popSeconds);
	switchClock_ = std::min(switchClock_ + deltaTime, switchSeconds);
	attentionClock_ = std::min(attentionClock_ + deltaTime, attentionDelay + attentionSeconds);
}

f32 DpadHud::reveal(int order) const
{
	return easeOut((timeline_ - contentStart - order * slotStagger) / slotSeconds);
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
	Layout at;
	at.screen = screen;
	at.cell = side / 3;
	s32 labelRoom = (s32)(150.0f * scale + 0.5f);
	at.font = canvas.loadFont(labelFont);
	at.line = (s32)canvas.textSize(at.font, L"Ag").Height;
	at.box = rect<s32>(margin + labelRoom, (s32)screen.Height - margin - at.line - 6 - side, margin + labelRoom + side,
		(s32)screen.Height - margin - at.line - 6);
	s32 cell = at.cell;
	position2d<s32> origin = at.box.UpperLeftCorner;
	at.wedges[0] = rect<s32>(origin.X + cell, origin.Y, origin.X + 2 * cell, origin.Y + cell);
	at.wedges[1] = rect<s32>(origin.X + 2 * cell, origin.Y + cell, origin.X + 3 * cell, origin.Y + 2 * cell);
	at.wedges[2] = rect<s32>(origin.X + cell, origin.Y + 2 * cell, origin.X + 2 * cell, origin.Y + 3 * cell);
	at.wedges[3] = rect<s32>(origin.X, origin.Y + cell, origin.X + cell, origin.Y + 2 * cell);
	at.centre = rect<s32>(origin.X + cell, origin.Y + cell, origin.X + 2 * cell, origin.Y + 2 * cell);
	// Sizes are of the visible art, not the padded PNG canvas: the ring as seen, and the icon filling about 70% of it.
	at.ringSize = cell * 21 / 20;
	at.iconSize = at.ringSize * 68 / 100;

	// The plus-shaped frame: a dark see-through cross with a thin outline.
	SColor back = shade(backing, opacity_ * 0.45f);
	canvas.drawRect(rect<s32>(origin.X + cell, origin.Y, origin.X + 2 * cell, origin.Y + 3 * cell), back);
	canvas.drawRect(at.wedges[3], back);
	canvas.drawRect(at.wedges[1], back);
	SColor edge = shade(frameLine, opacity_ * 0.5f);
	const position2d<s32> outline[12] = { position2d<s32>(origin.X + cell, origin.Y), position2d<s32>(origin.X + 2 * cell, origin.Y),
		position2d<s32>(origin.X + 2 * cell, origin.Y + cell), position2d<s32>(origin.X + 3 * cell, origin.Y + cell),
		position2d<s32>(origin.X + 3 * cell, origin.Y + 2 * cell), position2d<s32>(origin.X + 2 * cell, origin.Y + 2 * cell),
		position2d<s32>(origin.X + 2 * cell, origin.Y + 3 * cell), position2d<s32>(origin.X + cell, origin.Y + 3 * cell),
		position2d<s32>(origin.X + cell, origin.Y + 2 * cell), position2d<s32>(origin.X, origin.Y + 2 * cell),
		position2d<s32>(origin.X, origin.Y + cell), position2d<s32>(origin.X + cell, origin.Y + cell) };
	for (int i = 0; i < 12; ++i)
		canvas.drawLine(outline[i], outline[(i + 1) % 12], edge);
	circle(canvas, at.centre.getCenter(), cell * 0.42f, shade(neutral, opacity_ * (0.5f + 0.4f * reveal(0))));

	// A target switch: the old content fades out over the first half, then the new content fades in.
	f32 half = switchSeconds / 2.0f;
	bool switching = switchClock_ < switchSeconds;
	f32 oldFade = switching ? 1.0f - easeOut(switchClock_ / half) : 0.0f;
	f32 newFade = switching ? easeOut((switchClock_ - half) / half) : 1.0f;

	// Arrows show where a slot holds nothing, and under an available slot until its ring has popped in.
	for (int i = 0; i < 4; ++i)
	{
		f32 shown = slots_[i].available ? reveal(i) * newFade : 0.0f;
		if (switching && oldSlots_[i].available)
			shown = std::max(shown, reveal(i) * oldFade);
		if (shown < 1.0f)
			chevron(canvas, at.wedges[i], i, shade(ghost, opacity_ * (timeline_ > 0.0f ? 0.6f : 1.0f) * (1.0f - shown)));
	}
	if (switching && oldFade > 0.0f)
		drawContent(canvas, at, oldSlots_, oldName_, oldIconPath_, oldIcon_, oldFade, false);
	if (newFade > 0.0f)
		drawContent(canvas, at, slots_, targetName_, targetIconPath_, targetIcon_, newFade, true);
}

void DpadHud::drawContent(Canvas& canvas, const Layout& at, Slot (&slots)[4], const std::string& name, const std::string& iconPath,
	TextureHandle& icon, f32 fade, bool current)
{
	f32 wave = std::sin(pulseClock_ / pulseSeconds * 6.2831853f);
	f32 pulse = 0.9f + 0.1f * wave;
	f32 swell = (wave + 1.0f) / 2.0f;
	int available = 0;
	for (int i = 0; i < 4; ++i)
		available += slots[i].available;
	for (int i = 0; i < 4; ++i)
	{
		const Slot& slot = slots[i];
		f32 shown = reveal(i) * fade;
		if (!slot.available || shown <= 0.0f)
			continue;
		bool chosen = current && (int)selected_ == i;
		f32 alpha = opacity_ * shown;
		position2d<s32> middle = at.wedges[i].getCenter();
		// Pops in from 80%; a picked slot swells 10% and settles, then breathes with the pulse.
		f32 grow = entranceScale + (1.0f - entranceScale) * shown;
		if (chosen)
			grow *= 1.0f + popGrowth * std::sin(3.1415927f * easeOut(popClock_ / popSeconds)) + 0.08f * swell * easeOut(popClock_ / popSeconds);
		s32 size = (s32)(at.ringSize * grow + 0.5f);
		canvas.drawImage(texture(canvas, rings[i], (current ? ringTextures_ : oldRingTextures_)[i]), artRect(rings[i], middle, size), shade(white, alpha));
		// The selected ring glows: bright rims in its colour that pulse just outside it. A lone slot glows once on arrival.
		f32 glow = 0.0f;
		if (chosen)
			glow = 0.5f + 0.5f * swell;
		else if (current && available == 1 && selected_ == DpadDirection::NONE)
			glow = 0.6f * std::sin(3.1415927f * unit((attentionClock_ - attentionDelay) / attentionSeconds));
		if (glow > 0.01f)
			for (int rim = 0; rim < 3; ++rim)
				circle(canvas, middle, size / 2.0f + 1.0f + rim, shade(slotColours[i], alpha * glow * (1.0f - rim * 0.3f), 1.3f));
		if (TextureHandle art = texture(canvas, slot.iconPath, slots[i].icon))
			canvas.drawImage(art, artRect(slot.iconPath, middle, (s32)(at.iconSize * grow + 0.5f)), shade(white, alpha));
		if (slot.label.empty())
			continue;
		const rect<s32>& w = at.wedges[i];
		std::wstring label = wide(slot.label);
		s32 width = (s32)canvas.textSize(at.font, label.c_str()).Width;
		position2d<s32> anchor;
		int align = 0;
		if (i == (int)DpadDirection::UP)
			anchor = position2d<s32>(middle.X, w.UpperLeftCorner.Y - at.line / 2 - 4);
		else if (i == (int)DpadDirection::DOWN)
			anchor = position2d<s32>(middle.X, w.LowerRightCorner.Y + at.line / 2 + 4);
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
		else if (align == 1 && anchor.X + width + 5 > (s32)at.screen.Width)
		{
			anchor.X = w.UpperLeftCorner.X - 8;
			align = -1;
		}
		pillText(canvas, at.font, label, anchor, align, at.line, shade(slotColours[i], alpha, chosen ? pulse : 1.0f), alpha);
	}

	f32 centre = reveal(0) * fade;
	if (centre <= 0.0f)
		return;
	f32 alpha = opacity_ * centre;
	if (TextureHandle art = texture(canvas, iconPath, icon))
		canvas.drawImage(art, artRect(iconPath, at.centre.getCenter(), (s32)(at.cell * 0.58f * (entranceScale + (1.0f - entranceScale) * centre))),
			shade(white, alpha));
	s32 above = at.wedges[0].UpperLeftCorner.Y - at.line - 10;
	if (!name.empty())
	{
		above -= at.line / 2 + 4;
		pillText(canvas, at.font, wide(name), position2d<s32>(at.box.getCenter().X, above), 0, at.line, shade(white, alpha), alpha);
	}
	if (current && selected_ != DpadDirection::NONE && slots[(int)selected_].available && !prompt_.empty())
	{
		above -= at.line + 8;
		pillText(canvas, at.font, wide("X: " + prompt_), position2d<s32>(at.box.getCenter().X, above), 0, at.line, shade(white, alpha), alpha);
	}
}
