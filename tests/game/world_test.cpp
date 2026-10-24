#include "doctest.h"
#include "World.h"
#include "Sky.h"
#include "Loot.h"
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>

using namespace irr;
using namespace core;
using namespace scene;

namespace
{
	const char* const mike = "assets/models/stick_mike.ms3d";
	const char* const dan = "assets/models/stick_dan.ms3d";
	const char* const greenKnit = "assets/models/playerskin_m2.jpg";

	struct Fixture
	{
		explicit Fixture(const char* model = mike, const char* skin = greenKnit)
			: device(createDevice(video::EDT_NULL, dimension2d<u32>(1280, 720))), left(0),
			  world(new World(device, model, skin, [this] { ++left; }))
		{
		}
		explicit Fixture(const character::CharacterSheet& sheet)
			: device(createDevice(video::EDT_NULL, dimension2d<u32>(1280, 720))), left(0),
			  world(new World(device, mike, greenKnit, [this] { ++left; }, sheet))
		{
		}
		~Fixture()
		{
			delete world;
			device->drop();
		}

		void key(EKEY_CODE code, bool down)
		{
			SEvent event;
			event.EventType = EET_KEY_INPUT_EVENT;
			event.KeyInput.Key = code;
			event.KeyInput.PressedDown = down;
			event.KeyInput.Char = 0;
			event.KeyInput.Shift = false;
			event.KeyInput.Control = false;
			world->onEvent(event);
		}

		void walk(EKEY_CODE code, f32 seconds)
		{
			key(code, true);
			world->update(seconds);
			key(code, false);
		}

		IrrlichtDevice* device;
		int left;
		World* world;
	};

	bool endsWith(const std::string& text, const std::string& tail)
	{
		return text.size() >= tail.size() && text.compare(text.size() - tail.size(), tail.size(), tail) == 0;
	}
}

TEST_CASE("the world is a flat ground at least 200 m across, under the menu's fog, with the player standing on it at the origin")
{
	Fixture fixture;
	ISceneNode* ground = fixture.world->scene()->getSceneNodeFromName("ground");
	REQUIRE(ground != nullptr);
	ground->updateAbsolutePosition();
	aabbox3df flat = ground->getTransformedBoundingBox();
	CHECK(flat.getExtent().X >= 200.0f);
	CHECK(flat.getExtent().Z >= 200.0f);
	CHECK(flat.getExtent().Y < 0.01f);
	CHECK(flat.MaxEdge.Y == doctest::Approx(0.0f).epsilon(0.01));
	CHECK(ground->getMaterial(0).FogEnable);
	REQUIRE(ground->getMaterial(0).getTexture(0) != nullptr);
	CHECK(endsWith(ground->getMaterial(0).getTexture(0)->getName().getPath().c_str(), "ground_brown.jpg"));

	IAnimatedMeshSceneNode* player = fixture.world->player();
	REQUIRE(player != nullptr);
	player->updateAbsolutePosition();
	aabbox3df body = player->getTransformedBoundingBox();
	CHECK(body.MinEdge.Y == doctest::Approx(0.0f).epsilon(0.05));
	CHECK(body.getExtent().Y == doctest::Approx(1.75f).epsilon(0.1));
	CHECK(std::fabs(player->getPosition().X) < 0.001f);
	CHECK(std::fabs(player->getPosition().Z) < 0.001f);
	CHECK(player->getMaterial(0).FogEnable);
	CHECK(fixture.world->scene() != fixture.device->getSceneManager());
}

TEST_CASE("the player wears the model and outfit chosen at character creation")
{
	Fixture fixture(dan, "assets/models/playerskin_d1.bmp");
	CHECK(fixture.world->player()->getMesh() == fixture.device->getSceneManager()->getMesh(dan));
	std::string skin = fixture.world->player()->getMaterial(0).getTexture(0)->getName().getPath().c_str();
	CHECK(endsWith(skin, "playerskin_d1.bmp"));
}

TEST_CASE("the camera sits behind and above the player and looks at them")
{
	Fixture fixture;
	ICameraSceneNode* camera = fixture.world->camera();
	vector3df eye = camera->getPosition();
	vector3df player = fixture.world->player()->getPosition();
	CHECK(eye.Z < player.Z - 2.0f);
	CHECK(std::fabs(eye.X - player.X) < 0.01f);
	CHECK(eye.Y > 1.8f);
	f32 distance = vector3df(eye.X - player.X, 0.0f, eye.Z - player.Z).getLength();
	CHECK(distance > 3.0f);
	CHECK(distance < 7.0f);
	vector3df target = camera->getTarget();
	CHECK(target.Y > 0.8f);
	CHECK(target.Y < 2.0f);
	CHECK(vector3df(target.X - player.X, 0.0f, target.Z - player.Z).getLength() < 1.5f);
}

