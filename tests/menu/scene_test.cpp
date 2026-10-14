#include "doctest.h"
#include "MenuScene.h"
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
	CHECK(scene.count("car") >= 1);
	CHECK(scene.count("debris") >= 1);
	for (ISceneNode* node : scene.menu->scene()->getRootSceneNode()->getChildren())
	{
		std::string name = node->getName();
		if (name == "building")
			CHECK(node->getPosition().Z > 400.0f);
		if (name == "backpack")
		{
			CHECK(node->getPosition().Z < 5.0f);
			CHECK(node->getPosition().X > 0.0f);
		}
	}
}

TEST_CASE("the only light is a warm sun behind the city, over dim warm ambient light")
{
	Scene scene;
	ILightSceneNode* sun = nullptr;
	int lights = 0;
	for (ISceneNode* node : scene.menu->scene()->getRootSceneNode()->getChildren())
		if (node->getType() == ESNT_LIGHT)
		{
			++lights;
			sun = static_cast<ILightSceneNode*>(node);
		}
	REQUIRE(lights == 1);
	CHECK(sun->getLightType() == video::ELT_DIRECTIONAL);
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
