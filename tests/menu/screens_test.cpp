#include "doctest.h"
#include "IconArt.h"
#include "CharacterStore.h"
#include "IrrlichtCanvas.h"
#include "MenuFlow.h"
#include "MenuScene.h"
#include "MenuScreens.h"
#include "Theme.h"
#include "Outfits.h"
#include "World.h"
#include "Hud.h"
#include "RecordingCanvas.h"
#include <deque>
#include <functional>
#include <set>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace irr;
using namespace gui;

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
			forward.screens = &screens;
			device->setEventReceiver(&forward);
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

		void press(s32 id)
		{
			SEvent event;
			event.EventType = EET_GUI_EVENT;
			event.GUIEvent.EventType = gui::EGET_BUTTON_CLICKED;
			event.GUIEvent.Caller = device->getGUIEnvironment()->getRootGUIElement()->getElementFromId(id, true);
			event.GUIEvent.Element = nullptr;
			REQUIRE(event.GUIEvent.Caller != nullptr);
			screens->onGuiEvent(event);
		}

		void key(EKEY_CODE code, bool shift = false, wchar_t typed = 0)
		{
			for (bool down : { true, false })
			{
				SEvent event;
				event.EventType = EET_KEY_INPUT_EVENT;
				event.KeyInput.Key = code;
				event.KeyInput.PressedDown = down;
				event.KeyInput.Shift = shift;
				event.KeyInput.Control = false;
				event.KeyInput.Char = typed;
				if (!screens->onEvent(event))
					device->getGUIEnvironment()->postEventFromUser(event);
			}
		}

		void type(const std::string& text)
		{
			for (char letter : text)
				key(KEY_KEY_A, false, (wchar_t)letter);
		}

		s32 focused() const
		{
			IGUIElement* element = device->getGUIEnvironment()->getFocus();
			return element ? element->getID() : -1;
		}

		bool tabTo(s32 id)
		{
			for (int i = 0; i < 80 && focused() != id; ++i)
				key(KEY_TAB);
			return focused() == id;
		}

		struct Forward : IEventReceiver
		{
			MenuScreens** screens;
			bool OnEvent(const SEvent& event) override { return event.EventType == EET_GUI_EVENT && (*screens)->onGuiEvent(event); }
		};

		IrrlichtDevice* device;
		IrrlichtCanvas canvas;
		std::string directory;
		menu::CharacterStore store;
		menu::MenuFlow flow;
		std::deque<std::string> status;
		MenuScene* backdrop;
		MenuScreens* screens;
		Forward forward;
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
	menu.flow.startServer();
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

TEST_CASE("host game opens world settings: a world name, three time modes, a player limit and Start Server, which hosts with them")
{
	Menu menu("screens_world_settings");
	menu.flow.play();
	menu.flow.newCharacter();
	REQUIRE(menu.flow.create("Dana", character::Gender::Female, 0));
	menu.flow.highlight(0);
	REQUIRE(menu.flow.select());
	menu.screens->draw();
	IGUIElement* root = menu.device->getGUIEnvironment()->getRootGUIElement();
	CHECK(root->getElementFromId(MenuScreens::GUI_ID_START, true) == nullptr);
	CHECK(root->getElementFromId(MenuScreens::GUI_ID_WORLD_NAME, true) == nullptr);

	menu.press(MenuScreens::GUI_ID_HOST);
	CHECK(menu.flow.screen() == menu::Screen::WorldSettings);
	menu.screens->draw();
	IGUIElement* name = root->getElementFromId(MenuScreens::GUI_ID_WORLD_NAME, true);
	IGUIElement* players = root->getElementFromId(MenuScreens::GUI_ID_MAX_PLAYERS, true);
	REQUIRE(name != nullptr);
	REQUIRE(players != nullptr);
	CHECK(std::wstring(name->getText()) == L"New World");
	CHECK(std::wstring(players->getText()) == L"8");
	IGUIElement* start = root->getElementFromId(MenuScreens::GUI_ID_START, true);
	REQUIRE(start != nullptr);
	for (int i = 0; i < 3; ++i)
	{
		IGUIElement* mode = root->getElementFromId(MenuScreens::GUI_ID_TIME_FIRST + i, true);
		REQUIRE(mode != nullptr);
		CHECK(mode->getAbsolutePosition().UpperLeftCorner.Y > name->getAbsolutePosition().LowerRightCorner.Y);
		CHECK(mode->getAbsolutePosition().LowerRightCorner.Y < players->getAbsolutePosition().UpperLeftCorner.Y);
	}

	menu.press(MenuScreens::GUI_ID_TIME_FIRST + 2);
	name->setText(L"Ashes");
	players->setText(L"3");
	menu.press(MenuScreens::GUI_ID_START);
	CHECK(menu.flow.screen() == menu::Screen::Playing);
	CHECK(menu.flow.worldName() == "Ashes");
	CHECK(menu.flow.timeMode() == menu::TimeMode::Fast);
	CHECK(menu.flow.maxPlayers() == 3);
}

TEST_CASE("the host and join cards switch between world settings and the address form, and back returns to the characters")
{
	Menu menu("screens_world_cards");
	menu.flow.play();
	menu.flow.newCharacter();
	REQUIRE(menu.flow.create("Dana", character::Gender::Female, 0));
	menu.flow.highlight(0);
	REQUIRE(menu.flow.select());
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_HOST);
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_TIME_FIRST + 1);
	menu.press(MenuScreens::GUI_ID_JOIN);
	CHECK(menu.flow.screen() == menu::Screen::JoinGame);
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_HOST);
	CHECK(menu.flow.screen() == menu::Screen::WorldSettings);
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_START);
	CHECK(menu.flow.timeMode() == menu::TimeMode::RealTime);
	menu.flow.returnToTitle();
	menu.flow.play();
	menu.flow.highlight(0);
	REQUIRE(menu.flow.select());
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_HOST);
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_BACK);
	CHECK(menu.flow.screen() == menu::Screen::CharacterSelect);
}

TEST_CASE("the delete button under the character list removes the selected character on its second press")
{
	Menu menu("screens_delete");
	menu.flow.play();
	for (const char* name : { "Ada", "Bo" })
	{
		menu.flow.newCharacter();
		REQUIRE(menu.flow.create(name, character::Gender::Male, 0));
	}
	menu.screens->draw();
	IGUIElement* root = menu.device->getGUIEnvironment()->getRootGUIElement();
	IGUIListBox* list = static_cast<IGUIListBox*>(root->getElementFromId(MenuScreens::GUI_ID_CHARACTERS, true));
	IGUIElement* remove = root->getElementFromId(MenuScreens::GUI_ID_DELETE, true);
	REQUIRE(list != nullptr);
	REQUIRE(remove != nullptr);
	CHECK(remove->getAbsolutePosition().UpperLeftCorner.Y >= list->getAbsolutePosition().LowerRightCorner.Y);
	CHECK(remove->getAbsolutePosition().LowerRightCorner.Y <= root->getElementFromId(MenuScreens::GUI_ID_SELECT, true)->getAbsolutePosition().UpperLeftCorner.Y);
	REQUIRE(list->getItemCount() == 2);
	list->setSelected(0);
	menu.press(MenuScreens::GUI_ID_DELETE);
	CHECK(std::filesystem::exists(menu.store.pathFor("Ada")));
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_DELETE);
	CHECK_FALSE(std::filesystem::exists(menu.store.pathFor("Ada")));
	menu.screens->draw();
	list = static_cast<IGUIListBox*>(root->getElementFromId(MenuScreens::GUI_ID_CHARACTERS, true));
	REQUIRE(list != nullptr);
	CHECK(list->getItemCount() == 1);
	CHECK(menu.flow.screen() == menu::Screen::CharacterSelect);
}