TEST_CASE("W walks the way the camera faces, S walks back, A and D strafe, and the player turns to face where they walk")
{
	Fixture fixture;
	fixture.walk(KEY_KEY_W, 1.0f);
	vector3df at = fixture.world->player()->getPosition();
	CHECK(at.Z > 2.0f);
	CHECK(at.Z < 6.0f);
	CHECK(std::fabs(at.X) < 0.001f);
	CHECK(fixture.world->player()->getRotation().Y == doctest::Approx(0.0f));

	fixture.walk(KEY_KEY_S, 1.0f);
	CHECK(fixture.world->player()->getPosition().Z == doctest::Approx(0.0f).epsilon(0.001));
	CHECK(std::fabs(fixture.world->player()->getRotation().Y) == doctest::Approx(180.0f));

	fixture.walk(KEY_KEY_D, 1.0f);
	CHECK(fixture.world->player()->getPosition().X == doctest::Approx(at.Z));
	CHECK(fixture.world->player()->getRotation().Y == doctest::Approx(90.0f));

	fixture.walk(KEY_KEY_A, 2.0f);
	CHECK(fixture.world->player()->getPosition().X == doctest::Approx(-at.Z));
	CHECK(std::fmod(fixture.world->player()->getRotation().Y + 360.0f, 360.0f) == doctest::Approx(270.0f));
}

TEST_CASE("walking diagonally is no faster than walking straight, and standing still stays put")
{
	Fixture fixture;
	fixture.key(KEY_KEY_W, true);
	fixture.key(KEY_KEY_D, true);
	fixture.world->update(1.0f);
	f32 diagonal = fixture.world->player()->getPosition().getLength();
	fixture.key(KEY_KEY_W, false);
	fixture.key(KEY_KEY_D, false);
	Fixture straight;
	straight.walk(KEY_KEY_W, 1.0f);
	CHECK(diagonal == doctest::Approx(straight.world->player()->getPosition().getLength()));
	vector3df before = fixture.world->player()->getPosition();
	fixture.world->update(1.0f);
	CHECK(fixture.world->player()->getPosition() == before);
}

TEST_CASE("looking right swings the camera round, W then walks the new way, and the camera follows the player")
{
	Fixture fixture;
	fixture.world->look(90.0f);
	fixture.world->update(0.0f);
	CHECK(fixture.world->camera()->getPosition().X < -2.0f);
	fixture.walk(KEY_KEY_W, 1.0f);
	vector3df at = fixture.world->player()->getPosition();
	CHECK(at.X > 2.0f);
	CHECK(std::fabs(at.Z) < 0.001f);
	CHECK(fixture.world->camera()->getPosition().X == doctest::Approx(at.X - fixture.world->cameraDistance()));
	CHECK(fixture.world->camera()->getTarget().X == doctest::Approx(at.X));
}

TEST_CASE("walking plays the walk loop where the model has one, and stopping goes back to idle")
{
	Fixture fixture;
	IAnimatedMeshSceneNode* player = fixture.world->player();
	CHECK(player->getStartFrame() == 206);
	CHECK(player->getEndFrame() == 250);
	fixture.key(KEY_KEY_W, true);
	fixture.world->update(0.1f);
	CHECK(player->getStartFrame() == 301);
	CHECK(player->getEndFrame() == 316);
	fixture.key(KEY_KEY_W, false);
	fixture.world->update(0.1f);
	CHECK(player->getStartFrame() == 206);

	Fixture shortModel(dan, "assets/models/playerskin_d1.bmp");
	shortModel.key(KEY_KEY_W, true);
	shortModel.world->update(0.1f);
	CHECK(shortModel.world->player()->getStartFrame() == 206);
	CHECK(shortModel.world->player()->getEndFrame() == 250);
}

TEST_CASE("Escape leaves the world, and other keys pass through to the layers below")
{
	Fixture fixture;
	SEvent escape;
	escape.EventType = EET_KEY_INPUT_EVENT;
	escape.KeyInput.Key = KEY_ESCAPE;
	escape.KeyInput.PressedDown = true;
	escape.KeyInput.Char = 0;
	escape.KeyInput.Shift = false;
	escape.KeyInput.Control = false;
	CHECK(fixture.world->onEvent(escape));
	CHECK(fixture.left == 1);
	SEvent w = escape;
	w.KeyInput.Key = KEY_KEY_W;
	CHECK_FALSE(fixture.world->onEvent(w));
	CHECK(fixture.left == 1);
}

namespace
{
	std::vector<aabbox3df> props(World* world)
	{
		std::vector<aabbox3df> boxes;
		for (ISceneNode* node : world->scene()->getRootSceneNode()->getChildren())
			if (std::string(node->getName()).compare(0, 5, "prop ") == 0)
			{
				node->updateAbsolutePosition();
				boxes.push_back(node->getTransformedBoundingBox());
			}
		return boxes;
	}

