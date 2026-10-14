#include "doctest.h"
#include "MenuSkin.h"
#include "Theme.h"
#include <irrlicht.h>
#include <string>

using namespace irr;
using namespace gui;

namespace
{
	struct Gui
	{
		Gui() : device(createDevice(video::EDT_NULL, core::dimension2d<u32>(1280, 720))), env(device->getGUIEnvironment()) {}
		~Gui() { device->drop(); }

		IrrlichtDevice* device;
		IGUIEnvironment* env;
	};
}

TEST_CASE("every menu font loads with all 96 glyphs, the checkmark included, at its intended height")
{
	Gui gui;
	struct Font { const char* name; u32 height; };
	const Font fonts[] = { { "title", 71 }, { "heading", 45 }, { "button", 24 }, { "body", 21 }, { "label", 17 } };
	for (const Font& font : fonts)
	{
		INFO(font.name);
		IGUIFont* loaded = gui.env->getFont((std::string(ASSETS_DIR) + "/fonts/" + font.name + ".png").c_str());
		REQUIRE(loaded != nullptr);
		CHECK(static_cast<IGUIFontBitmap*>(loaded)->getSpriteBank()->getSprites().size() == 96);
		CHECK(loaded->getDimension(L"Ag").Height == font.height);
		CHECK(loaded->getDimension(L"\x7f").Width > loaded->getDimension(L".").Width);
		CHECK(loaded->getDimension(L"W").Width > loaded->getDimension(L"i").Width);
	}
}

TEST_CASE("the skin leaves menu buttons to the game and draws scroll bar buttons itself")
{
	Gui gui;
	IGUIButton* menuButton = gui.env->addButton(core::rect<s32>(0, 0, 100, 40));
	IGUIListBox* list = gui.env->addListBox(core::rect<s32>(0, 50, 200, 100));
	for (int i = 0; i < 20; ++i)
		list->addItem(L"row");
	CHECK(MenuSkin::isDrawnByGame(menuButton));
	CHECK_FALSE(MenuSkin::isDrawnByGame(list));
	CHECK_FALSE(MenuSkin::isDrawnByGame(nullptr));

	const IGUIElement* scrollBar = nullptr;
	for (IGUIElement* child : list->getChildren())
		if (child->getType() == EGUIET_SCROLL_BAR)
			scrollBar = child;
	REQUIRE(scrollBar != nullptr);
	int arrows = 0;
	for (IGUIElement* child : scrollBar->getChildren())
		if (child->getType() == EGUIET_BUTTON)
		{
			++arrows;
			CHECK_FALSE(MenuSkin::isDrawnByGame(child));
		}
	CHECK(arrows == 2);
}

TEST_CASE("the skin takes the theme's colours for text, selection and inputs")
{
	Gui gui;
	MenuSkin* skin = new MenuSkin(gui.env, gui.env->getSkin());
	gui.env->setSkin(skin);
	skin->drop();
	IGUISkin* active = gui.env->getSkin();
	CHECK(active->getColor(EGDC_HIGH_LIGHT) == theme::accentFill);
	CHECK(active->getColor(EGDC_HIGH_LIGHT_TEXT) == theme::text);
	CHECK(active->getColor(EGDC_BUTTON_TEXT) == theme::dim);
	CHECK(active->getColor(EGDC_EDITABLE) == theme::input);
	CHECK(active->getSize(EGDS_TEXT_DISTANCE_X) == 12);
	active->setColor(EGDC_BUTTON_TEXT, theme::accent);
	CHECK(active->getColor(EGDC_BUTTON_TEXT) == theme::accent);
}
