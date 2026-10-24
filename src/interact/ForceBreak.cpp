#include "ForceBreak.h"
#include <algorithm>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;

const f32 ForceBreak::PUFF_SECONDS = 0.08f;
const f32 ForceBreak::BURST_SECONDS = 1.6f;

ForceBreak::~ForceBreak()
{
	for (Burst& burst : bursts_)
		burst.node->drop();
}

const char* ForceBreak::crackTexture(int stage)
{
	static const char* const textures[3] = { "assets/textures/damage/crack_stage1.png", "assets/textures/damage/crack_stage2.png",
		"assets/textures/damage/crack_stage3.png" };
	return stage >= 1 && stage <= 3 ? textures[stage - 1] : "";
}

void ForceBreak::add(const Interactable* object, ISceneNode* node, int durability, SColor debris, std::function<void()> broken)
{
	breakables_[object] = Breakable{ node, std::max(durability, 1), 0, debris, broken, nullptr };
}

int ForceBreak::hits(const Interactable* object) const
{
	std::map<const Interactable*, Breakable>::const_iterator found = breakables_.find(object);
	return found == breakables_.end() ? 0 : found->second.hits;
}

int ForceBreak::durability(const Interactable* object) const
{
	std::map<const Interactable*, Breakable>::const_iterator found = breakables_.find(object);
	return found == breakables_.end() ? 0 : found->second.durability;
}

int ForceBreak::crackStage(const Interactable* object) const
{
	int taken = hits(object);
	int most = durability(object);
	if (taken <= 0 || most <= 0 || taken >= most)
		return 0;
	return std::min(3, std::max(1, (3 * taken + most - 1) / most));
}

ISceneNode* ForceBreak::cracks(const Interactable* object) const
{
	std::map<const Interactable*, Breakable>::const_iterator found = breakables_.find(object);
	return found == breakables_.end() ? nullptr : found->second.cracks;
}

BreakResult ForceBreak::applyDamage(const Interactable* object)
{
	std::map<const Interactable*, Breakable>::iterator found = breakables_.find(object);
	if (found == breakables_.end())
		return BreakResult::NotBreakable;
	Breakable& breakable = found->second;
	if (breakable.hits >= breakable.durability)
		return BreakResult::AlreadyBroken;
	++breakable.hits;
	if (breakable.hits < breakable.durability)
	{
		showCracks(breakable);
		return BreakResult::Cracked;
	}
	burst(breakable);
	if (breakable.node)
		breakable.node->setVisible(false);
	if (breakable.broken)
		breakable.broken();
	return BreakResult::Broken;
}

void ForceBreak::showCracks(Breakable& breakable)
{
	IMeshSceneNode* body = breakable.node && (breakable.node->getType() == ESNT_MESH || breakable.node->getType() == ESNT_OCTREE)
		? static_cast<IMeshSceneNode*>(breakable.node) : nullptr;
	if (!body || !body->getMesh())
		return;
	if (!breakable.cracks)
	{
		// A white copy of the mesh, a hair larger, so the crack texture shows in its own colours over the object's.
		SMesh* copy = scene_->getMeshManipulator()->createMeshCopy(body->getMesh());
		scene_->getMeshManipulator()->setVertexColors(copy, SColor(255, 255, 255, 255));
		breakable.cracks = scene_->addMeshSceneNode(copy, body, -1, vector3df(), vector3df(), vector3df(1.02f, 1.02f, 1.02f));
		copy->drop();
		breakable.cracks->setName("cracks");
		breakable.cracks->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL);
		breakable.cracks->setMaterialFlag(EMF_LIGHTING, false);
		breakable.cracks->setMaterialFlag(EMF_FOG_ENABLE, true);
	}
	int taken = breakable.hits;
	int stage = std::min(3, std::max(1, (3 * taken + breakable.durability - 1) / breakable.durability));
	breakable.cracks->setMaterialTexture(0, scene_->getVideoDriver()->getTexture(crackTexture(stage)));
}

void ForceBreak::burst(const Breakable& breakable)
{
	if (!breakable.node)
		return;
	breakable.node->updateAbsolutePosition();
	aabbox3df box = breakable.node->getTransformedBoundingBox();
	vector3df half = box.getExtent() / 2.0f;
	IParticleSystemSceneNode* ps = scene_->addParticleSystemSceneNode(false, 0, -1, box.getCenter());
	ps->setName("force break debris");
	SColor dark(255, breakable.debris.getRed() * 3 / 5, breakable.debris.getGreen() * 3 / 5, breakable.debris.getBlue() * 3 / 5);
	// Irrlicht times are in milliseconds: 4 m/s outward, 0.5-1 s of life, chips 6-18 cm across.
	IParticleEmitter* emitter = ps->createBoxEmitter(aabbox3df(-half, half), vector3df(0.0f, 0.004f, 0.0f), 700, 900, dark, breakable.debris,
		500, 1000, 75, dimension2df(0.06f, 0.06f), dimension2df(0.18f, 0.18f));
	ps->setEmitter(emitter);
	emitter->drop();
	IParticleAffector* gravity = ps->createGravityAffector(vector3df(0.0f, -0.008f, 0.0f), 500);
	ps->addAffector(gravity);
	gravity->drop();
	IParticleAffector* fade = ps->createFadeOutParticleAffector(SColor(0, breakable.debris.getRed(), breakable.debris.getGreen(), breakable.debris.getBlue()), 500);
	ps->addAffector(fade);
	fade->drop();
	ps->setMaterialFlag(EMF_LIGHTING, false);
	ps->setMaterialFlag(EMF_ZWRITE_ENABLE, false);
	ps->setMaterialTexture(0, scene_->getVideoDriver()->getTexture("assets/textures/damage/debris.png"));
	ps->setMaterialType(EMT_TRANSPARENT_VERTEX_ALPHA);
	ps->grab();
	bursts_.push_back(Burst{ ps, 0.0f });
}

void ForceBreak::update(f32 seconds)
{
	for (Burst& burst : bursts_)
	{
		burst.age += seconds;
		// A burst, not a fountain: the emitter puffs once and stops.
		if (burst.age >= PUFF_SECONDS && burst.node->getEmitter())
			burst.node->setEmitter(0);
	}
	bursts_.erase(std::remove_if(bursts_.begin(), bursts_.end(), [](Burst& burst) {
		if (burst.age < BURST_SECONDS)
			return false;
		burst.node->remove();
		burst.node->drop();
		return true;
	}), bursts_.end());
}
