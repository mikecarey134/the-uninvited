#include "doctest.h"
#include "CharacterProfile.h"
#include "Multiplayer.h"
#include "RemoteStatsCommand.h"
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
			  host(hostTransport, [this] { return now; }, remoteEvents([this] { return hostWorld.get(); }, [](const std::string&) {}, hostSheets)),
			  guest(guestTransport, [this] { return now; }, remoteEvents([this] { return guestWorld.get(); }, [](const std::string&) {}, guestSheets)),
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
				hostWorld->advanceClock(0.05f);
				guestWorld->advanceClock(0.05f);
			}
		}

		IrrlichtDevice* device;
		std::map<uint16_t, character::CharacterSheet> hostSheets;
		std::map<uint16_t, character::CharacterSheet> guestSheets;
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

	character::CharacterSheet rook()
	{
		character::Aegis aegis;
		for (int i = 0; i < character::ATTRIBUTE_COUNT; ++i)
			aegis.rolled[i] = 10 + i;
		aegis.placed[character::STRENGTH] = 5;
		character::CharacterSheet sheet = character::CharacterSheet::create("Rook", character::Gender::Male, character::Profession{ "Blacksmith", { { character::SMITHING, 30 } }, {} }, aegis);
		sheet.setAge(52.0f);
		return sheet;
	}

	character::CharacterSheet dana()
	{
		character::Aegis aegis;
		for (int i = 0; i < character::ATTRIBUTE_COUNT; ++i)
			aegis.rolled[i] = 20 - i;
		aegis.hasBloodline = true;
		aegis.bloodlineAttribute = character::INTELLIGENCE;
		aegis.bloodline = "[Placeholder]";
		return character::CharacterSheet::create("Dana", character::Gender::Female,
			character::Profession{ "Paramedic", { { character::MEDICINE, 15 }, { character::ELECTRONICS, 10 }, { character::SURVIVALISM, 5 } }, {} }, aegis);
	}

	struct Lines : IC_MessageSink
	{
		void appendMessage(const WideString message) override { text.push_back(irr::core::stringc(message).c_str()); }
		void clearMessages() override { text.clear(); }
		void toggleVisible() override {}
		std::vector<std::string> text;
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

TEST_CASE("each player's full character arrives on the other side when they join, is listed by remote_stats, and is forgotten when they leave")
{
	Match match;
	match.host.setProfile(character::packProfile(rook()));
	match.guest.setProfile(character::packProfile(dana()));
	REQUIRE(match.session.host(4302, "Rook", menu::packAppearance(character::Gender::Male, 2)));
	REQUIRE(match.guest.connect("127.0.0.1", 4302, "Dana", menu::packAppearance(character::Gender::Female, 0)));
	match.pump(10);
	REQUIRE(match.guest.isJoined());
	uint16_t danaId = match.guest.localId();
	uint16_t rookId = match.host.localId();

	REQUIRE(match.hostSheets.size() == 1);
	REQUIRE(match.hostSheets.count(danaId) == 1);
	CHECK(character::describe(match.hostSheets.at(danaId)) == character::describe(dana()));
	CHECK(match.hostSheets.at(danaId).age() == dana().age());
	REQUIRE(match.guestSheets.size() == 1);
	REQUIRE(match.guestSheets.count(rookId) == 1);
	CHECK(character::describe(match.guestSheets.at(rookId)) == character::describe(rook()));
	CHECK(match.guestSheets.at(rookId).age() == 52.0f);
	CHECK(match.guestSheets.at(rookId).outfit() == 2);

	RemoteStatsCommand command(match.hostSheets);
	CHECK(command.getName() == L"remote_stats");
	Lines out;
	CHECK(command.invoke(array<WideString>(), nullptr, &out));
	std::vector<std::string> expected{ "Player " + std::to_string(danaId) + ", age 28" };
	for (const std::string& line : character::describe(dana()))
		expected.push_back(line);
	CHECK(out.text == expected);

	match.guest.shutdown();
	match.pump(10);
	CHECK(match.hostSheets.empty());
	Lines none;
	CHECK(command.invoke(array<WideString>(), nullptr, &none));
	CHECK(none.text == std::vector<std::string>{ "No other players are connected" });
}

TEST_CASE("everyone who joins, the host included, takes the server's time of day and time compression from the welcome")
{
	Match match;
	match.session.setClock(5.5f, 72.0f);
	REQUIRE(match.session.host(4303, "Rook", menu::packAppearance(character::Gender::Male, 2), 2));
	match.pump(5);
	REQUIRE(match.host.isJoined());
	CHECK(match.hostWorld->timeCompression() == 72.0f);
	CHECK(match.hostWorld->timeOfDay() == doctest::Approx(5.5f).epsilon(0.01));
	CHECK(match.guestWorld->timeCompression() == 1.0f);
	CHECK(match.guestWorld->timeOfDay() == doctest::Approx(World::START_HOUR).epsilon(0.001));

	match.pump(495);
	REQUIRE(match.guest.connect("127.0.0.1", 4303, "Dana", menu::packAppearance(character::Gender::Female, 0)));
	match.pump(5);
	REQUIRE(match.guest.isJoined());
	CHECK(match.guestWorld->timeCompression() == 72.0f);
	CHECK(match.guestWorld->timeOfDay() == doctest::Approx(6.0f).epsilon(0.01));
	CHECK(match.guestWorld->timeOfDay() == doctest::Approx(match.hostWorld->timeOfDay()).epsilon(0.002));
}