	f32 gap(const vector3df& at, const aabbox3df& box)
	{
		f32 dx = std::max(std::max(box.MinEdge.X - at.X, 0.0f), at.X - box.MaxEdge.X);
		f32 dz = std::max(std::max(box.MinEdge.Z - at.Z, 0.0f), at.Z - box.MaxEdge.Z);
		return std::sqrt(dx * dx + dz * dz);
	}

	void aimAt(World* world, const vector3df& at, f32& yaw)
	{
		vector3df from = world->player()->getPosition();
		f32 wanted = vector3df(at.X - from.X, 0.0f, at.Z - from.Z).getHorizontalAngle().Y;
		world->look(wanted - yaw);
		yaw = wanted;
	}
}

TEST_CASE("the world has eight to twelve of the game's own props standing on the ground, clear of the spawn point")
{
	Fixture fixture;
	std::vector<aabbox3df> boxes = props(fixture.world);
	CHECK(boxes.size() >= 8);
	CHECK(boxes.size() <= 12);
	for (const aabbox3df& box : boxes)
	{
		CHECK(box.MinEdge.Y == doctest::Approx(0.0f).epsilon(0.05));
		CHECK(box.getExtent().Y > 0.2f);
		CHECK(std::fabs(box.getCenter().X) < 90.0f);
		CHECK(std::fabs(box.getCenter().Z) < 90.0f);
		CHECK(gap(vector3df(), box) > 2.0f);
	}
	int kinds = 0;
	for (const char* kind : { "prop crate", "prop barrel", "prop chest", "prop log", "prop car", "prop car2" })
		kinds += fixture.world->scene()->getSceneNodeFromName(kind) != nullptr;
	CHECK(kinds == 6);
}

TEST_CASE("walking at the edge of the world stops the player there, in every direction")
{
	Fixture fixture;
	const f32 edge = 100.0f - fixture.world->playerRadius();
	for (int side = 0; side < 4; ++side)
	{
		fixture.world->look(side == 0 ? 0.0f : 90.0f);
		f32 yaw = 90.0f * side;
		f32 radians = yaw * DEGTORAD;
		vector3df heading(std::sin(radians), 0.0f, std::cos(radians));
		fixture.key(KEY_KEY_W, true);
		for (int i = 0; i < 1200; ++i)
			fixture.world->update(0.1f);
		fixture.key(KEY_KEY_W, false);
		vector3df at = fixture.world->player()->getPosition();
		INFO(side);
		CHECK(std::fabs(at.X) <= edge + 0.001f);
		CHECK(std::fabs(at.Z) <= edge + 0.001f);
		CHECK(at.dotProduct(heading) >= edge - 0.5f);
	}
}

TEST_CASE("the player never ends a frame inside a prop, however they wander, even across long frames")
{
	Fixture fixture;
	std::vector<aabbox3df> boxes = props(fixture.world);
	REQUIRE_FALSE(boxes.empty());
	const f32 radius = fixture.world->playerRadius();
	const EKEY_CODE keys[] = { KEY_KEY_W, KEY_KEY_D, KEY_KEY_S, KEY_KEY_A };
	for (int leg = 0; leg < 40; ++leg)
	{
		fixture.world->look(37.0f * (f32)(leg % 7) + 11.0f);
		fixture.key(keys[leg % 4], true);
		for (int i = 0; i < 30; ++i)
		{
			fixture.world->update(leg % 5 == 0 ? 0.7f : 0.05f);
			for (const aabbox3df& box : boxes)
				REQUIRE(gap(fixture.world->player()->getPosition(), box) >= radius - 0.001f);
		}
		fixture.key(keys[leg % 4], false);
	}
}

TEST_CASE("walking straight at a prop brings the player right up against it, then slides them along it")
{
	Fixture fixture;
	std::vector<aabbox3df> boxes = props(fixture.world);
	REQUIRE_FALSE(boxes.empty());
	const aabbox3df* nearest = &boxes[0];
	for (const aabbox3df& box : boxes)
		if (gap(vector3df(), box) < gap(vector3df(), *nearest))
			nearest = &box;
	f32 yaw = 0.0f;
	aimAt(fixture.world, nearest->getCenter(), yaw);
	f32 closest = 1000.0f;
	vector3df touching;
	fixture.key(KEY_KEY_W, true);
	for (int i = 0; i < 200; ++i)
	{
		fixture.world->update(0.05f);
		f32 now = gap(fixture.world->player()->getPosition(), *nearest);
		if (now < closest)
		{
			closest = now;
			touching = fixture.world->player()->getPosition();
		}
	}
	fixture.key(KEY_KEY_W, false);
	CHECK(closest == doctest::Approx(fixture.world->playerRadius()).epsilon(0.05));
	CHECK(fixture.world->player()->getPosition().getDistanceFrom(touching) > 1.0f);
}

