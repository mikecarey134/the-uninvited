#include "World.h"
#include <algorithm>
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
	const f32 radius = 0.35f;
	const f32 edge = groundSize / 2.0f - radius;
	const f32 longestStep = 0.1f;
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
	addProps();
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

void World::addProps()
{
	addProp("prop barrel", "assets/models/props/barrel/barrel.obj", -4.0f, 7.0f, vector3df(), 0.9f);
	addProp("prop crate", "assets/models/props/crate/crate.obj", 4.0f, 9.0f, vector3df(0.0f, 20.0f, 0.0f), 0.6f);
	addProp("prop chest", "assets/models/props/chest/chest.obj", 7.0f, -5.0f, vector3df(0.0f, 60.0f, 0.0f), 0.8f);
	addProp("prop log", "assets/models/props/log/log.obj", -7.0f, -6.0f, vector3df(0.0f, 30.0f, 90.0f), 1.6f);
	addProp("prop car", "assets/models/props/car/car.obj", 10.0f, 20.0f, vector3df(0.0f, 35.0f, 0.0f), 4.5f);
	addProp("prop car2", "assets/models/props/car2/car2.obj", -16.0f, 8.0f, vector3df(0.0f, -70.0f, 0.0f), 4.5f);
	addProp("prop crate", "assets/models/props/crate/crate.obj", -12.0f, -14.0f, vector3df(0.0f, 45.0f, 0.0f), 0.6f);
	addProp("prop barrel", "assets/models/props/barrel/barrel.obj", 15.0f, -12.0f, vector3df(), 0.9f);
	addProp("prop car", "assets/models/props/car/car.obj", -7.0f, 24.0f, vector3df(0.0f, 100.0f, 0.0f), 4.5f);
	addProp("prop chest", "assets/models/props/chest/chest.obj", 20.0f, 4.0f, vector3df(0.0f, 10.0f, 0.0f), 0.8f);
}

void World::addProp(const char* name, const char* file, f32 x, f32 z, const vector3df& rotation, f32 length)
{
	IAnimatedMesh* mesh = scene_->getMesh(file);
	if (!mesh)
		return;
	vector3df size = mesh->getBoundingBox().getExtent();
	f32 scale = length / std::max(size.X, std::max(size.Y, size.Z));
	ISceneNode* node = scene_->addMeshSceneNode(mesh->getMesh(0), 0, -1, vector3df(x, 0.0f, z), rotation, vector3df(scale, scale, scale));
	node->setName(name);
	node->updateAbsolutePosition();
	node->setPosition(vector3df(x, -node->getTransformedBoundingBox().MinEdge.Y, z));
	node->updateAbsolutePosition();
	obstacles_.push_back(node->getTransformedBoundingBox());
	for (u32 i = 0; i < node->getMaterialCount(); ++i)
	{
		SMaterial& material = node->getMaterial(i);
		SColor colour = material.DiffuseColor;
		if (material.getTexture(0))
			material.AmbientColor = SColor(255, 255, 255, 255);
		else
		{
			material.ColorMaterial = ECM_NONE;
			material.AmbientColor = colour;
			material.EmissiveColor = SColor(255, colour.getRed() * 3 / 10, colour.getGreen() * 3 / 10, colour.getBlue() * 3 / 10);
		}
		material.FogEnable = true;
		material.NormalizeNormals = true;
	}
}

bool World::blocked(const vector3df& at) const
{
	for (const aabbox3df& box : obstacles_)
	{
		f32 dx = std::max(std::max(box.MinEdge.X - at.X, 0.0f), at.X - box.MaxEdge.X);
		f32 dz = std::max(std::max(box.MinEdge.Z - at.Z, 0.0f), at.Z - box.MaxEdge.Z);
		if (dx * dx + dz * dz < radius * radius)
			return true;
	}
	return false;
}

void World::move(const vector3df& step)
{
	s32 steps = std::max(1, (s32)std::ceil(step.getLength() / longestStep));
	vector3df at = player_->getPosition();
	for (s32 i = 0; i < steps; ++i)
	{
		vector3df across(clamp(at.X + step.X / steps, -edge, edge), at.Y, at.Z);
		if (!blocked(across))
			at = across;
		vector3df along(at.X, at.Y, clamp(at.Z + step.Z / steps, -edge, edge));
		if (!blocked(along))
			at = along;
	}
	player_->setPosition(at);
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
			move(step * walkSpeed * seconds);
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

f32 World::playerRadius() const
{
	return radius;
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
