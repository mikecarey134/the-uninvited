#include "doctest.h"
#include "CharacterStore.h"
#include "IrrlichtCanvas.h"
#include "MenuFlow.h"
#include "MenuScene.h"
#include "MenuScreens.h"
#include "Outfits.h"
#include "World.h"
#include "Hud.h"
#include "RecordingCanvas.h"
#include <deque>
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
	menu.screens->draw();
	CHECK(endsWith(skinOf(menu.backdrop), "playerskin_m3.jpg"));
}

TEST_CASE("every profession row on the create screen fits inside its list, long ones shortened with an ellipsis")
{
	Menu menu("screens_professions");
	menu.flow.play();
	menu.flow.newCharacter();
	menu.screens->draw();
	IGUIElement* element = menu.device->getGUIEnvironment()->getRootGUIElement()->getElementFromId(MenuScreens::GUI_ID_PROFESSION, true);
	REQUIRE(element != nullptr);
	IGUIListBox* list = static_cast<IGUIListBox*>(element);
	IGUISkin* skin = menu.device->getGUIEnvironment()->getSkin();
	IGUIFont* font = skin->getFont();
	REQUIRE(font != nullptr);
	s32 room = list->getAbsolutePosition().getWidth() - skin->getSize(EGDS_SCROLLBAR_SIZE) - 2 * skin->getSize(EGDS_TEXT_DISTANCE_X);
	REQUIRE(list->getItemCount() == menu.flow.professions().size());
	int shortened = 0;
	for (u32 i = 0; i < list->getItemCount(); ++i)
	{
		std::wstring item = list->getListItem(i);
		INFO(core::stringc(item.c_str()).c_str());
		CHECK((s32)font->getDimension(item.c_str()).Width <= room);
		CHECK(item.find(std::wstring(menu.flow.professions()[i].name.begin(), menu.flow.professions()[i].name.end())) != std::wstring::npos);
		if (item.size() >= 3 && item.compare(item.size() - 3, 3, L"...") == 0)
			++shortened;
	}
	CHECK(shortened >= 1);
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