TEST_CASE("Create Character opens the skills step: ten skills with minus and plus, points left, Back keeps what was chosen, and Confirm saves the totals")
{
	Menu menu("screens_skills");
	menu.flow.play();
	menu.flow.newCharacter();
	menu.screens->draw();
	IGUIElement* root = menu.device->getGUIEnvironment()->getRootGUIElement();
	root->getElementFromId(MenuScreens::GUI_ID_NAME, true)->setText(L"Ann");
	menu.press(MenuScreens::GUI_ID_FEMALE);
	menu.press(MenuScreens::GUI_ID_CREATE);
	CHECK(menu.flow.screen() == menu::Screen::SkillAllocation);
	CHECK_FALSE(std::filesystem::exists(menu.store.pathFor("Ann")));
	menu.screens->draw();
	for (int i = 0; i < character::SKILL_COUNT; ++i)
	{
		INFO(i);
		IGUIElement* minus = root->getElementFromId(MenuScreens::GUI_ID_SKILL_MINUS_FIRST + i, true);
		IGUIElement* plus = root->getElementFromId(MenuScreens::GUI_ID_SKILL_PLUS_FIRST + i, true);
		REQUIRE(minus != nullptr);
		REQUIRE(plus != nullptr);
		CHECK(minus->getAbsolutePosition().UpperLeftCorner.Y == plus->getAbsolutePosition().UpperLeftCorner.Y);
		if (i > 0)
			CHECK(plus->getAbsolutePosition().UpperLeftCorner.Y >
				root->getElementFromId(MenuScreens::GUI_ID_SKILL_PLUS_FIRST + i - 1, true)->getAbsolutePosition().UpperLeftCorner.Y);
	}
	CHECK(root->getElementFromId(MenuScreens::GUI_ID_CONFIRM, true) != nullptr);
	CHECK(root->getElementFromId(MenuScreens::GUI_ID_BACK, true) != nullptr);
	CHECK(root->getElementFromId(MenuScreens::GUI_ID_NAME, true) == nullptr);
	int pool = menu.flow.skillPool();
	menu.press(MenuScreens::GUI_ID_SKILL_PLUS_FIRST + character::SPEECH);
	menu.press(MenuScreens::GUI_ID_SKILL_PLUS_FIRST + character::SPEECH);
	menu.press(MenuScreens::GUI_ID_SKILL_PLUS_FIRST + character::COOKING);
	menu.press(MenuScreens::GUI_ID_SKILL_MINUS_FIRST + character::COOKING);
	CHECK(menu.flow.skillsUnplaced() == pool - 2);
	CHECK(menu.flow.skillValue(character::SPEECH) == 2);

	menu.press(MenuScreens::GUI_ID_BACK);
	CHECK(menu.flow.screen() == menu::Screen::CharacterCreation);
	menu.screens->draw();
	CHECK(std::wstring(root->getElementFromId(MenuScreens::GUI_ID_NAME, true)->getText()) == L"Ann");
	CHECK(std::string(menu.backdrop->characterModel()).find("stick_dan.ms3d") != std::string::npos);
	menu.press(MenuScreens::GUI_ID_CREATE);
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_SKILL_PLUS_FIRST + character::SPEECH);
	menu.press(MenuScreens::GUI_ID_CONFIRM);
	CHECK(menu.flow.screen() == menu::Screen::CharacterSelect);
	REQUIRE(menu.flow.characters().size() == 1);
	CHECK(menu.flow.characters()[0].name() == "Ann");
	CHECK(menu.flow.characters()[0].gender() == character::Gender::Female);
	CHECK(menu.flow.characters()[0].getSpeech() == 1);
}

TEST_CASE("the skills step lists every skill with its value and shows the points left in the AEGIS style")
{
	IrrlichtDevice* device = createDevice(video::EDT_NULL, core::dimension2d<u32>(1280, 720));
	RecordingCanvas canvas;
	canvas.screenDimension = core::dimension2d<u32>(1280, 720);
	std::string directory = (std::filesystem::temp_directory_path() / "screens_skill_draw").string();
	std::filesystem::remove_all(directory);
	{
		menu::CharacterStore store(directory);
		menu::MenuFlow flow(store, professions(), 3);
		std::deque<std::string> status;
		MenuScreens screens(device, canvas, flow, nullptr, status, nullptr);
		flow.play();
		flow.newCharacter();
		int doctor = -1;
		for (size_t i = 0; i < flow.professions().size(); ++i)
			if (flow.professions()[i].name == "Doctor")
				doctor = (int)i;
		REQUIRE(flow.toSkills("Ann", character::Gender::Male, doctor));
		flow.adjustSkill(character::SPEECH, 1);
		screens.draw();
		auto says = [&canvas](const std::wstring& line) -> const RecordingCanvas::Call* {
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::TEXT && call.text == line)
					return &call;
			return nullptr;
		};
		for (int i = 0; i < character::SKILL_COUNT; ++i)
		{
			std::string skill = character::skillName(i);
			const RecordingCanvas::Call* name = says(std::wstring(skill.begin(), skill.end()));
			INFO(character::skillName(i));
			REQUIRE(name != nullptr);
			CHECK(name->colour == theme::muted);
		}
		CHECK(says(L"30") != nullptr);
		CHECK(says(L"1") != nullptr);
		const RecordingCanvas::Call* left = says(L"Points remaining: " + std::to_wstring(flow.skillPool() - 1));
		REQUIRE(left != nullptr);
		CHECK(left->colour == theme::accent);
		const RecordingCanvas::Call* why = says(L"Intelligence " + std::to_wstring(flow.draft().value(character::INTELLIGENCE)) + L" gives " +
			std::to_wstring(flow.skillPool()) + L" points");
		CHECK(why != nullptr);
	}
	std::filesystem::remove_all(directory);
	device->drop();
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

namespace
{
	bool endsWith(const std::string& text, const std::string& tail)
	{
		return text.size() >= tail.size() && text.compare(text.size() - tail.size(), tail.size(), tail) == 0;
	}
}

TEST_CASE("choosing Female on the create screen stands stick Dan in the street, and Male brings stick Mike back, keeping the turn")
{
	Menu menu("screens_gender");
	menu.flow.play();
	menu.flow.newCharacter();
	menu.screens->draw();
	REQUIRE(menu.backdrop->character() != nullptr);
	CHECK(endsWith(menu.backdrop->characterModel(), "stick_mike.ms3d"));
	menu.mouse(EMIE_LMOUSE_PRESSED_DOWN, 900, 400);
	menu.mouse(EMIE_MOUSE_MOVED, 840, 400);
	menu.mouse(EMIE_LMOUSE_LEFT_UP, 840, 400);
	REQUIRE(menu.backdrop->characterTargetYaw() == doctest::Approx(30.0f));

	menu.press(MenuScreens::GUI_ID_FEMALE);
	menu.screens->draw();
	REQUIRE(menu.backdrop->character() != nullptr);
	CHECK(endsWith(menu.backdrop->characterModel(), "stick_dan.ms3d"));
	CHECK(menu.backdrop->character()->getMaterial(0).getTexture(0) != nullptr);
	CHECK(menu.backdrop->characterTargetYaw() == doctest::Approx(30.0f));
	CHECK(menu.backdrop->scene()->getSceneNodeFromName("character") == menu.backdrop->character());

	menu.press(MenuScreens::GUI_ID_MALE);
	menu.screens->draw();
	CHECK(endsWith(menu.backdrop->characterModel(), "stick_mike.ms3d"));
	int characters = 0;
	for (scene::ISceneNode* node : menu.backdrop->scene()->getRootSceneNode()->getChildren())
		if (std::string(node->getName()) == "character")
			++characters;
	CHECK(characters == 1);
}

