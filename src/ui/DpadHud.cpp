#include "DpadHud.h"
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
	const SColor ghost(255, 140, 140, 140);
	const SColor backing(255, 12, 11, 9);
	const SColor white(255, 255, 255, 255);

	SColor shade(SColor colour, f32 alpha, f32 brightness = 1.0f)
	{
		return SColor((u32)(std::min(std::max(alpha, 0.0f), 1.0f) * 255.0f), (u32)(colour.getRed() * brightness),
			(u32)(colour.getGreen() * brightness), (u32)(colour.getBlue() * brightness));
	}

	std::wstring wide(const std::string& text)
	{
		return std::wstring(text.begin(), text.end());
	}

	rect<s32> square(position2d<s32> centre, s32 side)
	{
		return rect<s32>(centre.X - side / 2, centre.Y - side / 2, centre.X - side / 2 + side, centre.Y - side / 2 + side);
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
	// About 120px square at 1080p, scaled with the screen height.
	s32 side = std::max((s32)(120.0f * screen.Height / 1080.0f + 0.5f), 60);
	s32 cell = side / 3;
	FontHandle font = canvas.loadFont(labelFont);
	s32 line = (s32)canvas.textSize(font, L"Ag").Height;
	rect<s32> box(margin + side / 2, (s32)screen.Height - margin - line - side, margin + side / 2 + side, (s32)screen.Height - margin - line);
	position2d<s32> origin = box.UpperLeftCorner;
	const rect<s32> wedges[4] = { rect<s32>(origin.X + cell, origin.Y, origin.X + 2 * cell, origin.Y + cell),
		rect<s32>(origin.X + 2 * cell, origin.Y + cell, origin.X + 3 * cell, origin.Y + 2 * cell),
		rect<s32>(origin.X + cell, origin.Y + 2 * cell, origin.X + 2 * cell, origin.Y + 3 * cell),
		rect<s32>(origin.X, origin.Y + cell, origin.X + cell, origin.Y + 2 * cell) };
	rect<s32> centre(origin.X + cell, origin.Y + cell, origin.X + 2 * cell, origin.Y + 2 * cell);
	bool live = state_ != DpadState::IDLE;
	f32 pulse = 0.9f + 0.1f * std::sin(pulseClock_ / pulseSeconds * 6.2831853f);

	canvas.drawRect(centre, shade(backing, opacity_ * 0.6f));
	for (int i = 0; i < 4; ++i)
	{
		const Slot& slot = slots_[i];
		bool shown = live && slot.available;
		bool chosen = shown && (int)selected_ == i;
		f32 brightness = chosen ? pulse : 1.0f;
		SColor colour = shown ? slotColours[i] : ghost;
		canvas.drawRect(wedges[i], shade(colour, opacity_ * (shown ? 0.55f : 0.35f), brightness));
		chevron(canvas, wedges[i], i, shade(shown ? white : ghost, opacity_, brightness));
		if (!shown)
			continue;
		if (chosen)
			canvas.drawRectOutline(wedges[i], shade(white, opacity_, brightness));
		if (TextureHandle icon = texture(canvas, slot.iconPath, slots_[i].icon))
			canvas.drawImage(icon, square(wedges[i].getCenter(), cell * 4 / 5), shade(white, opacity_, brightness));
		if (slot.label.empty())
			continue;
		std::wstring label = wide(slot.label);
		s32 width = (s32)canvas.textSize(font, label.c_str()).Width;
		const rect<s32>& w = wedges[i];
		rect<s32> area;
		if (i == (int)DpadDirection::UP)
			area = rect<s32>(w.getCenter().X - width / 2 - 4, w.UpperLeftCorner.Y - line - 2, w.getCenter().X + width / 2 + 4, w.UpperLeftCorner.Y - 2);
		else if (i == (int)DpadDirection::RIGHT)
			area = rect<s32>(w.LowerRightCorner.X + 4, w.getCenter().Y - line / 2, w.LowerRightCorner.X + 8 + width, w.getCenter().Y + line / 2 + 1);
		else if (i == (int)DpadDirection::DOWN)
			area = rect<s32>(w.getCenter().X - width / 2 - 4, w.LowerRightCorner.Y + 2, w.getCenter().X + width / 2 + 4, w.LowerRightCorner.Y + line + 2);
		else
			area = rect<s32>(w.UpperLeftCorner.X - 8 - width, w.getCenter().Y - line / 2, w.UpperLeftCorner.X - 4, w.getCenter().Y + line / 2 + 1);
		canvas.drawText(font, label.c_str(), area, shade(slotColours[i], opacity_, brightness), true, true, false);
	}
	if (!live)
		return;

	if (TextureHandle icon = texture(canvas, targetIconPath_, targetIcon_))
		canvas.drawImage(icon, square(centre.getCenter(), side / 5), shade(white, opacity_));
	if (!targetName_.empty())
	{
		std::wstring name = wide(targetName_);
		s32 top = wedges[0].UpperLeftCorner.Y - 2 * line - 4;
		canvas.drawText(font, name.c_str(), rect<s32>(box.UpperLeftCorner.X - side, top, box.LowerRightCorner.X + side, top + line), shade(white, opacity_),
			true, true, false);
	}
	if (selected_ != DpadDirection::NONE && slots_[(int)selected_].available && !prompt_.empty())
	{
		std::wstring prompt = wide("X: " + prompt_);
		s32 top = wedges[0].UpperLeftCorner.Y - 3 * line - 6;
		canvas.drawText(font, prompt.c_str(), rect<s32>(box.UpperLeftCorner.X - side, top, box.LowerRightCorner.X + side, top + line), shade(white, opacity_),
			true, true, false);
	}
}
