#include "doctest.h"
#include "MenuScene.h"
#include <algorithm>
#include <cmath>
#include <string>

using namespace irr;
using namespace core;
using namespace scene;

namespace
{
	struct Scene
	{
		Scene() : device(createDevice(video::EDT_NULL, dimension2d<u32>(1280, 720))), menu(new MenuScene(device)) {}
		~Scene()
		{
			delete menu;
			device->drop();
		}

		int count(const char* name) const
		{
			int found = 0;
			for (ISceneNode* node : menu->scene()->getRootSceneNode()->getChildren())
				if (std::string(node->getName()) == name)
					++found;
			return found;
		}

		IrrlichtDevice* device;
		MenuScene* menu;
	};
}

TEST_CASE("the menu camera stands at street level looking slightly up toward the city with a cinematic lens")
{
	Scene scene;
	ICameraSceneNode* camera = scene.menu->camera();
	CHECK(camera->getPosition().Y == doctest::Approx(1.0f));
	vector3df look = camera->getTarget() - camera->getPosition();
	CHECK(look.Z > 0.0f);
	CHECK(look.Y > 0.0f);
	CHECK(look.Y / look.Z < 0.1f);
	f32 degrees = camera->getFOV() * RADTODEG;
	CHECK(degrees >= 50.0f);
	CHECK(degrees <= 60.0f);
}

TEST_CASE("the scene has ground, sky, a distant skyline, a backpack close by, and street props")
{
	Scene scene;
	CHECK(scene.count("ground") == 1);
	CHECK(scene.count("sky") == 1);
	CHECK(scene.count("building") >= 30);
	CHECK(scene.count("backpack") >= 1);
	CHECK(scene.count("car") >= 2);
	CHECK(scene.count("debris") >= 1);
	for (ISceneNode* node : scene.menu->scene()->getRootSceneNode()->getChildren())
	{
		std::string name = node->getName();
		if (name == "building")
			CHECK(node->getPosition().Z > 400.0f);
		if (name == "backpack")
		{
			CHECK(node->getPosition().X < 0.0f);
			f32 nearest = 1000.0f;
			for (ISceneNode* other : scene.menu->scene()->getRootSceneNode()->getChildren())
				if (std::string(other->getName()) == "car")
					nearest = std::min(nearest, other->getPosition().getDistanceFrom(node->getPosition()));
			CHECK(nearest < 3.0f);
		}
	}
}

TEST_CASE("a warm sun backlights the city, and a faint warm fill near the camera fades out long before the skyline")
{
	Scene scene;
	ILightSceneNode* sun = nullptr;
	ILightSceneNode* fill = nullptr;
	int lights = 0;
	for (ISceneNode* node : scene.menu->scene()->getRootSceneNode()->getChildren())
		if (node->getType() == ESNT_LIGHT)
		{
			++lights;
			ILightSceneNode* light = static_cast<ILightSceneNode*>(node);
			if (light->getLightType() == video::ELT_DIRECTIONAL)
				sun = light;
			else
				fill = light;
		}
	REQUIRE(lights == 2);
	REQUIRE(sun != nullptr);
	REQUIRE(fill != nullptr);
	scene.menu->draw();
	vector3df direction = sun->getLightData().Direction;
	CHECK(direction.Z < 0.0f);
	CHECK(direction.Y < 0.0f);
	video::SColorf colour = sun->getLightData().DiffuseColor;
	CHECK(colour.r > colour.g);
	CHECK(colour.g > colour.b);
	video::SColorf ambient = scene.menu->scene()->getAmbientLight();
	CHECK(ambient.r < 0.25f);
	CHECK(ambient.r > ambient.b);

	CHECK(fill->getLightType() == video::ELT_POINT);
	CHECK(fill->getPosition().Z < scene.menu->camera()->getPosition().Z);
	video::SColorf warm = fill->getLightData().DiffuseColor;
	CHECK(warm.r > warm.b);
	vector3df falloff = fill->getLightData().Attenuation;
	auto strength = [&falloff](f32 distance) { return 1.0f / (falloff.X + falloff.Y * distance + falloff.Z * distance * distance); };
	CHECK(strength(5.0f) > 0.6f);
	CHECK(strength(500.0f) < 0.01f);
}

TEST_CASE("the menu scene is its own scene manager, so the character preview never shows the city")
{
	Scene scene;
	CHECK(scene.menu->scene() != scene.device->getSceneManager());
	CHECK(scene.device->getSceneManager()->getRootSceneNode()->getChildren().empty());
}

TEST_CASE("the camera drifts only a few degrees over time")
{
	Scene scene;
	vector3df start = scene.menu->camera()->getTarget() - scene.menu->camera()->getPosition();
	ITimer* timer = scene.device->getTimer();
	f32 widest = 0.0f;
	for (u32 second = 0; second <= 120; second += 5)
	{
		timer->setTime(second * 1000);
		scene.menu->draw();
		vector3df now = scene.menu->camera()->getTarget() - scene.menu->camera()->getPosition();
		f32 angle = std::acos(now.normalize().dotProduct(vector3df(start).normalize())) * RADTODEG;
		widest = std::max(widest, angle);
	}
	CHECK(widest > 0.5f);
	CHECK(widest < 5.0f);
}

namespace
{
	const char* const model = ASSETS_DIR "/models/stick_mike.ms3d";

	position2d<s32> onScreen(Scene& scene, const vector3df& point)
	{
		return scene.menu->scene()->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(point, scene.menu->camera());
	}
}