TEST_CASE("select and the game screens stand each saved character as their own gender")
{
	Menu menu("screens_saved_gender");
	menu.flow.play();
	menu.flow.newCharacter();
	REQUIRE(menu.flow.create("Dana", character::Gender::Female, 0));
	menu.flow.newCharacter();
	REQUIRE(menu.flow.create("Eli", character::Gender::Male, 0));
	int dana = menu.flow.characters()[0].name() == "Dana" ? 0 : 1;
	menu.flow.highlight(dana);
	menu.screens->draw();
	CHECK(endsWith(menu.backdrop->characterModel(), "stick_dan.ms3d"));
	menu.flow.highlight(1 - dana);
	menu.screens->draw();
	CHECK(endsWith(menu.backdrop->characterModel(), "stick_mike.ms3d"));
	menu.flow.highlight(dana);
	REQUIRE(menu.flow.select());
	menu.flow.host();
	menu.flow.startServer();
	menu.screens->draw();
	CHECK(endsWith(menu.backdrop->characterModel(), "stick_dan.ms3d"));
}

namespace
{
	std::string skinOf(MenuScene* backdrop)
	{
		REQUIRE(backdrop->character() != nullptr);
		video::ITexture* texture = backdrop->character()->getMaterial(0).getTexture(0);
		REQUIRE(texture != nullptr);
		return texture->getName().getPath().c_str();
	}
}

TEST_CASE("the outfit arrows swap the model's clothes in place, wrapping both ways, and female starts on her own outfit")
{
	Menu menu("screens_outfits");
	menu.flow.play();
	menu.flow.newCharacter();
	menu.screens->draw();
	scene::ISceneNode* model = menu.backdrop->character();
	CHECK(endsWith(skinOf(menu.backdrop), "playerskin_m1.jpg"));
	menu.press(MenuScreens::GUI_ID_OUTFIT_NEXT);
	menu.screens->draw();
	CHECK(endsWith(skinOf(menu.backdrop), "playerskin_m2.jpg"));
	CHECK(menu.backdrop->character() == model);
	menu.press(MenuScreens::GUI_ID_OUTFIT_NEXT);
	menu.screens->draw();
	CHECK(endsWith(skinOf(menu.backdrop), "playerskin_m3.jpg"));
	menu.press(MenuScreens::GUI_ID_OUTFIT_NEXT);
	menu.screens->draw();
	CHECK(endsWith(skinOf(menu.backdrop), "playerskin_m1.jpg"));
	menu.press(MenuScreens::GUI_ID_OUTFIT_PREVIOUS);
	menu.screens->draw();
	CHECK(endsWith(skinOf(menu.backdrop), "playerskin_m3.jpg"));

	menu.press(MenuScreens::GUI_ID_FEMALE);
	menu.screens->draw();
	CHECK(endsWith(skinOf(menu.backdrop), "playerskin_d1.bmp"));
	menu.press(MenuScreens::GUI_ID_OUTFIT_NEXT);
	menu.screens->draw();
	CHECK(endsWith(skinOf(menu.backdrop), "playerskin_d1.bmp"));
	for (const std::string& skin : menu::outfitSkins(character::Gender::Male))
		CHECK((endsWith(skin, "m1.jpg") || endsWith(skin, "m2.jpg") || endsWith(skin, "m3.jpg")));
	CHECK(menu::outfitSkins(character::Gender::Male).size() == 3);
	CHECK(menu::outfitSkins(character::Gender::Female).size() == 1);
}

TEST_CASE("the outfit picked on create is saved and worn on select and in the game")
{
	Menu menu("screens_outfit_saved");
	menu.flow.play();
	menu.flow.newCharacter();
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_OUTFIT_PREVIOUS);
	menu.screens->draw();
	IGUIElement* name = menu.device->getGUIEnvironment()->getRootGUIElement()->getElementFromId(MenuScreens::GUI_ID_NAME, true);
	REQUIRE(name != nullptr);
	name->setText(L"Ivy");
	menu.press(MenuScreens::GUI_ID_CREATE);
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_CONFIRM);
	REQUIRE(menu.flow.screen() == menu::Screen::CharacterSelect);
	REQUIRE(menu.flow.characters().size() == 1);
	CHECK(menu.flow.characters()[0].outfit() == 2);
	menu.screens->draw();
	menu.flow.newCharacter();
	menu.screens->draw();
	CHECK(endsWith(skinOf(menu.backdrop), "playerskin_m1.jpg"));
	menu.flow.back();
	menu.flow.highlight(0);
	menu.screens->draw();
	CHECK(endsWith(skinOf(menu.backdrop), "playerskin_m3.jpg"));
	REQUIRE(menu.flow.select());
	menu.flow.host();
	menu.flow.startServer();
	menu.screens->draw();
	CHECK(endsWith(skinOf(menu.backdrop), "playerskin_m3.jpg"));
}

TEST_CASE("every profession row on the create screen shows its full bonus text, wrapped to fit inside its list")
{
	Menu menu("screens_professions");
	menu.flow.play();
	menu.flow.newCharacter();
	menu.screens->draw();
	IGUIElement* element = menu.device->getGUIEnvironment()->getRootGUIElement()->getElementFromId(MenuScreens::GUI_ID_PROFESSION, true);
	REQUIRE(element != nullptr);
	IGUIListBox* list = static_cast<IGUIListBox*>(element);
	CHECK(list->getAbsolutePosition() == core::rect<s32>(64, 244, 448, 418));
	IGUISkin* skin = menu.device->getGUIEnvironment()->getSkin();
	IGUIFont* font = skin->getFont();
	REQUIRE(font != nullptr);
	s32 room = list->getAbsolutePosition().getWidth() - skin->getSize(EGDS_SCROLLBAR_SIZE) - 2 * skin->getSize(EGDS_TEXT_DISTANCE_X);
	REQUIRE(list->getItemCount() == menu.flow.professions().size());
	REQUIRE(list->getItemCount() == 28);
	core::rect<s32> box = list->getAbsolutePosition();
	s32 rowHeight = 0;
	while (rowHeight < box.getHeight() && list->getItemAt(box.UpperLeftCorner.X + 4, box.UpperLeftCorner.Y + rowHeight) != 1)
		++rowHeight;
	int wrapped = 0;
	for (u32 i = 0; i < list->getItemCount(); ++i)
	{
		std::wstring item = list->getListItem(i);
		INFO(core::stringc(item.c_str()).c_str());
		CHECK((s32)font->getDimension(item.c_str()).Width <= room);
		CHECK((s32)font->getDimension(item.c_str()).Height <= rowHeight);
		CHECK(item.find(L"...") == std::wstring::npos);
		std::wstring flat;
		for (wchar_t c : item)
			if (c != L'\n' && c != L' ')
				flat += c;
		const character::Profession& profession = menu.flow.professions()[i];
		CHECK(flat.find(std::wstring(profession.name.begin(), profession.name.end()).substr(0, 4)) == 0);
		for (const std::pair<int, int>& bonus : profession.skillBonuses)
		{
			std::string expected = std::string(character::skillName(bonus.first)) + "+" + std::to_string(bonus.second);
			CHECK(flat.find(std::wstring(expected.begin(), expected.end())) != std::wstring::npos);
		}
		if (item.find(L'\n') != std::wstring::npos)
			++wrapped;
	}
	CHECK(wrapped >= 1);
}

