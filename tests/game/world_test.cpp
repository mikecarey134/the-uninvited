#include "doctest.h"
#include "World.h"
#include <cmath>
#include <string>

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
