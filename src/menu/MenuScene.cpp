#include "MenuScene.h"
#include <cmath>
#include <random>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;

namespace
{
	const SColor haze(255, 84, 54, 36);
	const SColor zenith(255, 14, 13, 13);
	const SColor groundColour(255, 24, 21, 19);
	const SColor ruinColour(255, 22, 20, 19);
	const SColor propColour(255, 34, 31, 27);
	const SColor packColour(255, 52, 48, 36);
	const vector3df eye(0.0f, 1.0f, 0.0f);
	const vector3df ahead(0.0f, 3.5f, 60.0f);
	const f32 fieldOfView = 55.0f * DEGTORAD;
}

MenuScene::MenuScene(IrrlichtDevice* device)
	: device_(device), scene_(device->getSceneManager()->createNewSceneManager(false)), camera_(0)
{
	camera_ = scene_->addCameraSceneNode(0, eye, ahead);
	camera_->setFOV(fieldOfView);
	camera_->setNearValue(0.1f);
	camera_->setFarValue(4000.0f);
	camera_->setName("camera");
	addSky();
	addGround();
	addCity();
	addProps();
	addLights();
}

MenuScene::~MenuScene()
{
	scene_->drop();
}

ISceneNode* MenuScene::box(const char* name, const vector3df& position, const vector3df& size, SColor colour, const vector3df& rotation)
{
	ISceneNode* node = scene_->addCubeSceneNode(1.0f, 0, -1, position, rotation, size);
	node->setName(name);
	SMaterial& material = node->getMaterial(0);
	material.ColorMaterial = ECM_NONE;
	material.AmbientColor = colour;
	material.DiffuseColor = colour;
	material.FogEnable = true;
	material.NormalizeNormals = true;
	return node;
}

void MenuScene::addSky()
{
	IVideoDriver* driver = device_->getVideoDriver();
	IImage* image = driver->createImage(ECF_A8R8G8B8, dimension2d<u32>(4, 128));
	for (u32 y = 0; y < 128; ++y)
	{
		f32 down = y / 127.0f;
		SColor colour = haze.getInterpolated(zenith, down * down);
		for (u32 x = 0; x < 4; ++x)
			image->setPixel(x, y, colour);
	}
	ITexture* texture = driver->addTexture("menu-sky", image);
	image->drop();
	ISceneNode* sky = scene_->addSkyDomeSceneNode(texture, 16, 8, 1.0f, 1.0f, 3000.0f);
	sky->setName("sky");
}

void MenuScene::addGround()
{
	IAnimatedMesh* plane = scene_->addHillPlaneMesh("menu-ground", dimension2d<f32>(40.0f, 40.0f), dimension2d<u32>(100, 100));
	scene_->getMeshManipulator()->setVertexColors(plane->getMesh(0), groundColour);
	ISceneNode* ground = scene_->addMeshSceneNode(plane->getMesh(0));
	ground->setName("ground");
	ground->getMaterial(0).FogEnable = true;
}

void MenuScene::addCity()
{
	std::mt19937 random(1987);
	for (s32 i = 0; i < 44; ++i)
	{
		f32 x = -900.0f + i * 41.0f + (f32)(random() % 20);
		f32 z = 520.0f + (f32)(random() % 320);
		f32 width = 24.0f + (f32)(random() % 40);
		f32 height = 30.0f + (f32)(random() % 150);
		f32 lean = (random() % 4 == 0) ? (f32)((s32)(random() % 13) - 6) : 0.0f;
		box("building", vector3df(x, height * 0.5f, z), vector3df(width, height, 20.0f + (f32)(random() % 30)), ruinColour,
			vector3df(0.0f, (f32)(random() % 20), lean));
	}
}

void MenuScene::addProps()
{
	box("backpack", vector3df(0.95f, 0.2f, 3.1f), vector3df(0.34f, 0.42f, 0.22f), packColour, vector3df(0.0f, -32.0f, 6.0f));
	box("backpack", vector3df(0.93f, 0.44f, 3.09f), vector3df(0.3f, 0.08f, 0.2f), packColour, vector3df(0.0f, -32.0f, 6.0f));
	box("backpack", vector3df(0.83f, 0.12f, 2.98f), vector3df(0.22f, 0.18f, 0.08f), packColour, vector3df(0.0f, -32.0f, 4.0f));

	box("car", vector3df(-6.5f, 0.75f, 24.0f), vector3df(4.4f, 1.4f, 1.9f), propColour, vector3df(0.0f, 28.0f, 168.0f));
	box("car", vector3df(-6.7f, 1.55f, 24.1f), vector3df(2.4f, 0.7f, 1.7f), propColour, vector3df(0.0f, 28.0f, 168.0f));
	for (s32 i = 0; i < 6; ++i)
		box("debris", vector3df(5.0f + (i % 3) * 0.9f, 0.25f + (i / 3) * 0.3f, 15.0f + (i % 2) * 0.8f),
			vector3df(1.2f - i * 0.1f, 0.5f, 0.9f), propColour, vector3df((f32)(i * 17), (f32)(i * 41), (f32)(i * 13)));
	box("curb", vector3df(-9.0f, 0.1f, 200.0f), vector3df(0.4f, 0.2f, 400.0f), propColour);
	box("curb", vector3df(9.0f, 0.1f, 200.0f), vector3df(0.4f, 0.2f, 400.0f), propColour);
}

void MenuScene::addLights()
{
	scene_->setAmbientLight(SColorf(0.16f, 0.12f, 0.09f));
	ILightSceneNode* sun = scene_->addLightSceneNode(0, vector3df(0.0f, 0.0f, 0.0f), SColorf(0.85f, 0.5f, 0.28f));
	sun->setLightType(ELT_DIRECTIONAL);
	sun->setRotation(vector3df(14.0f, 180.0f, 0.0f));
	sun->setName("sun");
}

void MenuScene::draw()
{
	IVideoDriver* driver = device_->getVideoDriver();
	dimension2d<u32> screen = driver->getScreenSize();
	f32 seconds = device_->getTimer()->getTime() / 1000.0f;
	f32 yaw = 2.5f * DEGTORAD * std::sin(seconds * 2.0f * PI / 120.0f);
	camera_->setTarget(eye + vector3df(ahead.X * std::cos(yaw) + ahead.Z * std::sin(yaw), ahead.Y - eye.Y,
		ahead.Z * std::cos(yaw) - ahead.X * std::sin(yaw)));
	camera_->setAspectRatio((f32)screen.Width / (f32)screen.Height);
	driver->setFog(haze, EFT_FOG_LINEAR, 40.0f, 1100.0f, 0.01f, true, false);
	scene_->drawAll();
}