TEST_CASE("every profession fits on the select screen beside the longest possible name")
{
	Menu menu("screens_select_professions");
	menu.flow.play();
	for (size_t i = 0; i < menu.flow.professions().size(); ++i)
	{
		menu.flow.newCharacter();
		std::string name = std::string(menu::MAX_NAME_LENGTH - 2, 'W') + (char)('A' + i / 26) + (char)('A' + i % 26);
		REQUIRE(menu.flow.create(name, character::Gender::Male, (int)i));
	}
	menu.flow.play();
	menu.screens->draw();
	IGUIElement* element = menu.device->getGUIEnvironment()->getRootGUIElement()->getElementFromId(MenuScreens::GUI_ID_CHARACTERS, true);
	REQUIRE(element != nullptr);
	IGUIListBox* list = static_cast<IGUIListBox*>(element);
	IGUISkin* skin = menu.device->getGUIEnvironment()->getSkin();
	IGUIFont* font = skin->getFont();
	REQUIRE(font != nullptr);
	s32 room = list->getAbsolutePosition().getWidth() - skin->getSize(EGDS_SCROLLBAR_SIZE) - 2 * skin->getSize(EGDS_TEXT_DISTANCE_X);
	REQUIRE(list->getItemCount() == menu.flow.professions().size());
	core::rect<s32> box = list->getAbsolutePosition();
	CHECK(box == core::rect<s32>(64, 104, 608, 584));
	s32 rowHeight = 0;
	while (rowHeight < box.getHeight() && list->getItemAt(box.UpperLeftCorner.X + 4, box.UpperLeftCorner.Y + rowHeight) != 1)
		++rowHeight;
	int wrapped = 0;
	for (u32 i = 0; i < list->getItemCount(); ++i)
	{
		std::wstring item = list->getListItem(i);
		INFO(core::stringc(item.c_str()).c_str());
		CHECK((s32)font->getDimension(item.c_str()).Width <= room);
		CHECK((s32)font->getDimension(item.c_str()).Height <= rowHeight);
		if (item.find(L'\n') != std::wstring::npos)
			++wrapped;
		const std::string& profession = menu.flow.characters()[i].profession().name;
		CHECK(item.find(std::wstring(profession.begin(), profession.end())) != std::wstring::npos);
	}
	CHECK(wrapped >= 1);
	CHECK(wrapped < (int)list->getItemCount());
}

TEST_CASE("switching to female after picking a male outfit saves her on her own first outfit")
{
	Menu menu("screens_outfit_female");
	menu.flow.play();
	menu.flow.newCharacter();
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_OUTFIT_NEXT);
	menu.press(MenuScreens::GUI_ID_OUTFIT_NEXT);
	menu.press(MenuScreens::GUI_ID_FEMALE);
	menu.screens->draw();
	IGUIElement* name = menu.device->getGUIEnvironment()->getRootGUIElement()->getElementFromId(MenuScreens::GUI_ID_NAME, true);
	REQUIRE(name != nullptr);
	name->setText(L"Ada");
	menu.press(MenuScreens::GUI_ID_CREATE);
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_CONFIRM);
	REQUIRE(menu.flow.characters().size() == 1);
	CHECK(menu.flow.characters()[0].gender() == character::Gender::Female);
	CHECK(menu.flow.characters()[0].outfit() == 0);
}

TEST_CASE("once a world is attached the game screen hands the scene over to it, and the menus keep the backdrop")
{
	Menu menu("screens_world");
	menu.flow.play();
	menu.flow.newCharacter();
	REQUIRE(menu.flow.create("Dana", character::Gender::Female, 0));
	menu.flow.highlight(0);
	REQUIRE(menu.flow.select());
	menu.flow.host();
	menu.flow.startServer();
	menu.screens->draw();
	REQUIRE(menu.backdrop->character() != nullptr);
	World world(menu.device, menu::characterModel(character::Gender::Female), menu::outfitSkin(character::Gender::Female, 0).c_str(), [] {});
	menu.screens->setWorld(&world);
	menu.screens->draw();
	CHECK(menu.backdrop->character() == nullptr);
	CHECK(world.player() != nullptr);
	menu.flow.returnToTitle();
	menu.screens->setWorld(nullptr);
	menu.screens->draw();
	CHECK(menu.flow.screen() == menu::Screen::Title);
	CHECK(menu.backdrop->character() == nullptr);
	CHECK(std::string(menu::characterModel(character::Gender::Male)) == "assets/models/stick_mike.ms3d");
	CHECK(std::string(menu::characterModel(character::Gender::Female)) == "assets/models/stick_dan.ms3d");
}

TEST_CASE("the vitals HUD shows only in the world, replacing the big name heading there, and never on the menus")
{
	IrrlichtDevice* device = createDevice(video::EDT_NULL, core::dimension2d<u32>(1280, 720));
	RecordingCanvas canvas;
	canvas.screenDimension = core::dimension2d<u32>(1280, 720);
	std::string directory = (std::filesystem::temp_directory_path() / "screens_hud").string();
	std::filesystem::remove_all(directory);
	{
		menu::CharacterStore store(directory);
		menu::MenuFlow flow(store, professions(), 3);
		std::deque<std::string> status;
		MenuScene backdrop(device);
		MenuScreens screens(device, canvas, flow, nullptr, status, &backdrop);
		auto hud = [&canvas] {
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::IMAGE && canvas.pathOf(call.texture) == "assets/hud/fill_hp.png")
					return true;
			return false;
		};
		auto heading = [&canvas](const std::wstring& text) {
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::TEXT && call.text == text && canvas.pathOf(call.font).find("heading") != std::string::npos)
					return true;
			return false;
		};
		screens.draw();
		CHECK_FALSE(hud());
		flow.play();
		flow.newCharacter();
		REQUIRE(flow.create("Rook", character::Gender::Male, 0));
		flow.highlight(0);
		REQUIRE(flow.select());
		canvas.calls.clear();
		screens.draw();
		CHECK_FALSE(hud());
		flow.host();
		flow.startServer();
		canvas.calls.clear();
		screens.draw();
		CHECK_FALSE(hud());
		CHECK(heading(L"Rook"));
		World world(device, menu::characterModel(character::Gender::Male), menu::outfitSkin(character::Gender::Male, 0).c_str(), [] {});
		screens.setWorld(&world);
		canvas.calls.clear();
		screens.draw();
		CHECK(hud());
		CHECK_FALSE(heading(L"Rook"));
		world.update(0.0f);
		REQUIRE_FALSE(world.detector().hasTarget());
		REQUIRE(world.detector().getInRange().empty());
		canvas.calls.clear();
		screens.draw();
		int idleLines = 0;
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::LINE && call.from.X < 300 && call.from.Y > (s32)canvas.screenDimension.Height - 250 && call.colour.getAlpha() > 0)
				++idleLines;
		CHECK(idleLines == 12 + 32 + 8); // the plus frame's outline, the centre circle and the four arrows
		world.teleport(-8.0f, 0.0f, -1.5f);
		world.update(0.0f);
		REQUIRE(world.detector().hasTarget());
		canvas.calls.clear();
		screens.draw();
		const RecordingCanvas::Call* marker = nullptr;
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::IMAGE && canvas.pathOf(call.texture) == "assets/ui/icons/rings/gold.png" && call.area.getCenter().X > 300)
				marker = &call;
		REQUIRE(marker != nullptr);
		f32 seen = marker->area.getWidth() * artBox("rings/gold.png").fill;
		CHECK(seen >= 48.0f);
		CHECK(seen <= 64.0f);
		int rim = 0;
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::LINE && marker->area.isPointInside(call.from) && call.colour.getRed() == 255 && call.colour.getBlue() < 100)
				++rim;
		CHECK(rim >= 3 * 40);
		world.teleport(0.0f, 0.0f, -20.0f);
		world.update(0.0f);
		auto drawn = [&canvas](const std::string& file) {
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::IMAGE && canvas.pathOf(call.texture) == file)
					return true;
			return false;
		};
		CHECK_FALSE(drawn("assets/hud/hud_weapon_icons.png"));
		world.arm(HudWeapon{ HUD_PISTOL, "", 9 });
		canvas.calls.clear();
		screens.draw();
		CHECK(drawn("assets/hud/hud_weapon_icons.png"));
		bool nine = false;
		for (const RecordingCanvas::Call& call : canvas.calls)
			nine = nine || (call.kind == RecordingCanvas::TEXT && call.text == L"9");
		CHECK(nine);
		flow.returnToTitle();
		screens.setWorld(nullptr);
		canvas.calls.clear();
		screens.draw();
		CHECK_FALSE(hud());
	}
	std::filesystem::remove_all(directory);
	device->drop();
}

