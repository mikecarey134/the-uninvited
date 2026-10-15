#include "World.h"
#include <cmath>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;

namespace
{
	const SColor haze(255, 84, 54, 36);
	const f32 groundSize = 200.0f;
	const f32 groundTile = 4.0f;
	const f32 fogStart = 20.0f;
	const f32 fogEnd = 100.0f;
	const f32 characterHeight = 1.75f;
	const f32 walkSpeed = 3.5f;
	const f32 followDistance = 4.5f;
	const f32 eyeHeight = 2.6f;
	const f32 lookHeight = 1.4f;
	const s32 idleStart = 206;
	const s32 idleEnd = 250;
	const s32 walkStart = 301;
	const s32 walkEnd = 316;
	const EKEY_CODE moveKeys[4] = { KEY_KEY_W, KEY_KEY_S, KEY_KEY_A, KEY_KEY_D };
}

World::World(IrrlichtDevice* device, const char* model, const char* skin, std::function<void()> leave)
	: device_(device), scene_(device->getSceneManager()->createNewSceneManager(false)), camera_(0), player_(0), leave_(leave),
	walking_(false), yaw_(0.0f), feet_(0.0f)
{
	for (bool& held : held_)
		held = false;
	if (ITexture* sky = device->getVideoDriver()->getTexture("menu-sky"))
		scene_->addSkyDomeSceneNode(sky, 16, 8, 1.0f, 1.0f, 3000.0f)->setName("sky");
	camera_ = scene_->addCameraSceneNode();
	camera_->setNearValue(0.1f);
	camera_->setFarValue(4000.0f);
	camera_->setFOV(55.0f * DEGTORAD);
	addGround();
	addPlayer(model, skin);
	addLights();
	follow();
}

World::~World()
{
	scene_->drop();
}

void World::addGround()
{
	IMesh* plane = scene_->getGeometryCreator()->createPlaneMesh(dimension2d<f32>(groundSize / 20.0f, groundSize / 20.0f),
		dimension2d<u32>(20, 20), 0, dimension2df(groundSize / groundTile, groundSize / groundTile));
	ISceneNode* ground = scene_->addMeshSceneNode(plane);
	plane->drop();
	ground->setName("ground");
	ground->setMaterialTexture(0, device_->getVideoDriver()->getTexture("assets/textures/ground_brown.jpg"));
	ground->getMaterial(0).FogEnable = true;
}

void World::addPlayer(const char* model, const char* skin)
{
	IAnimatedMesh* mesh = scene_->getMesh(model);
	if (!mesh)
		return;
	f32 scale = characterHeight / mesh->getBoundingBox().getExtent().Y;
	feet_ = -mesh->getMesh(idleStart)->getBoundingBox().MinEdge.Y * scale;
	player_ = scene_->addAnimatedMeshSceneNode(mesh, 0, -1, vector3df(0.0f, feet_, 0.0f), vector3df(), vector3df(scale, scale, scale));
	player_->setName("player");
	if (ITexture* texture = device_->getVideoDriver()->getTexture(skin))
		player_->setMaterialTexture(0, texture);
	player_->setMaterialFlag(EMF_FOG_ENABLE, true);
	player_->setMaterialFlag(EMF_NORMALIZE_NORMALS, true);
	player_->setLoopMode(true);
	player_->setFrameLoop(idleStart, idleEnd);
	player_->setAnimationSpeed(12.0f);
}

void World::addLights()
{
	scene_->setAmbientLight(SColorf(0.35f, 0.3f, 0.26f));
	ILightSceneNode* sun = scene_->addLightSceneNode(0, vector3df(), SColorf(0.85f, 0.7f, 0.55f));
	sun->setLightType(ELT_DIRECTIONAL);
	sun->setRotation(vector3df(50.0f, 30.0f, 0.0f));
}

bool World::onEvent(const SEvent& event)
{
	if (event.EventType != EET_KEY_INPUT_EVENT)
		return false;
	if (event.KeyInput.Key == KEY_ESCAPE && event.KeyInput.PressedDown)
	{
		leave_();
		return true;
	}
	for (int i = 0; i < 4; ++i)
		if (event.KeyInput.Key == moveKeys[i])
			held_[i] = event.KeyInput.PressedDown;
	return false;
}

void World::look(f32 degrees)
{
	yaw_ += degrees;
}

void World::update(f32 seconds)
{
	if (player_)
	{
		f32 radians = yaw_ * DEGTORAD;
		vector3df forward(std::sin(radians), 0.0f, std::cos(radians));
		vector3df right(std::cos(radians), 0.0f, -std::sin(radians));
		vector3df step = forward * (f32)(held_[0] - held_[1]) + right * (f32)(held_[3] - held_[2]);
		bool walking = step.getLengthSQ() > 0.0f;
		if (walking)
		{
			step.normalize();
			player_->setPosition(player_->getPosition() + step * walkSpeed * seconds);
			player_->setRotation(vector3df(0.0f, step.getHorizontalAngle().Y, 0.0f));
		}
		animate(walking);
	}
	follow();
}

f32 World::cameraDistance() const
{
	return followDistance;
}

void World::follow()
{
	vector3df at = player_ ? player_->getPosition() - vector3df(0.0f, feet_, 0.0f) : vector3df();
	f32 radians = yaw_ * DEGTORAD;
	vector3df forward(std::sin(radians), 0.0f, std::cos(radians));
	camera_->setPosition(at - forward * followDistance + vector3df(0.0f, eyeHeight, 0.0f));
	camera_->setTarget(at + vector3df(0.0f, lookHeight, 0.0f));
}

void World::animate(bool walking)
{
	if (!player_ || walking == walking_)
		return;
	walking_ = walking;
	bool canWalk = (s32)player_->getMesh()->getFrameCount() > walkEnd;
	if (walking && canWalk)
	{
		player_->setFrameLoop(walkStart, walkEnd);
		player_->setAnimationSpeed(15.0f);
	}
	else
	{
		player_->setFrameLoop(idleStart, idleEnd);
		player_->setAnimationSpeed(12.0f);
	}
}

void World::draw()
{
	IVideoDriver* driver = device_->getVideoDriver();
	dimension2d<u32> screen = driver->getScreenSize();
	camera_->setAspectRatio((f32)screen.Width / (f32)screen.Height);
	driver->draw2DRectangle(haze, rect<s32>(0, 0, (s32)screen.Width, (s32)screen.Height));
	driver->setFog(haze, EFT_FOG_LINEAR, fogStart, fogEnd, 0.01f, true, false);
	scene_->drawAll();
}
