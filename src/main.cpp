#include <chrono>
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <irrlicht.h>
#include "GuiInputLayer.h"
#include "InputRouter.h"
#include "IrrlichtCanvas.h"
#include "MenuFlow.h"
#include "MenuScreens.h"
#include "MenuSkin.h"
#include "SoundEngine.h"
#include "client/NetSession.h"
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
		NetClient::Events events;
		events.line = [&status](const std::string& text, NetClient::LineKind) {
			status.push_back(text);
			if (status.size() > 8)
				status.pop_front();
		};
		EnetTransport clientTransport;
		EnetTransport serverTransport;
		NetClient client(clientTransport, clock, events);
		NetSession session(serverTransport, client, clock, [](const std::string& line) { std::cout << line << std::endl; });

		MenuScreens screens(device, canvas, flow, sound, status);
		InputRouter router;
		GuiInputLayer layer(device->getGUIEnvironment(), [&screens](const SEvent& event) { return screens.onGuiEvent(event); },
			[&screens] { screens.back(); }, [] { return true; });
		router.push(&layer);
		device->setEventReceiver(&router);

		bool playing = false;
		while (device->run())
		{
			if (flow.screen() == menu::Screen::Quit)
				device->closeDevice();
			if (!playing && flow.screen() == menu::Screen::Playing)
			{
				status.clear();
				std::string name = flow.chosen() ? flow.chosen()->name() : "player";
				bool hosting = flow.session() == menu::Session::Host;
				bool connecting = hosting ? session.host(flow.port(), name, 0) : session.join(flow.address(), flow.port(), name, 0);
				if (!connecting)
					status.push_back(hosting ? "Port " + std::to_string(flow.port()) + " is already in use"
						: "Unable to reach " + flow.address() + ":" + std::to_string(flow.port()));
				playing = true;
			}
			else if (playing && flow.screen() != menu::Screen::Playing)
			{
				session.leave();
				playing = false;
			}

			session.tick();
			client.update(protocol::State{ 0, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 0 });

			device->getVideoDriver()->beginScene(true, true, video::SColor(255,0,0,0));
			screens.draw();
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