TEST_CASE("the HUD follows the world's draining character, and death puts YOU DIED in big letters in the middle of the screen")
{
	IrrlichtDevice* device = createDevice(video::EDT_NULL, core::dimension2d<u32>(1280, 720));
	RecordingCanvas canvas;
	canvas.screenDimension = core::dimension2d<u32>(1280, 720);
	std::string directory = (std::filesystem::temp_directory_path() / "screens_death").string();
	std::filesystem::remove_all(directory);
	{
		menu::CharacterStore store(directory);
		menu::MenuFlow flow(store, professions(), 3);
		std::deque<std::string> status;
		MenuScene backdrop(device);
		MenuScreens screens(device, canvas, flow, nullptr, status, &backdrop);
		flow.play();
		flow.newCharacter();
		REQUIRE(flow.create("Rook", character::Gender::Male, 0));
		flow.highlight(0);
		REQUIRE(flow.select());
		flow.host();
		flow.startServer();
		std::string text = flow.chosen()->toText();
		for (const char* key : { "water", "hunger" })
		{
			std::string::size_type at = text.find(std::string("\n") + key + " = ");
			REQUIRE(at != std::string::npos);
			std::string::size_type end = text.find('\n', at + 1);
			text.replace(at + 1, end - at - 1, std::string(key) + " = 0");
		}
		std::string::size_type at = text.find("\nhealth = ");
		std::string::size_type end = text.find('\n', at + 1);
		text.replace(at + 1, end - at - 1, "health = 3");
		std::string problem;
		character::CharacterSheet dying = character::CharacterSheet::fromText(text, problem);
		REQUIRE(problem.empty());
		World world(device, menu::characterModel(character::Gender::Male), menu::outfitSkin(character::Gender::Male, 0).c_str(), [] {}, dying);
		int endurance = dying.getEndurance();
		world.setTimeCompression((float)(2.5 / (character::waterDrainPerSecond(endurance) + character::hungerDrainPerSecond(endurance))));
		screens.setWorld(&world);
		auto says = [&canvas](const std::wstring& line) -> const RecordingCanvas::Call* {
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::TEXT && call.text == line)
					return &call;
			return nullptr;
		};
		canvas.calls.clear();
		screens.draw();
		CHECK(says(L"3") != nullptr);
		CHECK(says(L"YOU DIED") == nullptr);
		world.update(1.0f);
		canvas.calls.clear();
		screens.draw();
		CHECK(says(L"1") != nullptr);
		CHECK(flow.chosen()->health() == 100);
		world.update(1.0f);
		canvas.calls.clear();
		screens.draw();
		CHECK(says(L"0") != nullptr);
		const RecordingCanvas::Call* died = says(L"YOU DIED");
		REQUIRE(died != nullptr);
		CHECK(canvas.pathOf(died->font).find("title") != std::string::npos);
		CHECK(died->hcenter);
		CHECK((died->area.UpperLeftCorner.X + died->area.LowerRightCorner.X) / 2 == 640);
		CHECK((died->area.UpperLeftCorner.Y + died->area.LowerRightCorner.Y) / 2 == 360);
	}
	std::filesystem::remove_all(directory);
	device->drop();
}

TEST_CASE("with F3 on, Mike's debug panel is drawn over the game with the frame rate and the player's position, and it goes when F3 is pressed again")
{
	IrrlichtDevice* device = createDevice(video::EDT_NULL, core::dimension2d<u32>(1280, 720));
	RecordingCanvas canvas;
	canvas.screenDimension = core::dimension2d<u32>(1280, 720);
	std::string directory = (std::filesystem::temp_directory_path() / "screens_debug").string();
	std::filesystem::remove_all(directory);
	{
		menu::CharacterStore store(directory);
		menu::MenuFlow flow(store, professions(), 3);
		std::deque<std::string> status;
		MenuScene backdrop(device);
		MenuScreens screens(device, canvas, flow, nullptr, status, &backdrop);
		flow.play();
		flow.newCharacter();
		REQUIRE(flow.create("Rook", character::Gender::Male, 0));
		flow.highlight(0);
		REQUIRE(flow.select());
		flow.host();
		flow.startServer();
		World world(device, menu::characterModel(character::Gender::Male), menu::outfitSkin(character::Gender::Male, 0).c_str(), [] {});
		world.teleport(5.0f, 0.0f, 6.0f);
		screens.setWorld(&world);
		auto panel = [&canvas]() -> std::wstring {
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::TEXT && call.text.find(L"FPS:") != std::wstring::npos)
					return call.text;
			return L"";
		};
		canvas.calls.clear();
		screens.draw();
		CHECK(panel().empty());

		SEvent f3;
		f3.EventType = EET_KEY_INPUT_EVENT;
		f3.KeyInput.Key = KEY_F3;
		f3.KeyInput.PressedDown = true;
		f3.KeyInput.Char = 0;
		f3.KeyInput.Shift = false;
		f3.KeyInput.Control = false;
		world.onEvent(f3);
		canvas.calls.clear();
		screens.draw();
		std::wstring text = panel();
		REQUIRE_FALSE(text.empty());
		CHECK(text.find(L"---GameCorev0.1.1---") != std::wstring::npos);
		CHECK(text.find(L"X:5") != std::wstring::npos);
		CHECK(text.find(L"Z:6") != std::wstring::npos);
		CHECK(text.find(L"Prim Drawn:") != std::wstring::npos);

		world.onEvent(f3);
		canvas.calls.clear();
		screens.draw();
		CHECK(panel().empty());
		screens.setWorld(nullptr);
	}
	std::filesystem::remove_all(directory);
	device->drop();
}

