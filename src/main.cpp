#include <chrono>
#include <deque>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <irrlicht.h>
#include "ActionMap.h"
#include "ConsoleLayer.h"
#include "Gamepad.h"
#include "GuiInputLayer.h"
#include "InputRouter.h"
#include "IrrlichtCanvas.h"
#include "MenuFlow.h"
#include "MenuScreens.h"
#include "MenuScene.h"
#include "MenuSkin.h"
#include "MouseCapture.h"
#include "Notebook.h"
#include "InventoryScreen.h"
#include "LootScreen.h"
#include "Outfits.h"
#include "PlayerStatsCommand.h"
#include "SoundEngine.h"
#include "World.h"
#include "client/NetSession.h"
#include "Multiplayer.h"
#include "Backpack.h"
#include "ChatQueue.h"
#include "DebugCommands.h"
#include "UiAnchor.h"
#include "Autosave.h"
#include "CharacterProfile.h"
#include "RemoteStatsCommand.h"
#include "net/EnetTransport.h"

using namespace irr;

namespace
{
	std::string readText(const char* path)
	{
		std::ifstream in(path, std::ios::binary);
		std::ostringstream text;
		text << in.rdbuf();
		return text.str();
	}

	std::vector<character::Profession> loadProfessionList()
	{
		std::vector<character::Profession> professions;
		character::LoadError error;
		if (!character::loadProfessions(readText("assets/config/professions.csv"), professions, error))
			std::cerr << "Professions did not load: " << error.file << " line " << error.line << ": " << error.message << std::endl;
		return professions;
	}

	const char* const settingsFile = "saves/settings.cfg";