TEST_CASE("a shown character stands on the street in the right half, whole from head to feet, facing the camera")
{
	Scene scene;
	scene.device->getTimer()->setTime(0);
	scene.menu->showCharacter(model);
	IAnimatedMeshSceneNode* character = scene.menu->character();
	REQUIRE(character != nullptr);
	scene.menu->draw();
	character->updateAbsolutePosition();
	aabbox3df body = character->getMesh()->getMesh(206)->getBoundingBox();
	character->getAbsoluteTransformation().transformBoxEx(body);
	CHECK(body.MinEdge.Y == doctest::Approx(0.0f).epsilon(0.02));
	CHECK(body.getExtent().Y > 1.5f);
	CHECK(body.getExtent().Y < 2.0f);
	position2d<s32> head = onScreen(scene, vector3df(body.getCenter().X, body.MaxEdge.Y, body.getCenter().Z));
	position2d<s32> feet = onScreen(scene, vector3df(body.getCenter().X, body.MinEdge.Y, body.getCenter().Z));
	CHECK(head.X > 640);
	CHECK(head.X < 1216);
	CHECK(head.Y > 40);
	CHECK(feet.Y < 700);
	CHECK(feet.Y - head.Y > 360);
	CHECK(character->getPosition().Z < 3.5f);
	CHECK(character->getRotation().Y == doctest::Approx(180.0f));
}

TEST_CASE("the character breathes on the idle loop and is lit by the scene's own lights and fog")
{
	Scene scene;
	scene.menu->showCharacter(model);
	IAnimatedMeshSceneNode* character = scene.menu->character();
	REQUIRE(character != nullptr);
	CHECK(character->getStartFrame() == 206);
	CHECK(character->getEndFrame() == 250);
	CHECK(character->getLoopMode());
	CHECK(character->getMaterial(0).Lighting);
	CHECK(character->getMaterial(0).FogEnable);
	int lights = 0;
	for (ISceneNode* node : scene.menu->scene()->getRootSceneNode()->getChildren())
		if (node->getType() == ESNT_LIGHT)
			++lights;
	CHECK(lights == 2);
	CHECK(scene.device->getSceneManager()->getRootSceneNode()->getChildren().empty());
}

TEST_CASE("turning eases toward the dragged angle and stays where it was left")
{
	Scene scene;
	ITimer* timer = scene.device->getTimer();
	timer->setTime(1000);
	scene.menu->showCharacter(model);
	scene.menu->draw();
	scene.menu->turnCharacter(90.0f);
	timer->setTime(1016);
	scene.menu->draw();
	CHECK(scene.menu->characterYaw() > 0.0f);
	CHECK(scene.menu->characterYaw() < 45.0f);
	timer->setTime(3000);
	scene.menu->draw();
	CHECK(scene.menu->characterYaw() == doctest::Approx(90.0f).epsilon(0.01));
	timer->setTime(9000);
	scene.menu->draw();
	CHECK(scene.menu->characterYaw() == doctest::Approx(90.0f).epsilon(0.01));
	CHECK(scene.menu->character()->getRotation().Y == doctest::Approx(270.0f).epsilon(0.01));
}

TEST_CASE("the nameplate point sits just above the character's head")
{
	Scene scene;
	position2d<s32> at;
	CHECK_FALSE(scene.menu->characterTop(at));
	scene.menu->showCharacter(model);
	scene.menu->draw();
	REQUIRE(scene.menu->characterTop(at));
	IAnimatedMeshSceneNode* character = scene.menu->character();
	aabbox3df body = character->getMesh()->getMesh(206)->getBoundingBox();
	character->getAbsoluteTransformation().transformBoxEx(body);
	position2d<s32> head = onScreen(scene, vector3df(body.getCenter().X, body.MaxEdge.Y, body.getCenter().Z));
	CHECK(at.Y < head.Y);
	CHECK(head.Y - at.Y < 40);
	CHECK(std::abs(at.X - head.X) < 4);
}

TEST_CASE("showing twice keeps one character, and hiding removes it")
{
	Scene scene;
	scene.menu->showCharacter(model);
	scene.menu->showCharacter(model);
	CHECK(scene.count("character") == 1);
	scene.menu->hideCharacter();
	CHECK(scene.count("character") == 0);
	CHECK(scene.menu->character() == nullptr);
}

TEST_CASE("Mike's OBJ props lie on the street at real-world sizes, textured and lit like the rest of the scene")
{
	Scene scene;
	struct Prop { const char* name; f32 shortest; f32 longest; };
	const Prop props[] = { { "crate", 0.4f, 0.9f }, { "barrel", 0.7f, 1.0f }, { "chest", 0.5f, 1.0f }, { "log", 1.2f, 2.0f },
		{ "axe", 0.6f, 0.9f }, { "flint axe", 0.4f, 0.7f }, { "flint knife", 0.2f, 0.4f } };
	for (const Prop& prop : props)
	{
		INFO(prop.name);
		ISceneNode* node = scene.menu->scene()->getSceneNodeFromName(prop.name);
		REQUIRE(node != nullptr);
		node->updateAbsolutePosition();
		aabbox3df box = node->getTransformedBoundingBox();
		CHECK(box.MinEdge.Y == doctest::Approx(0.0f).epsilon(0.02));
		vector3df size = box.getExtent();
		f32 longest = std::max(size.X, std::max(size.Y, size.Z));
		CHECK(longest >= prop.shortest);
		CHECK(longest <= prop.longest);
		CHECK(box.getCenter().Z > 4.0f);
		CHECK(box.getCenter().Z < 30.0f);
		for (u32 i = 0; i < node->getMaterialCount(); ++i)
		{
			const video::SMaterial& material = node->getMaterial(i);
			CHECK(material.getTexture(0) != nullptr);
			CHECK(material.Lighting);
			CHECK(material.AmbientColor.getRed() > 200);
			CHECK(material.FogEnable);
			CHECK(material.NormalizeNormals);
		}
	}
}