TEST_CASE("standing beside a container shows Press E to search at the bottom middle in the HUD's small white text, and walking away hides it")
{
	IrrlichtDevice* device = createDevice(video::EDT_NULL, core::dimension2d<u32>(1280, 720));
	RecordingCanvas canvas;
	canvas.screenDimension = core::dimension2d<u32>(1280, 720);
	std::string directory = (std::filesystem::temp_directory_path() / "screens_search").string();
	std::filesystem::remove_all(directory);
	{
		menu::CharacterStore store(directory);
		menu::MenuFlow flow(store, professions(), 3);
		std::deque<std::string> status;
		MenuScreens screens(device, canvas, flow, nullptr, status, nullptr);
		flow.play();
		flow.newCharacter();
		REQUIRE(flow.create("Rook", character::Gender::Male, 0));
		flow.highlight(0);
		REQUIRE(flow.select());
		flow.host();
		flow.startServer();
		World world(device, menu::characterModel(character::Gender::Male), menu::outfitSkin(character::Gender::Male, 0).c_str(), [] {});
		screens.setWorld(&world);
		auto prompt = [&canvas]() -> const RecordingCanvas::Call* {
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::TEXT && call.text == L"Press E to search")
					return &call;
			return nullptr;
		};
		canvas.calls.clear();
		screens.draw();
		CHECK(prompt() == nullptr);
		const loot::Container& chest = world.containers()[1];
		world.teleport(chest.x + 1.5f, 0.0f, chest.z);
		canvas.calls.clear();
		screens.draw();
		const RecordingCanvas::Call* shown = prompt();
		REQUIRE(shown != nullptr);
		CHECK(shown->hcenter);
		CHECK((shown->area.UpperLeftCorner.X + shown->area.LowerRightCorner.X) / 2 == 640);
		CHECK(shown->area.UpperLeftCorner.Y > 520);
		CHECK(shown->area.LowerRightCorner.Y <= 720);
		CHECK(shown->colour == video::SColor(255, 255, 255, 255));
		CHECK(canvas.pathOf(shown->font).find("label") != std::string::npos);
		world.teleport(chest.x + 4.0f, 0.0f, chest.z);
		canvas.calls.clear();
		screens.draw();
		CHECK(prompt() == nullptr);
		screens.setWorld(nullptr);
	}
	std::filesystem::remove_all(directory);
	device->drop();
}

TEST_CASE("on the title the first button has focus, arrows and Tab move it in layout order, and Enter presses it")
{
	Menu menu("screens_keys_title");
	menu.screens->draw();
	CHECK(menu.focused() == MenuScreens::GUI_ID_PLAY);
	menu.key(KEY_DOWN);
	CHECK(menu.focused() == MenuScreens::GUI_ID_SETTINGS);
	menu.key(KEY_DOWN);
	CHECK(menu.focused() == MenuScreens::GUI_ID_QUIT);
	menu.key(KEY_DOWN);
	CHECK(menu.focused() == MenuScreens::GUI_ID_QUIT);
	menu.key(KEY_TAB);
	CHECK(menu.focused() == MenuScreens::GUI_ID_PLAY);
	menu.key(KEY_TAB, true);
	CHECK(menu.focused() == MenuScreens::GUI_ID_QUIT);
	menu.key(KEY_UP);
	menu.key(KEY_UP);
	CHECK(menu.focused() == MenuScreens::GUI_ID_PLAY);
	menu.key(KEY_RETURN);
	CHECK(menu.flow.screen() == menu::Screen::CharacterSelect);
	menu.screens->draw();
	menu.key(KEY_DOWN);
	CHECK(menu.focused() != -1);
}

TEST_CASE("a character is made with the keyboard alone: name, gender, profession, AEGIS points, skills and confirm")
{
	Menu menu("screens_keys_create");
	menu.flow.play();
	menu.flow.newCharacter();
	menu.screens->draw();
	CHECK(menu.focused() == MenuScreens::GUI_ID_NAME);
	menu.type("Kit");
	menu.key(KEY_LEFT);
	CHECK(menu.focused() == MenuScreens::GUI_ID_NAME);
	menu.key(KEY_DOWN);
	CHECK(menu.focused() == MenuScreens::GUI_ID_MALE);
	menu.key(KEY_RIGHT);
	CHECK(menu.focused() == MenuScreens::GUI_ID_FEMALE);
	menu.key(KEY_SPACE, false, L' ');
	menu.key(KEY_DOWN);
	CHECK(menu.focused() == MenuScreens::GUI_ID_PROFESSION);
	menu.key(KEY_DOWN);
	CHECK(menu.focused() == MenuScreens::GUI_ID_PROFESSION);
	int before = menu.flow.draft().placed[character::STRENGTH];
	REQUIRE(menu.tabTo(MenuScreens::GUI_ID_PLUS_FIRST + character::STRENGTH));
	menu.key(KEY_RETURN);
	CHECK(menu.flow.draft().placed[character::STRENGTH] == before + 1);
	REQUIRE(menu.tabTo(MenuScreens::GUI_ID_CREATE));
	menu.key(KEY_RETURN);
	REQUIRE(menu.flow.screen() == menu::Screen::SkillAllocation);
	menu.screens->draw();
	REQUIRE(menu.tabTo(MenuScreens::GUI_ID_SKILL_PLUS_FIRST + character::SPEECH));
	menu.key(KEY_RETURN);
	REQUIRE(menu.tabTo(MenuScreens::GUI_ID_CONFIRM));
	menu.key(KEY_RETURN);
	REQUIRE(menu.flow.characters().size() == 1);
	CHECK(menu.flow.characters()[0].name() == "Kit");
	CHECK(menu.flow.characters()[0].gender() == character::Gender::Female);
	CHECK(menu.flow.characters()[0].profession().name == menu.flow.professions()[1].name);
	CHECK(menu.flow.characters()[0].placedPoints(character::STRENGTH) == before + 1);
	CHECK(menu.flow.characters()[0].getSpeech() >= 1);
}

namespace
{
	void controls(IGUIElement* element, std::set<s32>& ids)
	{
		for (IGUIElement* child : element->getChildren())
		{
			EGUI_ELEMENT_TYPE type = child->getType();
			if (child->isVisible() && (type == EGUIET_BUTTON || type == EGUIET_EDIT_BOX || type == EGUIET_LIST_BOX))
				ids.insert(child->getID());
			if (type != EGUIET_LIST_BOX)
				controls(child, ids);
		}
	}
}

TEST_CASE("Tab reaches every control on every menu screen")
{
	Menu menu("screens_keys_every");
	menu.flow.play();
	menu.flow.newCharacter();
	REQUIRE(menu.flow.create("Rook", character::Gender::Male, 0));
	std::vector<std::function<void()> > screens = {
		[&menu] { menu.flow.returnToTitle(); },
		[&menu] { menu.flow.returnToTitle(); menu.flow.settings(); },
		[&menu] { menu.flow.returnToTitle(); menu.flow.play(); },
		[&menu] { menu.flow.returnToTitle(); menu.flow.play(); menu.flow.newCharacter(); },
		[&menu] { menu.flow.returnToTitle(); menu.flow.play(); menu.flow.newCharacter(); REQUIRE(menu.flow.toSkills("Kit", character::Gender::Male, 0)); },
		[&menu] { menu.flow.returnToTitle(); menu.flow.play(); menu.flow.highlight(0); REQUIRE(menu.flow.select()); },
		[&menu] { menu.flow.returnToTitle(); menu.flow.play(); menu.flow.highlight(0); REQUIRE(menu.flow.select()); menu.flow.host(); },
		[&menu] { menu.flow.returnToTitle(); menu.flow.play(); menu.flow.highlight(0); REQUIRE(menu.flow.select()); menu.flow.join(); } };
	for (size_t i = 0; i < screens.size(); ++i)
	{
		INFO(i);
		screens[i]();
		menu.screens->draw();
		std::set<s32> expected;
		controls(menu.device->getGUIEnvironment()->getRootGUIElement(), expected);
		REQUIRE(expected.size() > 0);
		std::set<s32> reached;
		for (size_t step = 0; step <= expected.size(); ++step)
		{
			reached.insert(menu.focused());
			menu.key(KEY_TAB);
		}
		CHECK(reached == expected);
	}
}

