#include "World.h"
#include "Sky.h"
#include "AssimpMeshLoader.h"
#include <algorithm>
#include <cmath>
#include <string>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;

namespace
{
	const f32 groundSize = 200.0f;
	const f32 groundTile = 4.0f;
	const f32 fogStart = 20.0f;
	const f32 fogEnd = 100.0f;
	const f32 characterHeight = 1.75f;
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

	void loop(IAnimatedMeshSceneNode* node, bool walking)
	{
		if (walking && (s32)node->getMesh()->getFrameCount() > walkEnd)
		{
			node->setFrameLoop(walkStart, walkEnd);
			node->setAnimationSpeed(15.0f);
		}
		else
		{
			node->setFrameLoop(idleStart, idleEnd);
			node->setAnimationSpeed(12.0f);
		}
	}
}

const uint8_t World::IDLE_STATE;
const uint8_t World::WALKING_STATE;

World::World(IrrlichtDevice* device, const char* model, const char* skin, std::function<void()> leave,
	const character::CharacterSheet& sheet)
	: device_(device), scene_(device->getSceneManager()->createNewSceneManager(false)), camera_(0), player_(0), leave_(leave),
	walking_(false), yaw_(0.0f), feet_(0.0f), sheet_(sheet), unticked_(0.0f), compression_(1.0f), timeOfDay_(START_HOUR), speed_(3.5f), containers_(loot::flatWorldContainers()), debugShown_(false), sun_(0), clock_(0.0f)
{
	addAssimpMeshLoader(scene_);
	for (bool& held : held_)
		held = false;
	camera_ = scene_->addCameraSceneNode();
	camera_->setNearValue(0.1f);
	camera_->setFarValue(4000.0f);
	camera_->setFOV(55.0f * DEGTORAD);
	addGround();
	addProps();
	addTestInteractables();
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
		}
		material.FogEnable = true;
		material.NormalizeNormals = true;
	}
}

// TEMPORARY (SCRUM-1001): coloured boxes standing in for real interactables until the object system exists.
struct TestInteractable : public Interactable
{
	InteractableInfo info;
	vector3df position;

	InteractableInfo getInfo() const override { return info; }
	vector3df getPosition() const override { return position; }
};

// TEMPORARY (SCRUM-1001): a door and a crate side by side to try aiming between two, and a lone terminal for auto-targeting.
void World::addTestInteractables()
{
	struct Spec
	{
		const char* name;
		const char* icon;
		vector3df at;
		vector3df size;
		SColor colour;
	};
	const Spec specs[] = {
		{ "Test Door", "interactables/locked-door.png", vector3df(-2.5f, 0.0f, 4.0f), vector3df(1.0f, 2.0f, 0.2f), SColor(255, 110, 72, 40) },
		{ "Test Crate", "interactables/supply-crate.png", vector3df(-1.0f, 0.0f, 4.0f), vector3df(0.7f, 0.7f, 0.7f), SColor(255, 150, 118, 60) },
		{ "Test Terminal", "interactables/terminal.png", vector3df(-8.0f, 0.0f, 0.0f), vector3df(0.6f, 1.2f, 0.4f), SColor(255, 60, 90, 110) } };
	for (const Spec& spec : specs)
	{
		std::unique_ptr<TestInteractable> object(new TestInteractable());
		object->info.name = spec.name;
		object->info.iconPath = spec.icon;
		object->position = spec.at + vector3df(0.0f, spec.size.Y / 2.0f, 0.0f);
		IMesh* mesh = scene_->getGeometryCreator()->createCubeMesh(spec.size);
		scene_->getMeshManipulator()->setVertexColors(mesh, spec.colour);
		ISceneNode* node = scene_->addMeshSceneNode(mesh, 0, -1, object->position);
		mesh->drop();
		node->setName("test interactable");
		node->setMaterialFlag(EMF_LIGHTING, false);
		node->setMaterialFlag(EMF_FOG_ENABLE, true);
		detector_.addInteractable(object.get());
		testInteractables_.push_back(std::move(object));
	}
}

void World::detect()
{
	if (player_)
		detector_.update(player_->getPosition(), camera_->getTarget() - camera_->getPosition(), camera_->getPosition());
	Interactable* target = dead() ? nullptr : detector_.getTarget();
	if (target == shownTarget_)
		return;
	shownTarget_ = target;
	if (target)
	{
		InteractableInfo info = target->getInfo();
		dpad_.setTarget(info.name, info.iconPath);
		dpad_.setState(DpadState::ACTIVE);
	}
	else
		dpad_.setState(DpadState::IDLE);
}

