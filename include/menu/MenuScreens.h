#ifndef MENUSCREENS_H
#define MENUSCREENS_H

#include <deque>
#include <random>
#include <string>
#include <vector>
#include <irrlicht.h>
#include "Canvas.h"
#include "GuiPanel.h"
#include "InputRouter.h"
#include "MenuFlow.h"

namespace gamecore { class SoundEngine; }
class MenuScene;
class World;

class MenuScreens : public InputLayer
{
public:
	enum { GUI_ID_PLAY = 200, GUI_ID_SETTINGS, GUI_ID_QUIT, GUI_ID_BACK, GUI_ID_CHARACTERS, GUI_ID_NEW_CHARACTER,
		GUI_ID_SELECT, GUI_ID_NAME, GUI_ID_MALE, GUI_ID_FEMALE, GUI_ID_PROFESSION, GUI_ID_CREATE, GUI_ID_HOST, GUI_ID_JOIN,
		GUI_ID_ADDRESS, GUI_ID_PORT, GUI_ID_START, GUI_ID_LEAVE, GUI_ID_REROLL = 300, GUI_ID_MINUS_FIRST = 310,
		GUI_ID_PLUS_FIRST = 320, GUI_ID_OUTFIT_PREVIOUS = 330, GUI_ID_OUTFIT_NEXT,
		GUI_ID_WORLD_NAME = 340, GUI_ID_MAX_PLAYERS, GUI_ID_TIME_FIRST = 350, GUI_ID_DELETE = 360 };

	MenuScreens(irr::IrrlichtDevice* device, Canvas& canvas, menu::MenuFlow& flow, gamecore::SoundEngine* sound,
		const std::deque<std::string>& status, MenuScene* backdrop);

	bool onGuiEvent(const irr::SEvent& event);
	bool onEvent(const irr::SEvent& event) override;
	void back();
	void draw();
	void hide();
	void setWorld(World* world) { world_ = world; }

private:
	enum class Style { Primary, Secondary, Link, Choice, Card, Step };

	struct Button
	{
		irr::s32 id;
		irr::core::rect<irr::s32> area;
		std::string label;
		std::string detail;
		Style style;
	};

	void build();
	void header(const std::string& title);
	void addButton(irr::s32 id, const irr::core::rect<irr::s32>& area, const std::string& label, Style style,
		const std::string& detail = std::string());
	irr::gui::IGUIEditBox* addTextBox(irr::s32 id, const irr::core::rect<irr::s32>& area, const std::string& text,
		irr::u32 maxLength, bool focus);
	irr::core::rect<irr::s32> place(const irr::core::rect<irr::s32>& area) const;
	bool isChosen(irr::s32 id) const;
	void markCharacters();
	bool clicked(irr::s32 id);
	void click();
	void showCharacter();
	void drawNameplate();
	bool onScene(irr::core::position2d<irr::s32> point) const;
	void drawAegis();
	bool isEnabled(irr::s32 id) const;

	void drawButton(const Button& button);
	void drawIcon(irr::s32 id, irr::core::position2d<irr::s32> at, irr::video::SColor colour);
	void text(FontHandle font, const std::string& text, irr::s32 x, irr::s32 y, irr::video::SColor colour);
	void label(const std::string& text, irr::s32 x, irr::s32 y);

	irr::IrrlichtDevice* device_;
	Canvas& canvas_;
	menu::MenuFlow& flow_;
	gamecore::SoundEngine* sound_;
	const std::deque<std::string>& status_;
	GuiPanel panel_;
	std::vector<Button> buttons_;
	std::string title_;
	menu::Screen built_;
	bool hasBuilt_;
	character::Gender gender_;
	int outfit_;
	menu::TimeMode timeMode_;
	MenuScene* backdrop_;
	World* world_;
	bool turning_;
	irr::s32 turnFromX_;
};

#endif