TEST_CASE("the focused control wears a bright accent outline just outside it, which follows the focus")
{
	IrrlichtDevice* device = createDevice(video::EDT_NULL, core::dimension2d<u32>(1280, 720));
	RecordingCanvas canvas;
	canvas.screenDimension = core::dimension2d<u32>(1280, 720);
	std::string directory = (std::filesystem::temp_directory_path() / "screens_keys_outline").string();
	std::filesystem::remove_all(directory);
	{
		menu::CharacterStore store(directory);
		menu::MenuFlow flow(store, professions(), 3);
		std::deque<std::string> status;
		MenuScreens screens(device, canvas, flow, nullptr, status, nullptr);
		auto outlined = [&canvas, device](s32 id) {
			core::rect<s32> area = device->getGUIEnvironment()->getRootGUIElement()->getElementFromId(id, true)->getAbsolutePosition();
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::RECT_OUTLINE && call.colour == theme::accent && call.area.UpperLeftCorner.X < area.UpperLeftCorner.X &&
					call.area.UpperLeftCorner.Y < area.UpperLeftCorner.Y && call.area.LowerRightCorner.X > area.LowerRightCorner.X &&
					call.area.LowerRightCorner.Y > area.LowerRightCorner.Y && call.area.getWidth() <= area.getWidth() + 8)
					return true;
			return false;
		};
		screens.draw();
		CHECK(outlined(MenuScreens::GUI_ID_PLAY));
		CHECK_FALSE(outlined(MenuScreens::GUI_ID_SETTINGS));
		SEvent down;
		down.EventType = EET_KEY_INPUT_EVENT;
		down.KeyInput.Key = KEY_DOWN;
		down.KeyInput.PressedDown = true;
		down.KeyInput.Shift = false;
		down.KeyInput.Control = false;
		down.KeyInput.Char = 0;
		CHECK(screens.onEvent(down));
		canvas.calls.clear();
		screens.draw();
		CHECK(outlined(MenuScreens::GUI_ID_SETTINGS));
		CHECK_FALSE(outlined(MenuScreens::GUI_ID_PLAY));
	}
	std::filesystem::remove_all(directory);
	device->drop();
}

TEST_CASE("once in the world the menu arrows stand down, so the Leave button is never walked onto by accident")
{
	Menu menu("screens_keys_playing");
	menu.flow.play();
	menu.flow.newCharacter();
	REQUIRE(menu.flow.create("Rook", character::Gender::Male, 0));
	menu.flow.highlight(0);
	REQUIRE(menu.flow.select());
	menu.flow.host();
	REQUIRE(menu.flow.startServer());
	REQUIRE(menu.flow.screen() == menu::Screen::Playing);
	menu.screens->draw();
	CHECK(menu.focused() != MenuScreens::GUI_ID_LEAVE);
	SEvent event;
	event.EventType = EET_KEY_INPUT_EVENT;
	event.KeyInput.Key = KEY_DOWN;
	event.KeyInput.PressedDown = true;
	event.KeyInput.Shift = false;
	event.KeyInput.Control = false;
	event.KeyInput.Char = 0;
	CHECK_FALSE(menu.screens->onEvent(event));
	CHECK(menu.focused() != MenuScreens::GUI_ID_LEAVE);
}

namespace
{
	core::rect<s32> areaOf(Menu& menu, s32 id)
	{
		IGUIElement* element = menu.device->getGUIEnvironment()->getRootGUIElement()->getElementFromId(id, true);
		REQUIRE(element != nullptr);
		return element->getAbsolutePosition();
	}

	bool exists(Menu& menu, s32 id)
	{
		return menu.device->getGUIEnvironment()->getRootGUIElement()->getElementFromId(id, true) != nullptr;
	}
}

TEST_CASE("Settings opens four category tabs down the left, Audio first with its three volume sliders on the right")
{
	Menu menu("screens_settings_tabs");
	menu.flow.settings();
	menu.screens->draw();
	for (s32 i = 0; i < 4; ++i)
		REQUIRE(exists(menu, MenuScreens::GUI_ID_TAB_FIRST + i));
	for (s32 i = 1; i < 4; ++i)
		CHECK(areaOf(menu, MenuScreens::GUI_ID_TAB_FIRST + i).UpperLeftCorner.Y > areaOf(menu, MenuScreens::GUI_ID_TAB_FIRST + i - 1).UpperLeftCorner.Y);
	for (s32 id : { MenuScreens::GUI_ID_MASTER_VOLUME, MenuScreens::GUI_ID_MUSIC_VOLUME, MenuScreens::GUI_ID_SFX_VOLUME })
	{
		REQUIRE(exists(menu, id));
		CHECK(areaOf(menu, id).UpperLeftCorner.X > areaOf(menu, MenuScreens::GUI_ID_TAB_FIRST).LowerRightCorner.X);
	}
	CHECK_FALSE(exists(menu, MenuScreens::GUI_ID_FULLSCREEN));
	CHECK(exists(menu, MenuScreens::GUI_ID_BACK));
	CHECK(menu.focused() == MenuScreens::GUI_ID_TAB_FIRST);
	menu.key(KEY_DOWN);
	CHECK(menu.focused() == MenuScreens::GUI_ID_TAB_FIRST + 1);
	menu.key(KEY_RETURN);
	menu.screens->draw();
	CHECK(exists(menu, MenuScreens::GUI_ID_FULLSCREEN));
	CHECK_FALSE(exists(menu, MenuScreens::GUI_ID_MASTER_VOLUME));
	CHECK(menu.focused() == MenuScreens::GUI_ID_TAB_FIRST + 1);
	menu.screens->back();
	CHECK(menu.flow.screen() == menu::Screen::Title);
	menu.screens->draw();
	menu.flow.settings();
	menu.screens->draw();
	CHECK(exists(menu, MenuScreens::GUI_ID_MASTER_VOLUME));
}

TEST_CASE("a volume slider steps by five with Left and Right, stops at 0 and 100, jumps to a click, and every change is kept")
{
	Menu menu("screens_settings_sliders");
	std::vector<menu::Preferences> kept;
	menu.flow.onPreferences([&kept](const menu::Preferences& preferences) { kept.push_back(preferences); });
	menu.flow.settings();
	menu.screens->draw();
	REQUIRE(menu.tabTo(MenuScreens::GUI_ID_MASTER_VOLUME));
	menu.key(KEY_LEFT);
	CHECK(menu.flow.preferences().masterVolume == 95);
	CHECK(menu.focused() == MenuScreens::GUI_ID_MASTER_VOLUME);
	for (int i = 0; i < 3; ++i)
		menu.key(KEY_RIGHT);
	CHECK(menu.flow.preferences().masterVolume == 100);
	for (int i = 0; i < 30; ++i)
		menu.key(KEY_LEFT);
	CHECK(menu.flow.preferences().masterVolume == 0);
	CHECK(kept.back().masterVolume == 0);
	menu.key(KEY_DOWN);
	CHECK(menu.focused() == MenuScreens::GUI_ID_MUSIC_VOLUME);
	core::rect<s32> music = areaOf(menu, MenuScreens::GUI_ID_MUSIC_VOLUME);
	menu.mouse(EMIE_LMOUSE_PRESSED_DOWN, music.UpperLeftCorner.X + music.getWidth() / 4, music.getCenter().Y);
	CHECK(menu.flow.preferences().musicVolume == 25);
	menu.mouse(EMIE_LMOUSE_PRESSED_DOWN, music.LowerRightCorner.X - 1, music.getCenter().Y);
	CHECK(menu.flow.preferences().musicVolume == 100);
	REQUIRE(menu.tabTo(MenuScreens::GUI_ID_SFX_VOLUME));
	menu.key(KEY_LEFT);
	CHECK(menu.flow.preferences().sfxVolume == 95);
	CHECK(kept.back().sfxVolume == 95);
	CHECK(kept.back().masterVolume == 0);
	CHECK(kept.back().musicVolume == 100);
}

