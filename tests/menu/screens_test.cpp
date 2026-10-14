#include "doctest.h"
#include "CharacterStore.h"
#include "IrrlichtCanvas.h"
#include "MenuFlow.h"
#include "MenuScene.h"
#include "MenuScreens.h"
#include <deque>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace irr;

namespace
{
	std::vector<character::Profession> professions()
	{
		std::ifstream in(std::string(ASSETS_DIR) + "/config/professions.csv", std::ios::binary);
		std::ostringstream text;
		text << in.rdbuf();
		std::vector<character::Profession> loaded;
		character::LoadError error;
		REQUIRE(character::loadProfessions(text.str(), loaded, error));
		return loaded;
	}

	struct Menu
	{
		Menu(const char* name)
			: device(createDevice(video::EDT_NULL, core::dimension2d<u32>(1280, 720))), canvas(device),
			  directory((std::filesystem::temp_directory_path() / name).string()), store(directory),
			  flow(store, professions(), 3), backdrop(new MenuScene(device)),
			  screens(new MenuScreens(device, canvas, flow, nullptr, status, backdrop))
		{
			std::filesystem::remove_all(directory);
		}
		~Menu()
		{
			delete screens;
			delete backdrop;
			device->drop();
			std::filesystem::remove_all(directory);
		}

		void mouse(EMOUSE_INPUT_EVENT kind, s32 x, s32 y)
		{
			SEvent event;
			event.EventType = EET_MOUSE_INPUT_EVENT;
			event.MouseInput.Event = kind;
			event.MouseInput.X = x;
			event.MouseInput.Y = y;
			event.MouseInput.Wheel = 0.0f;
			event.MouseInput.ButtonStates = 0;
			event.MouseInput.Shift = false;
			event.MouseInput.Control = false;
			screens->onEvent(event);
		}

		IrrlichtDevice* device;
		IrrlichtCanvas canvas;
		std::string directory;
		menu::CharacterStore store;
		menu::MenuFlow flow;
		std::deque<std::string> status;
		MenuScene* backdrop;
		MenuScreens* screens;
	};
}

TEST_CASE("the character stands in the scene from create or select onward, and never on the title or settings")
{
	Menu menu("screens_where");
	menu.screens->draw();
	CHECK(menu.backdrop->character() == nullptr);
	menu.flow.play();
	menu.screens->draw();
	CHECK(menu.backdrop->character() == nullptr);
	menu.flow.newCharacter();
	menu.screens->draw();
	CHECK(menu.backdrop->character() != nullptr);
	REQUIRE(menu.flow.create("Dana", character::Gender::Female, 0));
	menu.screens->draw();
	CHECK(menu.flow.screen() == menu::Screen::CharacterSelect);
	CHECK(menu.backdrop->character() != nullptr);
	menu.flow.highlight(-1);
	menu.screens->draw();
	CHECK(menu.backdrop->character() == nullptr);
	menu.flow.highlight(0);
	REQUIRE(menu.flow.select());
	menu.screens->draw();
	CHECK(menu.flow.screen() == menu::Screen::WorldSelect);
	CHECK(menu.backdrop->character() != nullptr);
	menu.flow.join();
	menu.screens->draw();
	CHECK(menu.backdrop->character() != nullptr);
	menu.flow.back();
	menu.flow.host();
	menu.screens->draw();
	CHECK(menu.flow.screen() == menu::Screen::Playing);
	CHECK(menu.backdrop->character() != nullptr);
	menu.flow.returnToTitle();
	menu.screens->draw();
	CHECK(menu.backdrop->character() == nullptr);
	menu.flow.settings();
	menu.screens->draw();
	CHECK(menu.backdrop->character() == nullptr);
}

TEST_CASE("a click on a widget never starts turning the character, even on the scene side")
{
	Menu menu("screens_widget");
	menu.flow.play();
	menu.flow.newCharacter();
	REQUIRE(menu.flow.create("Dana", character::Gender::Female, 0));
	menu.flow.highlight(0);
	menu.screens->draw();
	REQUIRE(menu.backdrop->character() != nullptr);
	menu.mouse(EMIE_LMOUSE_PRESSED_DOWN, 560, 300);
	menu.mouse(EMIE_MOUSE_MOVED, 500, 300);
	CHECK(menu.backdrop->characterTargetYaw() == doctest::Approx(0.0f));
	menu.mouse(EMIE_LMOUSE_LEFT_UP, 500, 300);
	menu.mouse(EMIE_LMOUSE_PRESSED_DOWN, 900, 300);
	menu.mouse(EMIE_MOUSE_MOVED, 860, 300);
	CHECK(menu.backdrop->characterTargetYaw() == doctest::Approx(20.0f));
}

TEST_CASE("dragging left on the character turns them to their right, and only drags on the scene side count")
{
	Menu menu("screens_drag");
	menu.flow.play();
	menu.flow.newCharacter();
	menu.screens->draw();
	REQUIRE(menu.backdrop->character() != nullptr);

	menu.mouse(EMIE_LMOUSE_PRESSED_DOWN, 900, 400);
	menu.mouse(EMIE_MOUSE_MOVED, 880, 400);
	menu.mouse(EMIE_MOUSE_MOVED, 840, 400);
	CHECK(menu.backdrop->characterTargetYaw() == doctest::Approx(30.0f));
	menu.mouse(EMIE_LMOUSE_LEFT_UP, 840, 400);
	menu.mouse(EMIE_MOUSE_MOVED, 700, 400);
	CHECK(menu.backdrop->characterTargetYaw() == doctest::Approx(30.0f));

	menu.mouse(EMIE_LMOUSE_PRESSED_DOWN, 300, 400);
	menu.mouse(EMIE_MOUSE_MOVED, 200, 400);
	CHECK(menu.backdrop->characterTargetYaw() == doctest::Approx(30.0f));
}