TEST_CASE("one long frame walking head-on at a prop stops the player at it instead of passing through")
{
	Fixture fixture;
	ISceneNode* barrel = fixture.world->scene()->getSceneNodeFromName("prop barrel");
	REQUIRE(barrel != nullptr);
	barrel->updateAbsolutePosition();
	aabbox3df box = barrel->getTransformedBoundingBox();
	fixture.walk(KEY_KEY_A, -box.getCenter().X / 3.5f);
	REQUIRE(fixture.world->player()->getPosition().X == doctest::Approx(box.getCenter().X));
	REQUIRE(box.MinEdge.Z > 1.0f);
	fixture.walk(KEY_KEY_W, 5.0f);
	CHECK(fixture.world->player()->getPosition().Z == doctest::Approx(box.MinEdge.Z - fixture.world->playerRadius()).epsilon(0.03));
}

namespace
{
	character::CharacterSheet survivor(int water, int hunger, int health)
	{
		const character::Profession nobody{ "Nobody", {}, {} };
		character::Aegis aegis;
		for (int i = 0; i < character::ATTRIBUTE_COUNT; ++i)
			aegis.rolled[i] = 20;
		std::string text = character::CharacterSheet::create("Cal", character::Gender::Male, nobody, aegis).toText();
		auto set = [&text](const std::string& key, int value) {
			std::string::size_type at = text.find("\n" + key + " = ");
			REQUIRE(at != std::string::npos);
			std::string::size_type end = text.find('\n', at + 1);
			text.replace(at + 1, end - at - 1, key + " = " + std::to_string(value));
		};
		set("water", water);
		set("hunger", hunger);
		set("health", health);
		std::string problem;
		character::CharacterSheet sheet = character::CharacterSheet::fromText(text, problem);
		REQUIRE(problem.empty());
		return sheet;
	}
}

TEST_CASE("the world drains its player's meters once per whole game-second, which is frame time times the time compression")
{
	Fixture fixture(survivor(100, 100, 100));
	CHECK(fixture.world->timeCompression() == 1.0f);
	const double drain = character::waterDrainPerSecond(20);
	fixture.world->update(0.4f);
	fixture.world->update(0.4f);
	CHECK(fixture.world->sheet().water() == doctest::Approx(100.0f));
	fixture.world->update(0.4f);
	CHECK(fixture.world->sheet().water() == doctest::Approx(100.0 - drain).epsilon(5e-7));
	CHECK(fixture.world->sheet().hunger() == doctest::Approx(100.0 - character::hungerDrainPerSecond(20)).epsilon(5e-7));
	fixture.world->update(2.5f);
	CHECK(fixture.world->sheet().water() == doctest::Approx(100.0 - 3.0 * drain).epsilon(5e-7));

	Fixture doubled(survivor(100, 100, 100));
	doubled.world->setTimeCompression(2.0f);
	doubled.world->update(0.5f);
	CHECK(doubled.world->sheet().water() == doctest::Approx(100.0 - drain).epsilon(5e-7));
	doubled.world->update(1.5f);
	CHECK(doubled.world->sheet().water() == doctest::Approx(100.0 - 4.0 * drain).epsilon(5e-7));

	Fixture day(survivor(100, 100, 100));
	day.world->setTimeCompression(3600.0f);
	day.world->update(24.0f * 3.0f);
	CHECK(day.world->sheet().water() == doctest::Approx(0.0f).epsilon(0.001));
	CHECK(day.world->sheet().hunger() == doctest::Approx(100.0 * 4.0 / 7.0).epsilon(0.001));
}

TEST_CASE("at zero health the player is dead and frozen: no walking, no looking round, and Escape does not leave")
{
	Fixture fixture(survivor(0, 50, 2));
	fixture.world->setTimeCompression(3600.0f);
	CHECK_FALSE(fixture.world->dead());
	fixture.world->update(2.0f);
	CHECK(fixture.world->sheet().health() == 0);
	CHECK(fixture.world->dead());
	vector3df at = fixture.world->player()->getPosition();
	vector3df eye = fixture.world->camera()->getPosition();
	fixture.walk(KEY_KEY_W, 2.0f);
	fixture.world->look(90.0f);
	fixture.world->update(1.0f);
	CHECK(fixture.world->player()->getPosition() == at);
	CHECK(fixture.world->camera()->getPosition() == eye);
	f32 food = fixture.world->sheet().hunger();
	REQUIRE(food > 0.0f);
	fixture.world->update(5.0f);
	CHECK(fixture.world->sheet().hunger() == food);
	CHECK(fixture.world->player()->getAnimationSpeed() == 0.0f);
	SEvent escape;
	escape.EventType = EET_KEY_INPUT_EVENT;
	escape.KeyInput.Key = KEY_ESCAPE;
	escape.KeyInput.PressedDown = true;
	escape.KeyInput.Char = 0;
	escape.KeyInput.Shift = false;
	escape.KeyInput.Control = false;
	CHECK(fixture.world->onEvent(escape));
	CHECK(fixture.left == 0);
}