TEST_CASE("Display chooses windowed or fullscreen and cycles the resolution, Gameplay sets the time new worlds start with")
{
	Menu menu("screens_settings_display");
	menu.flow.settings();
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_TAB_FIRST + 1);
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_FULLSCREEN);
	CHECK(menu.flow.preferences().fullscreen);
	menu.press(MenuScreens::GUI_ID_WINDOWED);
	CHECK_FALSE(menu.flow.preferences().fullscreen);
	menu::Preferences big = menu.flow.preferences();
	big.width = 1920;
	big.height = 1080;
	menu.flow.setPreferences(big);
	auto moved = [&menu] {
		const menu::Preferences& now = menu.flow.preferences();
		return (now.width != 1920 || now.height != 1080) && now.width >= 1280 && now.height >= 720;
	};
	REQUIRE(menu.tabTo(MenuScreens::GUI_ID_RESOLUTION));
	menu.key(KEY_RIGHT);
	CHECK(menu.focused() == MenuScreens::GUI_ID_RESOLUTION);
	CHECK(moved());
	menu.flow.setPreferences(big);
	menu.key(KEY_LEFT);
	CHECK(moved());
	menu.flow.setPreferences(big);
	menu.press(MenuScreens::GUI_ID_RESOLUTION);
	CHECK(moved());

	menu.press(MenuScreens::GUI_ID_TAB_FIRST + 2);
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_DEFAULT_TIME_FIRST + 1);
	CHECK(menu.flow.preferences().time == menu::TimeMode::HalfSpeed);
	menu.screens->back();
	menu.flow.play();
	menu.flow.newCharacter();
	REQUIRE(menu.flow.create("Rook", character::Gender::Male, 0));
	menu.flow.highlight(0);
	REQUIRE(menu.flow.select());
	menu.flow.host();
	menu.screens->draw();
	menu.press(MenuScreens::GUI_ID_START);
	REQUIRE(menu.flow.screen() == menu::Screen::Playing);
	CHECK(menu.flow.timeScale() == menu::timeScale(menu::TimeMode::HalfSpeed));
}

TEST_CASE("the resolution choices are the driver's modes of 1280 by 720 and up, since the menus are laid out for that, each once, smallest first, always with the current size")
{
	std::vector<core::dimension2d<u32> > modes = { { 1920, 1080 }, { 640, 480 }, { 1280, 1024 }, { 1280, 720 }, { 1920, 1080 }, { 1024, 768 },
		{ 1366, 700 } };
	std::vector<core::dimension2d<u32> > choices = MenuScreens::resolutions(modes, core::dimension2d<u32>(1600, 900));
	const std::vector<core::dimension2d<u32> > expected = { { 1280, 720 }, { 1280, 1024 }, { 1600, 900 }, { 1920, 1080 } };
	CHECK(choices == expected);
	CHECK(MenuScreens::resolutions(std::vector<core::dimension2d<u32> >(), core::dimension2d<u32>(1280, 720)).size() == 1);
	const std::vector<core::dimension2d<u32> > bare = { { 1280, 720 }, { 1920, 1080 } };
	CHECK(MenuScreens::resolutions(std::vector<core::dimension2d<u32> >(), core::dimension2d<u32>(1920, 1080)) == bare);
}

TEST_CASE("Tab reaches every control on every settings tab, and the Controls tab is a read-only card of the keys")
{
	IrrlichtDevice* device = createDevice(video::EDT_NULL, core::dimension2d<u32>(1280, 720));
	RecordingCanvas canvas;
	canvas.screenDimension = core::dimension2d<u32>(1280, 720);
	std::string directory = (std::filesystem::temp_directory_path() / "screens_settings_controls").string();
	std::filesystem::remove_all(directory);
	{
		menu::CharacterStore store(directory);
		menu::MenuFlow flow(store, professions(), 3);
		std::deque<std::string> status;
		MenuScreens screens(device, canvas, flow, nullptr, status, nullptr);
		flow.settings();
		screens.draw();
		IGUIEnvironment* gui = device->getGUIEnvironment();
		std::set<std::wstring> seen;
		for (s32 tab = 0; tab < 4; ++tab)
		{
			INFO(tab);
			SEvent click;
			click.EventType = EET_GUI_EVENT;
			click.GUIEvent.EventType = EGET_BUTTON_CLICKED;
			click.GUIEvent.Caller = gui->getRootGUIElement()->getElementFromId(MenuScreens::GUI_ID_TAB_FIRST + tab, true);
			click.GUIEvent.Element = nullptr;
			REQUIRE(click.GUIEvent.Caller != nullptr);
			screens.onGuiEvent(click);
			canvas.calls.clear();
			screens.draw();
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::TEXT)
					seen.insert(call.text);
			std::set<s32> expected;
			controls(gui->getRootGUIElement(), expected);
			std::set<s32> reached;
			for (size_t step = 0; step <= expected.size(); ++step)
			{
				reached.insert(gui->getFocus() ? gui->getFocus()->getID() : -1);
				SEvent tabKey;
				tabKey.EventType = EET_KEY_INPUT_EVENT;
				tabKey.KeyInput.Key = KEY_TAB;
				tabKey.KeyInput.PressedDown = true;
				tabKey.KeyInput.Shift = false;
				tabKey.KeyInput.Control = false;
				tabKey.KeyInput.Char = 0;
				screens.onEvent(tabKey);
			}
			CHECK(reached == expected);
		}
		for (const wchar_t* line : { L"MASTER VOLUME", L"MUSIC VOLUME", L"SFX VOLUME", L"100", L"WINDOW", L"RESOLUTION", L"<   1280 x 720   >",
				 L"Applies the next time the game starts.", L"DEFAULT TIME FOR NEW WORLDS", L"Half Speed" })
		{
			std::string named;
			for (const wchar_t* letter = line; *letter; ++letter)
				named += (char)*letter;
			INFO(named);
			CHECK(seen.count(line) == 1);
		}
		auto says = [&canvas](const std::wstring& line) {
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::TEXT && call.text == line)
					return true;
			return false;
		};
		for (const wchar_t* line : { L"WASD", L"Move", L"Mouse", L"Look", L"I", L"Inventory", L"C", L"Notebook / Character Sheet", L"E",
				 L"Search Container", L"R", L"Rotate Item", L"F3", L"Debug Overlay", L"Backtick", L"Console", L"Escape", L"Back / Close",
				 L"Tab / Shift+Tab", L"Navigate UI", L"Arrow Keys", L"Enter / Space", L"Confirm", L"Gamepad A", L"Gamepad B", L"Back",
				 L"Gamepad X", L"Gamepad Start", L"Gamepad Back", L"Notebook", L"Stick / D-pad", L"Navigate" })
		{
			std::string named;
			for (const wchar_t* letter = line; *letter; ++letter)
				named += (char)*letter;
			INFO(named);
			CHECK(says(line));
		}
	}
	std::filesystem::remove_all(directory);
	device->drop();
}
