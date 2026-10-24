#include "doctest.h"
#include "ButtonGlyphs.h"
#include <fstream>
#include <set>
#include <string>

TEST_CASE("every glyph the registry names is a PNG in the assets, and each family has its own")
{
	const GlyphFamily families[3] = { GlyphFamily::KEYBOARD_MOUSE, GlyphFamily::XBOX, GlyphFamily::PLAYSTATION };
	const char* const folders[3] = { "/kb/", "/xbox/", "/ps/" };
	for (int f = 0; f < 3; ++f)
	{
		std::set<std::string> seen;
		for (int a = 0; a <= (int)GlyphAction::POINTER_SECONDARY; ++a)
		{
			std::string path = glyphPath((GlyphAction)a, families[f]);
			CAPTURE(f);
			CAPTURE(a);
			if (path.empty())
				continue;
			CHECK(path.find(folders[f]) != std::string::npos);
			CHECK(std::ifstream(std::string(ASSETS_DIR) + path.substr(std::string("assets").size())).good());
			CHECK(seen.insert(path).second);
		}
		CHECK(seen.size() >= 9);
	}
}

TEST_CASE("interact is E on the keyboard, X on an Xbox pad and Square on a PlayStation pad, where the game's X runs approaches")
{
	CHECK(std::string(glyphPath(GlyphAction::INTERACT, GlyphFamily::KEYBOARD_MOUSE)) == "assets/textures/glyphs/kb/key_e.png");
	CHECK(std::string(glyphPath(GlyphAction::INTERACT, GlyphFamily::XBOX)) == "assets/textures/glyphs/xbox/x.png");
	CHECK(std::string(glyphPath(GlyphAction::INTERACT, GlyphFamily::PLAYSTATION)) == "assets/textures/glyphs/ps/square.png");
	CHECK(std::string(glyphPath(GlyphAction::CANCEL, GlyphFamily::PLAYSTATION)) == "assets/textures/glyphs/ps/circle.png");
}

TEST_CASE("on the keyboard each D-pad direction shows the number key that runs the same slot")
{
	CHECK(std::string(glyphPath(GlyphAction::DPAD_LEFT, GlyphFamily::KEYBOARD_MOUSE)) == "assets/textures/glyphs/kb/key_1.png");
	CHECK(std::string(glyphPath(GlyphAction::DPAD_UP, GlyphFamily::KEYBOARD_MOUSE)) == "assets/textures/glyphs/kb/key_2.png");
	CHECK(std::string(glyphPath(GlyphAction::DPAD_DOWN, GlyphFamily::KEYBOARD_MOUSE)) == "assets/textures/glyphs/kb/key_3.png");
	CHECK(std::string(glyphPath(GlyphAction::DPAD_RIGHT, GlyphFamily::KEYBOARD_MOUSE)) == "assets/textures/glyphs/kb/key_4.png");
	CHECK(std::string(glyphPath(GlyphAction::DPAD_LEFT, GlyphFamily::XBOX)) == "assets/textures/glyphs/xbox/dpad_left.png");
}

TEST_CASE("the family follows the device used last, and the pad's name decides which pad family")
{
	CHECK(glyphFamily(InputMode::KEYBOARD_MOUSE, PadFamily::PLAYSTATION) == GlyphFamily::KEYBOARD_MOUSE);
	CHECK(glyphFamily(InputMode::GAMEPAD, PadFamily::XBOX) == GlyphFamily::XBOX);
	CHECK(glyphFamily(InputMode::GAMEPAD, PadFamily::PLAYSTATION) == GlyphFamily::PLAYSTATION);
	for (const char* sony : { "Sony Interactive Entertainment Wireless Controller", "Sony PLAYSTATION(R)3 Controller", "DualSense Wireless Controller",
		"PS4 Controller" })
	{
		CAPTURE(sony);
		CHECK(padFamilyFromName(sony) == PadFamily::PLAYSTATION);
	}
	for (const char* other : { "Microsoft X-Box 360 pad", "Xbox Wireless Controller", "Nintendo Switch Pro Controller", "Virtual Test Pad", "" })
	{
		CAPTURE(other);
		CHECK(padFamilyFromName(other) == PadFamily::XBOX);
	}
	CHECK(padFamilyFromName(nullptr) == PadFamily::XBOX);
}