TEST_CASE("a remote player appears with its own model and skin, standing on the ground, and only once")
{
	Fixture fixture;
	fixture.world->addRemote(7, dan, "assets/models/playerskin_d1.bmp");
	fixture.world->addRemote(7, dan, "assets/models/playerskin_d1.bmp");
	IAnimatedMeshSceneNode* remote = fixture.world->remote(7);
	REQUIRE(remote != nullptr);
	CHECK(remote->getMesh() == fixture.device->getSceneManager()->getMesh(dan));
	CHECK(endsWith(remote->getMaterial(0).getTexture(0)->getName().getPath().c_str(), "playerskin_d1.bmp"));
	remote->updateAbsolutePosition();
	CHECK(remote->getTransformedBoundingBox().MinEdge.Y == doctest::Approx(0.0f).epsilon(0.05));
	int remotes = 0;
	for (ISceneNode* node : fixture.world->scene()->getRootSceneNode()->getChildren())
		if (std::string(node->getName()) == "remote 7")
			++remotes;
	CHECK(remotes == 1);
	CHECK(fixture.world->remote(8) == nullptr);
}

TEST_CASE("a remote player glides between the positions it reports and faces their yaw")
{
	Fixture fixture;
	fixture.world->addRemote(3, mike, greenKnit);
	const f32 first[3] = { 0.0f, 0.0f, 10.0f };
	const f32 second[3] = { 2.0f, 0.0f, 10.0f };
	fixture.world->placeRemote(3, 1, first, 90.0f, true);
	fixture.world->updateRemotes(0.05f);
	fixture.world->placeRemote(3, 2, second, 90.0f, true);
	fixture.world->updateRemotes(0.075f);
	f32 between = fixture.world->remote(3)->getPosition().X;
	CHECK(between > 0.1f);
	CHECK(between < 1.9f);
	CHECK(fixture.world->remote(3)->getPosition().Z == doctest::Approx(10.0f));
	fixture.world->updateRemotes(1.0f);
	CHECK(fixture.world->remote(3)->getPosition().X == doctest::Approx(2.0f));
	fixture.world->remote(3)->updateAbsolutePosition();
	CHECK(fixture.world->remote(3)->getTransformedBoundingBox().MinEdge.Y == doctest::Approx(0.0f).epsilon(0.05));
	CHECK(fixture.world->remote(3)->getRotation().Y == doctest::Approx(90.0f));
	const f32 back[3] = { 2.0f, 0.0f, 10.0f };
	fixture.world->placeRemote(3, 1, back, 0.0f, false);
	fixture.world->updateRemotes(1.0f);
	CHECK(fixture.world->remote(3)->getRotation().Y == doctest::Approx(90.0f));
}

TEST_CASE("a remote player walks while it reports walking, idles when it stops, and vanishes when it leaves")
{
	Fixture fixture;
	fixture.world->addRemote(4, mike, greenKnit);
	const f32 at[3] = { 1.0f, 0.0f, 5.0f };
	fixture.world->placeRemote(4, 1, at, 0.0f, true);
	fixture.world->updateRemotes(0.2f);
	CHECK(fixture.world->remote(4)->getStartFrame() == 301);
	CHECK(fixture.world->remote(4)->getEndFrame() == 316);
	fixture.world->placeRemote(4, 2, at, 0.0f, false);
	fixture.world->updateRemotes(0.2f);
	CHECK(fixture.world->remote(4)->getStartFrame() == 206);
	fixture.world->removeRemote(4);
	CHECK(fixture.world->remote(4) == nullptr);
	CHECK(fixture.world->scene()->getSceneNodeFromName("remote 4") == nullptr);
	fixture.world->removeRemote(4);
	fixture.world->updateRemotes(0.1f);
}

TEST_CASE("the state sent for the local player is where it stands, the way it faces, and whether it walks or is dead")
{
	Fixture fixture;
	protocol::State still = fixture.world->localState();
	CHECK(still.pos[0] == doctest::Approx(0.0f));
	CHECK(still.pos[1] == doctest::Approx(0.0f));
	CHECK(still.pos[2] == doctest::Approx(0.0f));
	CHECK(still.state == World::IDLE_STATE);
	fixture.key(KEY_KEY_D, true);
	fixture.world->update(1.0f);
	protocol::State walking = fixture.world->localState();
	CHECK(walking.pos[0] == doctest::Approx(fixture.world->player()->getPosition().X));
	CHECK(walking.pos[1] == doctest::Approx(0.0f));
	CHECK(walking.rot[1] == doctest::Approx(90.0f));
	CHECK(walking.state == World::WALKING_STATE);
	CHECK(protocol::isAlive(walking.state));
	fixture.key(KEY_KEY_D, false);

	Fixture doomed(survivor(0, 0, 1));
	doomed.world->setTimeCompression(3600.0f);
	doomed.world->update(1.0f);
	REQUIRE(doomed.world->dead());
	CHECK_FALSE(protocol::isAlive(doomed.world->localState().state));
}