bool World::targetMarker(position2d<s32>& at) const
{
	if (!shownTarget_)
		return false;
	at = scene_->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(shownTarget_->getPosition() + vector3df(0.0f, 1.2f, 0.0f), camera_);
	return at.X != -10000;
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

void World::wield(const char* model)
{
	if (!player_)
		return;
	IAnimatedMesh* mesh = scene_->getMesh(model);
	s32 start = player_->getStartFrame();
	s32 end = player_->getEndFrame();
	f32 frame = player_->getFrameNr();
	ITexture* skin = player_->getMaterial(0).getTexture(0);
	player_->setMesh(mesh);
	player_->setFrameLoop(start, end);
	player_->setCurrentFrame(frame);
	player_->setMaterialTexture(0, skin);
	player_->setMaterialFlag(EMF_FOG_ENABLE, true);
	player_->setMaterialFlag(EMF_NORMALIZE_NORMALS, true);
}

void World::addPlayer(const char* model, const char* skin)
{
	player_ = addCharacter(model, skin, feet_);
	if (player_)
		player_->setName("player");
}

IAnimatedMeshSceneNode* World::addCharacter(const char* model, const char* skin, f32& feet)
{
	IAnimatedMesh* mesh = scene_->getMesh(model);
	if (!mesh)
		return 0;
	f32 scale = characterHeight / mesh->getBoundingBox().getExtent().Y;
	feet = -mesh->getMesh(idleStart)->getBoundingBox().MinEdge.Y * scale;
	IAnimatedMeshSceneNode* node = scene_->addAnimatedMeshSceneNode(mesh, 0, -1, vector3df(0.0f, feet, 0.0f), vector3df(), vector3df(scale, scale, scale));
	if (ITexture* texture = device_->getVideoDriver()->getTexture(skin))
		node->setMaterialTexture(0, texture);
	node->setMaterialFlag(EMF_FOG_ENABLE, true);
	node->setMaterialFlag(EMF_NORMALIZE_NORMALS, true);
	node->setLoopMode(true);
	node->setFrameLoop(idleStart, idleEnd);
	node->setAnimationSpeed(12.0f);
	return node;
}

void World::addLights()
{
	sun_ = scene_->addLightSceneNode(0, vector3df(), SColorf(0.0f, 0.0f, 0.0f));
	sun_->setLightType(ELT_DIRECTIONAL);
	setTimeOfDay(timeOfDay_);
}

void World::setTimeOfDay(f32 hours)
{
	timeOfDay_ = std::fmod(hours, 24.0f);
	if (timeOfDay_ < 0.0f)
		timeOfDay_ += 24.0f;
	SkyLook look = skyAt(timeOfDay_);
	zenith_ = look.zenith;
	horizon_ = look.horizon;
	scene_->setAmbientLight(look.ambient);
	if (sun_)
	{
		sun_->getLightData().DiffuseColor = look.sun;
		sun_->getLightData().SpecularColor = look.sun;
		sun_->setRotation(look.sunDirection.getHorizontalAngle());
	}
}

loot::Container* World::containerInReach()
{
	if (!player_)
		return nullptr;
	int index = loot::closest(containers_, player_->getPosition().X, player_->getPosition().Z);
	return index < 0 ? nullptr : &containers_[index];
}

void World::teleport(f32 x, f32 y, f32 z)
{
	if (player_)
		player_->setPosition(vector3df(x, y + feet_, z));
	follow();
}

void World::advanceClock(f32 seconds)
{
	setTimeOfDay(timeOfDay_ + seconds * compression_ / 3600.0f);
}

bool World::onEvent(const SEvent& event)
{
	if (event.EventType != EET_KEY_INPUT_EVENT)
		return false;
	if (event.KeyInput.Key == KEY_F3 && event.KeyInput.PressedDown)
	{
		debugShown_ = !debugShown_;
		return true;
	}
	// TEMPORARY (SCRUM-1000): F9 cycles the D-pad HUD through its states with dummy data until detection (SCRUM-1001) drives it.
	if (event.KeyInput.Key == KEY_F9 && event.KeyInput.PressedDown)
	{
		cycleDpadDemo();
		return true;
	}
	if (dead())
		return event.KeyInput.Key == KEY_ESCAPE;
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

// TEMPORARY (SCRUM-1000): dummy data for the F9 D-pad demo; remove once SCRUM-1001 and SCRUM-1003 drive the widget.
void World::cycleDpadDemo()
{
	dpadDemo_ = (dpadDemo_ + 1) % 4;
	dpad_.clearSelection();
	dpad_.setActionPrompt("");
	if (dpadDemo_ == 0)
	{
		dpad_.setState(DpadState::IDLE);
		return;
	}
	dpad_.setTarget("Locked Door", "interactables/locked-door.png");
	dpad_.setSlot(DpadDirection::UP, "Pick", "skills/lockpicking.png");
	dpad_.setSlot(DpadDirection::RIGHT, "Force", "approach/force.png");
	dpad_.setSlot(DpadDirection::DOWN, "Talk", "skills/speech.png");
	dpad_.setSlot(DpadDirection::LEFT, "Guile", "modes/guile.png");
	dpad_.setState(DpadState::ACTIVE);
	if (dpadDemo_ == 2)
	{
		dpad_.clearSlot(DpadDirection::DOWN);
		dpad_.setSelectedDirection(DpadDirection::UP);
		dpad_.setActionPrompt("Pick Lock");
	}
	if (dpadDemo_ == 3)
	{
		dpad_.setTarget("Supply Crate", "interactables/supply-crate.png");
		dpad_.setState(DpadState::MULTI_OBJECT);
	}
}

void World::look(f32 degrees)
{
	yaw_ += degrees;
}

void World::update(f32 seconds)
{
	dpad_.update(seconds);
	unticked_ += seconds * compression_;
	while (unticked_ >= 1.0f && !dead())
	{
		sheet_.survivalTick(1.0);
		unticked_ -= 1.0f;
	}
	if (dead())
	{
		if (player_)
			player_->setAnimationSpeed(0.0f);
		detect();
		return;
	}
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
			move(step * speed_ * seconds);
			player_->setRotation(vector3df(0.0f, step.getHorizontalAngle().Y, 0.0f));
		}
		animate(walking);
	}
	follow();
	detect();
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
	loop(player_, walking);
}