	void run(IrrlichtDevice* device, gamecore::SoundEngine* sound, bool smoke, const menu::Preferences& preferences)
	{
		IrrlichtCanvas canvas(device);
		menu::CharacterStore store("saves/characters");
		menu::MenuFlow flow(store, loadProfessionList());
		flow.setWarning([](const std::string& line) { std::cerr << line << std::endl; });
		flow.setPreferences(preferences);
		if (sound)
			sound->setSoundVolume(preferences.masterVolume / 100.0f);
		flow.onPreferences([sound](const menu::Preferences& changed) {
			std::string text = changed.toText();
			std::ofstream out(settingsFile, std::ios::binary);
			out << text;
			if (!out)
				std::cerr << "Settings could not be saved to " << settingsFile << std::endl;
			if (sound)
				sound->setSoundVolume(changed.masterVolume / 100.0f);
			// TODO: apply musicVolume once the game plays music.
		});
		menu::Autosave autosave;

		std::deque<std::string> status;
		std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
		auto clock = [started] {
			return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count();
		};
		std::unique_ptr<World> world;
		std::map<uint16_t, character::CharacterSheet> remoteSheets;
		ChatQueue chat(device, canvas, anchored(core::rect<s32>(0, 420, 580, 500), 0.0f, 1.0f, canvas.screenSize()), 5000, true, 500, true, false);
		chat.setFont(canvas.loadFont("assets/fonts/console.bmp"));
		NetClient::Events events = remoteEvents([&world] { return world.get(); }, [&chat](const std::string& text, NetClient::LineKind kind) {
			chat.addMessage(core::stringw(text.c_str()), lineColour(kind));
		}, remoteSheets);
		EnetTransport clientTransport;
		EnetTransport serverTransport;
		NetClient client(clientTransport, clock, events);
		NetSession session(serverTransport, client, clock, [](const std::string& line) { std::cout << line << std::endl; });

		MenuScene backdrop(device);
		MenuScreens screens(device, canvas, flow, sound, status, &backdrop);
		InputRouter router;
		GamepadLayer gamepad([device](EKEY_CODE key) {
			SEvent event;
			event.EventType = EET_KEY_INPUT_EVENT;
			event.KeyInput.Key = key;
			event.KeyInput.Char = 0;
			event.KeyInput.Shift = false;
			event.KeyInput.Control = false;
			for (bool down : { true, false })
			{
				event.KeyInput.PressedDown = down;
				device->postEventFromUser(event);
			}
		});
		router.push(&gamepad);
		core::array<SJoystickInfo> joysticks;
		if (device->activateJoysticks(joysticks))
			std::cout << "Gamepads found: " << joysticks.size() << std::endl;
		GuiInputLayer layer(device->getGUIEnvironment(), [&screens](const SEvent& event) { return screens.onGuiEvent(event); },
			[&screens] { screens.back(); }, [] { return true; });
		router.push(&layer);
		router.push(&screens);

		IC_Console console;
		console.loadDefaultCommands(device);
		console.registerCommand(new PlayerStatsCommand([&flow] { return flow.chosen(); }));
		console.registerCommand(new RemoteStatsCommand(remoteSheets));
		LuaVM lua;
		lua.setLogger([&console](const std::string& message) {
			console.logMessage_ANSI(ELL_ERROR, message.c_str());
			std::cerr << message << std::endl;
		});
		for (std::unique_ptr<IC_Command>& command : debugCommands([&world] { return world.get(); }, [&client](const std::string& text) { client.sendChat(text); }, lua))
			console.registerCommand(command.release());
		console.getConfig().fontName = "assets/fonts/label.png";
		core::dimension2d<u32> screen = device->getVideoDriver()->getScreenSize();
		console.initializeConsole(canvas, core::dimension2d<s32>((s32)screen.Width, (s32)screen.Height));
		console.setVisible(false);
		ActionMap actions;
		actions.bind("console", KEY_OEM_3);
		ConsoleLayer consoleLayer(console, actions, [&flow] { return flow.screen() == menu::Screen::Playing; });
		router.push(&consoleLayer);
		device->setEventReceiver(&router);

		MouseCapture capture(device->getCursorControl());
		inventory::Catalog itemCatalog;
		std::string itemProblem;
		bool itemsLoaded = backpack::loadCatalog(itemCatalog, itemProblem);
		if (!itemsLoaded)
			std::cerr << "Inventory data did not load: " << itemProblem << std::endl;
		std::unique_ptr<inventory::Inventory> carried;
		auto played = [&world, &carried, &itemCatalog] {
			character::CharacterSheet sheet = world->sheet();
			if (carried)
				sheet.setCarried(backpack::capture(*carried, itemCatalog));
			return sheet;
		};
		std::unique_ptr<backpack::Page> carriedPage;
		std::string wielded;
		Notebook* notebookOpen = nullptr;
		LootScreen* lootOpen = nullptr;
		InventoryScreen inventoryScreen([&console] { return console.isVisible(); }, [&console, &notebookOpen, &lootOpen] {
			console.setVisible(false);
			if (notebookOpen)
				notebookOpen->close();
			if (lootOpen)
				lootOpen->close();
		});
		Notebook notebook([&console] { return console.isVisible(); }, [&console, &inventoryScreen, &lootOpen] {
			console.setVisible(false);
			inventoryScreen.close();
			if (lootOpen)
				lootOpen->close();
		});
		notebookOpen = &notebook;
		LootScreen lootScreen([&console] { return console.isVisible(); }, [&console, &inventoryScreen, &notebook] {
			console.setVisible(false);
			inventoryScreen.close();
			notebook.close();
		}, [&world]() -> loot::Container* { return world && !world->dead() ? world->containerInReach() : nullptr; });
		lootOpen = &lootScreen;
		u32 lastFrame = device->getTimer()->getTime();
		bool playing = false;
		while (device->run())
		{
			if (flow.screen() == menu::Screen::Quit)
				device->closeDevice();
			if (!playing && flow.screen() == menu::Screen::Playing)
			{
				status.clear();
				std::string name = flow.chosen() ? flow.chosen()->name() : "player";
				uint8_t look = flow.chosen() ? menu::packAppearance(flow.chosen()->gender(), flow.chosen()->outfit()) : 0;
				client.setProfile(flow.chosen() ? character::packProfile(*flow.chosen()) : std::vector<uint8_t>());
				bool hosting = flow.session() == menu::Session::Host;
				if (hosting)
					session.setClock(World::START_HOUR, flow.timeScale());
				bool connecting = hosting ? session.host(flow.port(), name, look, flow.maxPlayers()) : session.join(flow.address(), flow.port(), name, look);
				if (!connecting)
					status.push_back(hosting ? "Port " + std::to_string(flow.port()) + " is already in use"
						: "Unable to reach " + flow.address() + ":" + std::to_string(flow.port()));
				playing = true;
				if (const character::CharacterSheet* chosen = flow.chosen())
				{
					world.reset(new World(device, menu::characterModel(chosen->gender()),
						menu::outfitSkin(chosen->gender(), chosen->outfit()).c_str(), [&flow] { flow.returnToTitle(); }, *chosen));
					world->setInteractionLog([&console](const std::string& line) {
						console.logMessage_ANSI(ELL_INFORMATION, line.c_str());
						std::cout << line << std::endl;
					});
					world->setOpenLoot([&lootScreen](loot::Container& container) { lootScreen.open(&container); });
					world->setInteractionBlocked([&console, &notebook, &inventoryScreen, &lootScreen] {
						return console.isVisible() || notebook.isOpen() || inventoryScreen.isOpen() || lootScreen.isOpen();
					});
					if (hosting)
						world->setTimeCompression(flow.timeScale());
					screens.setWorld(world.get());
					router.push(world.get());
					router.push(&notebook);
					router.push(&inventoryScreen);
					router.push(&lootScreen);
					if (itemsLoaded)
					{
						carried.reset(new inventory::Inventory(itemCatalog, backpack::COLUMNS, backpack::ROWS));
						for (const std::string& warning : backpack::restore(chosen->carried(), *carried, itemCatalog))
							std::cerr << "Inventory: " << warning << std::endl;
						carriedPage.reset(new backpack::Page(*carried, itemCatalog));
						wielded.clear();
						inventoryScreen.attach(carriedPage.get());
						lootScreen.attach(carried.get(), &itemCatalog);
					}
					capture.setCaptured(true);
				}
			}
			else if (playing && flow.screen() != menu::Screen::Playing)
			{
				console.setVisible(false);
				session.leave();
				playing = false;
				if (world)
				{
					flow.keep(played());
					capture.setCaptured(false);
					notebook.close();
					inventoryScreen.close();
					lootScreen.close();
					router.remove(&notebook);
					router.remove(&inventoryScreen);
					router.remove(&lootScreen);
					inventoryScreen.attach(nullptr);
					lootScreen.attach(nullptr, nullptr);
					carriedPage.reset();
					carried.reset();
					router.remove(world.get());
					screens.setWorld(nullptr);
					world.reset();
				}
			}

			u32 now = device->getTimer()->getTime();
			f32 seconds = now > lastFrame ? (now - lastFrame) / 1000.0f : 0.0f;
			lastFrame = now;
			if (world)
			{
				if (console.isVisible())
				{
					notebook.close();
					inventoryScreen.close();
					lootScreen.close();
				}
				bool paused = console.isVisible() || notebook.isOpen() || inventoryScreen.isOpen() || lootScreen.isOpen();
				capture.setCaptured(!paused && !world->contextPanelOpen());
				capture.beginFrame();
				world->look(capture.delta().X * 0.25f);
				world->update(paused ? 0.0f : seconds);
				if (carried)
				{
					world->arm(backpack::hudWeapon(*carried, itemCatalog));
					std::string weapon = backpack::wieldedWeapon(*carried, itemCatalog);
					if (weapon != wielded && flow.chosen())
					{
						wielded = weapon;
						std::string note;
						world->wield(menu::characterModel(flow.chosen()->gender(), weapon, note));
						if (!note.empty())
							std::cerr << "Debug: " << note << std::endl;
					}
				}
				if (autosave.due(seconds))
					flow.keep(played());
			}

			session.tick();
			client.update(world ? world->localState() : protocol::State{ 0, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 0 });
			if (world)
			{
				world->updateRemotes(seconds);
				world->advanceClock(seconds);
			}

			device->getVideoDriver()->beginScene(true, true, video::SColor(255,0,0,0));
			screens.draw();
			if (world)
			{
				chat.draw();
				notebook.draw(canvas, world->sheet());
				inventoryScreen.draw(canvas);
				lootScreen.draw(canvas);
			}
			console.renderConsole(0);
			device->getVideoDriver()->endScene();

			if (smoke)
			{
				std::cout << "Smoke run finished one frame" << std::endl;
				break;
			}
		}

		if (world)
			flow.keep(played());
		session.leave();
		screens.hide();
		device->setEventReceiver(0);
	}
}