namespace
{
	vector3df sunDirection(World& world)
	{
		ILightSceneNode* sun = world.sun();
		sun->updateAbsolutePosition();
		vector3df direction(0.0f, 0.0f, 1.0f);
		sun->getAbsoluteTransformation().rotateVect(direction);
		return direction.normalize();
	}
}

TEST_CASE("the world's clock starts at eight in the morning and runs at real seconds times the time compression, wrapping at midnight")
{
	Fixture fixture;
	CHECK(World::START_HOUR == 8.0f);
	CHECK(fixture.world->timeOfDay() == doctest::Approx(8.0f));
	fixture.world->advanceClock(1800.0f);
	CHECK(fixture.world->timeOfDay() == doctest::Approx(8.5f));
	fixture.world->setTimeCompression(72.0f);
	fixture.world->advanceClock(50.0f);
	CHECK(fixture.world->timeOfDay() == doctest::Approx(9.5f));
	fixture.world->advanceClock(20.0f * 60.0f);
	CHECK(fixture.world->timeOfDay() == doctest::Approx(9.5f).epsilon(0.001));
	fixture.world->setTimeOfDay(23.5f);
	fixture.world->advanceClock(100.0f);
	CHECK(fixture.world->timeOfDay() == doctest::Approx(1.5f).epsilon(0.001));
	fixture.world->setTimeOfDay(-2.0f);
	CHECK(fixture.world->timeOfDay() == doctest::Approx(22.0f));
	fixture.world->setTimeOfDay(50.0f);
	CHECK(fixture.world->timeOfDay() == doctest::Approx(2.0f));
}

TEST_CASE("pausing the world's survival does not stop its clock")
{
	Fixture fixture;
	fixture.world->setTimeCompression(72.0f);
	fixture.world->update(0.0f);
	fixture.world->advanceClock(50.0f);
	CHECK(fixture.world->timeOfDay() == doctest::Approx(9.0f));
}

TEST_CASE("the world's light, sun and sky follow its clock")
{
	Fixture fixture;
	CHECK(fixture.world->scene()->getSceneNodeFromName("sky") == nullptr);
	for (f32 hour : { 3.0f, 6.0f, 6.5f, 12.0f, 18.0f, 21.0f })
	{
		INFO(hour);
		fixture.world->setTimeOfDay(hour);
		SkyLook look = skyAt(hour);
		video::SColorf ambient = fixture.world->scene()->getAmbientLight();
		CHECK(ambient.r == doctest::Approx(look.ambient.r));
		CHECK(ambient.b == doctest::Approx(look.ambient.b));
		video::SColorf sun = fixture.world->sun()->getLightData().DiffuseColor;
		CHECK(sun.r == doctest::Approx(look.sun.r));
		CHECK(sun.g == doctest::Approx(look.sun.g));
		CHECK(fixture.world->sun()->getLightType() == video::ELT_DIRECTIONAL);
		vector3df direction = sunDirection(*fixture.world);
		CHECK(direction.X == doctest::Approx(look.sunDirection.X).epsilon(0.01));
		CHECK(direction.Y == doctest::Approx(look.sunDirection.Y).epsilon(0.01));
		CHECK(direction.Z == doctest::Approx(look.sunDirection.Z).epsilon(0.01));
		CHECK(fixture.world->skyHorizon() == look.horizon);
		CHECK(fixture.world->skyZenith() == look.zenith);
		fixture.world->draw();
		video::SColor fog;
		video::E_FOG_TYPE type;
		f32 start, end, density;
		bool pixel, range;
		fixture.device->getVideoDriver()->getFog(fog, type, start, end, density, pixel, range);
		CHECK(fog == look.horizon);
	}
	fixture.world->setTimeOfDay(11.0f);
	fixture.world->advanceClock(3600.0f);
	CHECK(fixture.world->skyHorizon() == skyAt(12.0f).horizon);
	CHECK(fixture.world->scene()->getAmbientLight().g == doctest::Approx(skyAt(12.0f).ambient.g));
}

TEST_CASE("no prop gives off its own light, so night darkens every one of them")
{
	Fixture fixture;
	int props = 0;
	for (ISceneNode* node : fixture.world->scene()->getRootSceneNode()->getChildren())
		if (std::string(node->getName()).compare(0, 5, "prop ") == 0)
			for (u32 i = 0; i < node->getMaterialCount(); ++i)
			{
				INFO(node->getName());
				video::SColor glow = node->getMaterial(i).EmissiveColor;
				CHECK(glow.getRed() + glow.getGreen() + glow.getBlue() == 0u);
				++props;
			}
	CHECK(props > 10);
}