protocol::State World::localState() const
{
	protocol::State state{ 0, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, IDLE_STATE };
	if (player_)
	{
		state.pos[0] = player_->getPosition().X;
		state.pos[1] = player_->getPosition().Y - feet_;
		state.pos[2] = player_->getPosition().Z;
		state.rot[1] = player_->getRotation().Y;
	}
	if (dead())
		state.state = protocol::STATE_DEAD;
	else if (walking_)
		state.state = WALKING_STATE;
	return state;
}

void World::addRemote(uint16_t id, const char* model, const char* skin)
{
	if (remotes_.count(id))
		return;
	Remote remote;
	remote.node = addCharacter(model, skin, remote.feet);
	if (!remote.node)
		return;
	remote.node->setName(("remote " + std::to_string(id)).c_str());
	remote.walking = false;
	remote.animatedWalking = false;
	remotes_[id] = remote;
}

void World::placeRemote(uint16_t id, uint16_t seq, const f32 position[3], f32 yaw, bool walking)
{
	auto found = remotes_.find(id);
	if (found == remotes_.end())
		return;
	found->second.path.addSnapshot(seq, position, yaw, clock_);
	found->second.walking = walking;
}

void World::removeRemote(uint16_t id)
{
	auto found = remotes_.find(id);
	if (found == remotes_.end())
		return;
	found->second.node->remove();
	remotes_.erase(found);
}

void World::updateRemotes(f32 seconds)
{
	clock_ += seconds;
	for (auto& entry : remotes_)
	{
		Remote& remote = entry.second;
		Pose pose;
		if (remote.path.sample(clock_ - Interpolator::DELAY, pose))
		{
			remote.node->setPosition(vector3df(pose.position[0], pose.position[1] + remote.feet, pose.position[2]));
			remote.node->setRotation(vector3df(0.0f, pose.yaw, 0.0f));
		}
		if (remote.walking != remote.animatedWalking)
		{
			remote.animatedWalking = remote.walking;
			loop(remote.node, remote.walking);
		}
	}
}

IAnimatedMeshSceneNode* World::remote(uint16_t id) const
{
	auto found = remotes_.find(id);
	return found == remotes_.end() ? 0 : found->second.node;
}

void World::draw()
{
	IVideoDriver* driver = device_->getVideoDriver();
	dimension2d<u32> screen = driver->getScreenSize();
	camera_->setAspectRatio((f32)screen.Width / (f32)screen.Height);
	driver->draw2DRectangle(rect<s32>(0, 0, (s32)screen.Width, (s32)screen.Height), zenith_, zenith_, horizon_, horizon_);
	driver->setFog(horizon_, EFT_FOG_LINEAR, fogStart, fogEnd, 0.01f, true, false);
	scene_->drawAll();
}