int main(int argc, char** argv)
{
	const bool smoke = argc > 1 && std::string(argv[1]) == "--smoke";
	std::vector<std::string> problems;
	menu::Preferences preferences = menu::Preferences::fromText(readText(settingsFile), problems);
	for (const std::string& problem : problems)
		std::cerr << "Settings: " << problem << std::endl;
	IrrlichtDevice* device = createDevice(smoke ? video::EDT_NULL : video::EDT_OPENGL,
		core::dimension2d<u32>((u32)preferences.width, (u32)preferences.height), 32, preferences.fullscreen);
	if (!device)
	{
		std::cerr << "Could not open the saved window, using 1280 x 720 windowed" << std::endl;
		device = createDevice(smoke ? video::EDT_NULL : video::EDT_OPENGL, core::dimension2d<u32>(1280, 720), 32);
	}
	if (!device)
		return 1;
	device->setWindowCaption(L"The Uninvited");
	video::SMaterial& flat = device->getVideoDriver()->getMaterial2D();
	flat.UseMipMaps = true;
	flat.TextureLayer[0].BilinearFilter = true;
	flat.TextureLayer[0].TrilinearFilter = true;
	device->getVideoDriver()->enableMaterial2D(true);
	gui::IGUIEnvironment* gui = device->getGUIEnvironment();
	gui::IGUISkin* skin = new MenuSkin(gui, gui->getSkin());
	gui->setSkin(skin);
	skin->drop();
	gamecore::SoundEngine* sound = gamecore::createSoundEngine();

	run(device, sound, smoke, preferences);

	if (sound)
		sound->drop();
	device->drop();
	return 0;
}
