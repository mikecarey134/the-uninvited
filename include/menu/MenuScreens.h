#ifndef MENUSCREENS_H
#define MENUSCREENS_H

#include <deque>
#include <random>
#include <string>
#include <irrlicht.h>
#include "Canvas.h"
#include "GuiPanel.h"
#include "MenuFlow.h"

namespace gamecore { class SoundEngine; }

class MenuScreens
{
public:
	enum { GUI_ID_PLAY = 200, GUI_ID_SETTINGS, GUI_ID_QUIT, GUI_ID_BACK, GUI_ID_CHARACTERS, GUI_ID_NEW_CHARACTER,
		GUI_ID_SELECT, GUI_ID_NAME, GUI_ID_GENDER, GUI_ID_PROFESSION, GUI_ID_CREATE, GUI_ID_HOST, GUI_ID_JOIN,
		GUI_ID_ADDRESS, GUI_ID_PORT, GUI_ID_CONNECT, GUI_ID_LEAVE };

	MenuScreens(irr::IrrlichtDevice* device, Canvas& canvas, menu::MenuFlow& flow, gamecore::SoundEngine* sound,
		const std::deque<std::string>& status);

	bool onGuiEvent(const irr::SEvent& event);
	void back();
	void draw();
	void hide();

private:
	void build();
	bool clicked(irr::s32 id);
	void click();
	void label(FontHandle font, const std::string& text, irr::s32 x1, irr::s32 y1, irr::s32 x2, irr::s32 y2,
		irr::video::SColor colour, bool centred);

	irr::IrrlichtDevice* device_;
	Canvas& canvas_;
	menu::MenuFlow& flow_;
	gamecore::SoundEngine* sound_;
	const std::deque<std::string>& status_;
	GuiPanel panel_;
	menu::Screen built_;
	bool hasBuilt_;
	std::mt19937 random_;
};

#endif
