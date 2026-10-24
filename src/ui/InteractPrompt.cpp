#include "InteractPrompt.h"

using namespace irr;
using namespace core;
using namespace video;

namespace
{
	const char* const labelFont = "assets/fonts/label.png";
	const s32 cap = 28; // the glyph, square
	const s32 gap = 8;
	const s32 pad = 6;
	const SColor pill(170, 0, 0, 0);
	const SColor actionText(255, 245, 242, 235);

	std::wstring wide(const std::string& text)
	{
		return std::wstring(text.begin(), text.end());
	}
}

void drawInteractPrompt(Canvas& canvas, position2d<s32> topCentre, const std::string& glyph, const std::string& action)
{
	if (action.empty())
		return;
	FontHandle font = canvas.loadFont(labelFont);
	std::wstring verb = wide(action);
	s32 verbWidth = (s32)canvas.textSize(font, verb.c_str()).Width;
	s32 width = cap + gap + verbWidth;
	s32 left = topCentre.X - width / 2;
	s32 top = topCentre.Y;
	canvas.drawRect(rect<s32>(left - pad, top - pad, left + width + pad, top + cap + pad), pill);
	if (!glyph.empty())
		canvas.drawImage(canvas.loadTexture(glyph.c_str()), rect<s32>(left, top, left + cap, top + cap));
	canvas.drawText(font, verb.c_str(), rect<s32>(left + cap + gap, top, left + width + 2, top + cap), actionText, false, true, false);
}
