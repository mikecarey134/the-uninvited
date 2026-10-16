#include "doctest.h"
#include "Multiplayer.h"
#include "Outfits.h"
#include "World.h"
#include "client/NetSession.h"
#include "net/LoopbackTransport.h"
#include <memory>
#include <string>

using namespace irr;

namespace
{
	struct Match
	{
		Match()
			: device(createDevice(video::EDT_NULL, core::dimension2d<u32>(1280, 720))), serverTransport(network), hostTransport(network),
			  guestTransport(network),
			  host(hostTransport, [this] { return now; }, remoteEvents([this] { return hostWorld.get(); }, [](const std::string&) {})),
			  guest(guestTransport, [this] { return now; }, remoteEvents([this] { return guestWorld.get(); }, [](const std::string&) {})),
			  session(serverTransport, host, [this] { return now; }, [](const std::string&) {})
		{
			hostWorld.reset(new World(device, menu::characterModel(character::Gender::Male), menu::outfitSkin(character::Gender::Male, 2).c_str(), [] {}));
			guestWorld.reset(new World(device, menu::characterModel(character::Gender::Female), menu::outfitSkin(character::Gender::Female, 0).c_str(), [] {}));
		}
		~Match()
		{
			hostWorld.reset();
			guestWorld.reset();
			device->drop();
		}

		void key(World& world, EKEY_CODE code, bool down)
		{
			SEvent event;
			event.EventType = EET_KEY_INPUT_EVENT;
			event.KeyInput.Key = code;
			event.KeyInput.PressedDown = down;
			event.KeyInput.Char = 0;
			event.KeyInput.Shift = false;
			event.KeyInput.Control = false;
			world.onEvent(event);
		}

		void pump(int rounds)
		{
			for (int i = 0; i < rounds; ++i)
			{
				now += 50;
				session.tick();
				hostWorld->update(0.05f);
				guestWorld->update(0.05f);
				host.update(hostWorld->localState());
				guest.update(guestWorld->localState());
				hostWorld->updateRemotes(0.05f);
				guestWorld->updateRemotes(0.05f);
			}
		}

		IrrlichtDevice* device;
		uint32_t now = 1000;
		LoopbackNetwork network;
		LoopbackTransport serverTransport;
		LoopbackTransport hostTransport;
		LoopbackTransport guestTransport;
		std::unique_ptr<World> hostWorld;
		std::unique_ptr<World> guestWorld;
		NetClient host;
		NetClient guest;
		NetSession session;
	};

	bool endsWith(const std::string& text, const std::string& tail)
	{
		return text.size() >= tail.size() && text.compare(text.size() - tail.size(), tail.size(), tail) == 0;
	}
}

TEST_CASE("a guest who joins the host's game appears in the host's world in their own model and outfit, follows where they move, and goes when they leave")
{
	Match match;
	REQUIRE(match.session.host(4300, "Rook", menu::packAppearance(character::Gender::Male, 2)));
	REQUIRE(match.guest.connect("127.0.0.1", 4300, "Dana", menu::packAppearance(character::Gender::Female, 0)));
	match.pump(10);
	REQUIRE(match.guest.isJoined());
	uint16_t id = match.guest.localId();
	scene::IAnimatedMeshSceneNode* dana = match.hostWorld->remote(id);
	REQUIRE(dana != nullptr);
	CHECK(dana->getMesh() == match.device->getSceneManager()->getMesh("assets/models/stick_dan.ms3d"));
	CHECK(endsWith(dana->getMaterial(0).getTexture(0)->getName().getPath().c_str(), "playerskin_d1.bmp"));
	CHECK(match.hostWorld->remote(match.host.localId()) == nullptr);

	match.key(*match.guestWorld, KEY_KEY_D, true);
	match.pump(10);
	match.key(*match.guestWorld, KEY_KEY_D, false);
	match.pump(10);
	core::vector3df there = match.guestWorld->player()->getPosition();
	REQUIRE(there.X > 1.0f);
	CHECK(dana->getPosition().X == doctest::Approx(there.X));
	CHECK(dana->getPosition().Z == doctest::Approx(there.Z));
	CHECK(dana->getRotation().Y == doctest::Approx(90.0f));

	match.guest.shutdown();
	match.pump(10);
	CHECK(match.hostWorld->remote(id) == nullptr);
}

TEST_CASE("a guest sees the host who was already there, in the host's model and outfit, where the host stands, walking while the host walks")
{
	Match match;
	REQUIRE(match.session.host(4301, "Rook", menu::packAppearance(character::Gender::Male, 2)));
	match.key(*match.hostWorld, KEY_KEY_W, true);
	match.pump(10);
	match.key(*match.hostWorld, KEY_KEY_W, false);
	REQUIRE(match.guest.connect("127.0.0.1", 4301, "Dana", menu::packAppearance(character::Gender::Female, 0)));
	match.pump(20);
	REQUIRE(match.guest.isJoined());
	scene::IAnimatedMeshSceneNode* rook = match.guestWorld->remote(match.host.localId());
	REQUIRE(rook != nullptr);
	CHECK(rook->getMesh() == match.device->getSceneManager()->getMesh("assets/models/stick_mike.ms3d"));
	CHECK(endsWith(rook->getMaterial(0).getTexture(0)->getName().getPath().c_str(), "playerskin_m3.jpg"));
	core::vector3df there = match.hostWorld->player()->getPosition();
	REQUIRE(there.Z > 1.0f);
	CHECK(rook->getPosition().X == doctest::Approx(there.X));
	CHECK(rook->getPosition().Z == doctest::Approx(there.Z));
	CHECK(rook->getStartFrame() == 206);
	match.key(*match.hostWorld, KEY_KEY_W, true);
	match.pump(5);
	CHECK(rook->getStartFrame() == 301);
	match.key(*match.hostWorld, KEY_KEY_W, false);
	match.pump(5);
	CHECK(rook->getStartFrame() == 206);
}
