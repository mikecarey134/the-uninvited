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
#include "GuiInputLayer.h"
#include "InputRouter.h"
#include "IrrlichtCanvas.h"
#include "MenuFlow.h"
#include "MenuScreens.h"
#include "MenuScene.h"
#include "MenuSkin.h"
#include "MouseCapture.h"
#include "Notebook.h"
#include "Outfits.h"
#include "PlayerStatsCommand.h"
#include "SoundEngine.h"
#include "World.h"
#include "client/NetSession.h"
#include "Multiplayer.h"
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

	void run(IrrlichtDevice* device, gamecore::SoundEngine* sound, bool smoke)
	{
		IrrlichtCanvas canvas(device);
		menu::CharacterStore store("saves/characters");
		menu::MenuFlow flow(store, loadProfessionList());

		std::deque<std::string> status;
		std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
		auto clock = [started] {
			return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count();
		};
		std::unique_ptr<World> world;
		NetClient::Events events = remoteEvents([&world] { return world.get(); }, [&status](const std::string& text) {
			status.push_back(text);
			if (status.size() > 8)
				status.pop_front();
		});
		EnetTransport clientTransport;
		EnetTransport serverTransport;
		NetClient client(clientTransport, clock, events);
		NetSession session(serverTransport, client, clock, [](const std::string& line) { std::cout << line << std::endl; });

		MenuScene backdrop(device);
		MenuScreens screens(device, canvas, flow, sound, status, &backdrop);
		InputRouter router;
		GuiInputLayer layer(device->getGUIEnvironment(), [&screens](const SEvent& event) { return screens.onGuiEvent(event); },
			[&screens] { screens.back(); }, [] { return true; });
		router.push(&layer);
		router.push(&screens);

		IC_Console console;
		console.loadDefaultCommands(device);
		console.registerCommand(new PlayerStatsCommand([&flow] { return flow.chosen(); }));
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
		Notebook notebook([&console] { return console.isVisible(); }, [&console] { console.setVisible(false); });
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
				bool hosting = flow.session() == menu::Session::Host;
				bool connecting = hosting ? session.host(flow.port(), name, look) : session.join(flow.address(), flow.port(), name, look);
				if (!connecting)
					status.push_back(hosting ? "Port " + std::to_string(flow.port()) + " is already in use"
						: "Unable to reach " + flow.address() + ":" + std::to_string(flow.port()));
				playing = true;
				if (const character::CharacterSheet* chosen = flow.chosen())
				{
					world.reset(new World(device, menu::characterModel(chosen->gender()),
						menu::outfitSkin(chosen->gender(), chosen->outfit()).c_str(), [&flow] { flow.returnToTitle(); }, *chosen));
					screens.setWorld(world.get());
					router.push(world.get());
					router.push(&notebook);
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
					capture.setCaptured(false);
					notebook.close();
					router.remove(&notebook);
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
					notebook.close();
				bool paused = console.isVisible() || notebook.isOpen();
				capture.setCaptured(!paused);
				capture.beginFrame();
				world->look(capture.delta().X * 0.25f);
				world->update(paused ? 0.0f : seconds);
			}

			session.tick();
			client.update(world ? world->localState() : protocol::State{ 0, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 0 });
			if (world)
				world->updateRemotes(seconds);

			device->getVideoDriver()->beginScene(true, true, video::SColor(255,0,0,0));
			screens.draw();
			if (world)
				notebook.draw(canvas, world->sheet());
			console.renderConsole(0);
			device->getVideoDriver()->endScene();

			if (smoke)
			{
				std::cout << "Smoke run finished one frame" << std::endl;
				break;
			}
		}

		session.leave();
		screens.hide();
		device->setEventReceiver(0);
	}
}

int main(int argc, char** argv)
{
	const bool smoke = argc > 1 && std::string(argv[1]) == "--smoke";
	IrrlichtDevice* device = createDevice(smoke ? video::EDT_NULL : video::EDT_OPENGL, core::dimension2d<u32>(1280, 720), 32);
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

	run(device, sound, smoke);

	if (sound)
		sound->drop();
	device->drop();
	return 0;
}