TEST_CASE("F3 toggles the debug panel, even for a dead player, and other keys leave it alone")
{
	Fixture fixture;
	CHECK_FALSE(fixture.world->debugShown());
	fixture.key(KEY_F3, true);
	CHECK(fixture.world->debugShown());
	fixture.key(KEY_F3, false);
	CHECK(fixture.world->debugShown());
	fixture.key(KEY_KEY_W, true);
	CHECK(fixture.world->debugShown());
	fixture.key(KEY_F3, true);
	CHECK_FALSE(fixture.world->debugShown());

	Fixture doomed(survivor(0, 0, 1));
	doomed.world->setTimeCompression(3600.0f);
	doomed.world->update(1.0f);
	REQUIRE(doomed.world->dead());
	doomed.key(KEY_F3, true);
	CHECK(doomed.world->debugShown());
}

TEST_CASE("teleport puts the player's feet at the given point, speed sets how fast they walk, and health can be set or knocked down by five")
{
	Fixture fixture;
	fixture.world->teleport(12.0f, 0.0f, -7.5f);
	fixture.world->player()->updateAbsolutePosition();
	CHECK(fixture.world->player()->getPosition().X == doctest::Approx(12.0f));
	CHECK(fixture.world->player()->getPosition().Z == doctest::Approx(-7.5f));
	CHECK(fixture.world->player()->getTransformedBoundingBox().MinEdge.Y == doctest::Approx(0.0f).epsilon(0.05));
	CHECK(fixture.world->localState().pos[1] == doctest::Approx(0.0f));
	fixture.world->teleport(1.0f, 3.0f, 1.0f);
	CHECK(fixture.world->localState().pos[1] == doctest::Approx(3.0f));
	fixture.world->teleport(12.0f, 0.0f, -30.0f);

	CHECK(fixture.world->speed() == doctest::Approx(3.5f));
	fixture.world->setSpeed(10.0f);
	fixture.walk(KEY_KEY_D, 1.0f);
	CHECK(fixture.world->player()->getPosition().X == doctest::Approx(22.0f));

	fixture.world->setHealth(40);
	CHECK(fixture.world->sheet().health() == 40);
	fixture.world->hurt();
	CHECK(fixture.world->sheet().health() == 35);
	fixture.world->setHealth(500);
	CHECK(fixture.world->sheet().health() == character::CharacterSheet::MAX_HEALTH);
	fixture.world->setHealth(-3);
	CHECK(fixture.world->sheet().health() == 0);
	CHECK(fixture.world->dead());
	fixture.world->hurt();
	CHECK(fixture.world->sheet().health() == 0);
}

TEST_CASE("every container sits on a chest or crate prop of its own kind, and walking up to one puts it in reach")
{
	Fixture fixture;
	const std::vector<loot::Container>& boxes = fixture.world->containers();
	REQUIRE(boxes.size() == 4);
	for (const loot::Container& box : boxes)
	{
		INFO(box.kind);
		std::string wanted = box.kind == "Chest" ? "prop chest" : "prop crate";
		bool sitting = false;
		for (ISceneNode* node : fixture.world->scene()->getRootSceneNode()->getChildren())
			if (std::string(node->getName()) == wanted && std::fabs(node->getPosition().X - box.x) < 0.01f && std::fabs(node->getPosition().Z - box.z) < 0.01f)
				sitting = true;
		CHECK(sitting);
	}
	CHECK(fixture.world->containerInReach() == nullptr);
	fixture.world->teleport(boxes[1].x + 1.5f, 0.0f, boxes[1].z);
	REQUIRE(fixture.world->containerInReach() != nullptr);
	CHECK(fixture.world->containerInReach() == &fixture.world->containers()[1]);
	fixture.world->teleport(boxes[1].x + 3.0f, 0.0f, boxes[1].z);
	CHECK(fixture.world->containerInReach() == nullptr);
}

TEST_CASE("wielding swaps the player's whole mesh in place, keeping where they stand and face, their skin, and the animation mid-stride")
{
	const char* const knife = "assets/models/stick_mike_knife.ms3d";
	Fixture fixture;
	IAnimatedMeshSceneNode* player = fixture.world->player();
	fixture.key(KEY_KEY_D, true);
	fixture.world->update(0.4f);
	player->setCurrentFrame(305.5f);
	player->setAnimationSpeed(7.0f);
	vector3df at = player->getPosition();
	vector3df facing = player->getRotation();
	vector3df scale = player->getScale();
	f32 frame = player->getFrameNr();
	f32 speed = player->getAnimationSpeed();
	video::ITexture* skin = player->getMaterial(0).getTexture(0);
	REQUIRE(player->getStartFrame() == 301);
	REQUIRE(frame > 301.0f);

	fixture.world->wield(knife);
	CHECK(fixture.world->player() == player);
	CHECK(player->getMesh() == fixture.world->scene()->getMesh(knife));
	CHECK(player->getPosition() == at);
	CHECK(player->getRotation() == facing);
	CHECK(player->getScale() == scale);
	CHECK(player->getStartFrame() == 301);
	CHECK(player->getEndFrame() == 316);
	CHECK(player->getFrameNr() == doctest::Approx(frame));
	CHECK(player->getAnimationSpeed() == doctest::Approx(speed));
	CHECK(player->getMaterial(0).getTexture(0) == skin);
	CHECK(player->getMaterial(0).FogEnable);
	CHECK(player->getMaterial(0).NormalizeNormals);

	fixture.world->wield(mike);
	CHECK(player->getMesh() == fixture.world->scene()->getMesh(mike));
	CHECK(player->getStartFrame() == 301);
	fixture.world->wield("assets/models/no_such_model.ms3d");
	CHECK(player->getMesh() == fixture.world->scene()->getMesh(mike));
}

