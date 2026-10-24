#include "ButtonGlyphs.h"

GlyphFamily glyphFamily(InputMode mode, PadFamily pad)
{
	if (mode == InputMode::KEYBOARD_MOUSE)
		return GlyphFamily::KEYBOARD_MOUSE;
	return pad == PadFamily::PLAYSTATION ? GlyphFamily::PLAYSTATION : GlyphFamily::XBOX;
}

const char* glyphPath(GlyphAction action, GlyphFamily family)
{
	// PLACEHOLDER art from tools/make_glyph_placeholders.py until real glyphs arrive; the paths stay the same.
	static const char* const table[][3] = {
		// keyboard and mouse                       Xbox                                         PlayStation
		{ "assets/textures/glyphs/kb/key_e.png", "assets/textures/glyphs/xbox/x.png", "assets/textures/glyphs/ps/square.png" },
		{ "assets/textures/glyphs/kb/key_esc.png", "assets/textures/glyphs/xbox/b.png", "assets/textures/glyphs/ps/circle.png" },
		{ "assets/textures/glyphs/kb/key_q.png", "", "" },
		{ "assets/textures/glyphs/kb/key_2.png", "assets/textures/glyphs/xbox/dpad_up.png", "assets/textures/glyphs/ps/dpad_up.png" },
		{ "assets/textures/glyphs/kb/key_4.png", "assets/textures/glyphs/xbox/dpad_right.png", "assets/textures/glyphs/ps/dpad_right.png" },
		{ "assets/textures/glyphs/kb/key_3.png", "assets/textures/glyphs/xbox/dpad_down.png", "assets/textures/glyphs/ps/dpad_down.png" },
		{ "assets/textures/glyphs/kb/key_1.png", "assets/textures/glyphs/xbox/dpad_left.png", "assets/textures/glyphs/ps/dpad_left.png" },
		{ "", "assets/textures/glyphs/xbox/lb.png", "assets/textures/glyphs/ps/l1.png" },
		{ "", "assets/textures/glyphs/xbox/rb.png", "assets/textures/glyphs/ps/r1.png" },
		{ "", "assets/textures/glyphs/xbox/lt.png", "assets/textures/glyphs/ps/l2.png" },
		{ "", "assets/textures/glyphs/xbox/rt.png", "assets/textures/glyphs/ps/r2.png" },
		{ "assets/textures/glyphs/kb/mouse_left.png", "", "" },
		{ "assets/textures/glyphs/kb/mouse_right.png", "", "" },
	};
	return table[(int)action][(int)family];
}