TEST_CASE("the world's own scene manager loads .fbx models through the engine's Assimp loader")
{
	Fixture fixture;
	const char* const rig = "engine/gamecorev0.1.1/gamecorev0.1.1/engine/tests/data/rig.fbx";
	IAnimatedMesh* mesh = fixture.world->scene()->getMesh(rig);
	REQUIRE(mesh != nullptr);
	CHECK(mesh->getMeshBufferCount() > 0);
	CHECK(mesh->getFrameCount() > 1);
}

TEST_CASE("walking up to the lone test terminal targets it and lights the D-pad; walking away puts the D-pad back to idle")
{
	Fixture fixture;
	CHECK(fixture.world->dpad().state() == DpadState::IDLE);
	fixture.world->teleport(-8.0f, 0.0f, -1.5f);
	fixture.world->update(0.0f);
	REQUIRE(fixture.world->detector().hasTarget());
	CHECK(fixture.world->detector().getTarget()->getInfo().name == "Test Terminal");
	CHECK(fixture.world->dpad().state() == DpadState::ACTIVE);
	fixture.world->teleport(0.0f, 0.0f, -20.0f);
	fixture.world->update(0.0f);
	CHECK_FALSE(fixture.world->detector().hasTarget());
	CHECK(fixture.world->dpad().state() == DpadState::IDLE);
}

TEST_CASE("standing between the test door and crate targets whichever the camera faces")
{
	Fixture fixture;
	fixture.world->teleport(-1.75f, 0.0f, 2.3f);
	fixture.world->update(0.0f);
	CHECK(fixture.world->detector().getInRange().size() == 2);
	fixture.world->look(-11.0f);
	fixture.world->update(0.0f);
	REQUIRE(fixture.world->detector().hasTarget());
	CHECK(fixture.world->detector().getTarget()->getInfo().name == "Test Door");
	fixture.world->look(22.0f);
	fixture.world->update(0.0f);
	REQUIRE(fixture.world->detector().hasTarget());
	CHECK(fixture.world->detector().getTarget()->getInfo().name == "Test Crate");
}

TEST_CASE("at the test terminal the D-pad picks an approach and X (posted as R) runs it; away from it both pass through")
{
	Fixture fixture;
	std::vector<std::string> log;
	fixture.world->setInteractionLog([&log](const std::string& line) { log.push_back(line); });
	SEvent r;
	r.EventType = EET_KEY_INPUT_EVENT;
	r.KeyInput.Key = KEY_KEY_R;
	r.KeyInput.PressedDown = true;
	r.KeyInput.Char = 0;
	r.KeyInput.Shift = false;
	r.KeyInput.Control = false;
	CHECK_FALSE(fixture.world->onEvent(r));
	fixture.world->teleport(-8.0f, 0.0f, -1.5f);
	fixture.world->update(0.0f);
	fixture.key(KEY_DOWN, true);
	CHECK(fixture.world->onEvent(r));
	CHECK(log == std::vector<std::string>{ "FAILED: Too advanced." });
}

TEST_CASE("beside a world container the D-pad offers Normal and Force, and UP then X opens the loot screen on that container")
{
	Fixture fixture;
	std::vector<loot::Container*> opened;
	fixture.world->setOpenLoot([&opened](loot::Container& container) { opened.push_back(&container); });
	loot::Container& crate = fixture.world->containers()[0];
	fixture.world->teleport(crate.x, 0.0f, crate.z - 1.5f);
	fixture.world->update(0.0f);
	REQUIRE(fixture.world->detector().hasTarget());
	CHECK(fixture.world->detector().getTarget()->getInfo().name == crate.kind);
	CHECK(fixture.world->detector().getTarget()->getObjectType() == "supply_crate");
	fixture.key(KEY_UP, true);
	fixture.key(KEY_KEY_R, true);
	CHECK(opened == std::vector<loot::Container*>{ &crate });
	CHECK(fixture.world->interactionFeedback() == "You open the " + crate.kind + ".");
}
