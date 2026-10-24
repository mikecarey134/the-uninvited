#include "ForceBreak.h"
#include <algorithm>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;

const f32 ForceBreak::PUFF_SECONDS = 0.08f;
const f32 ForceBreak::BURST_SECONDS = 1.6f;
const f32 ForceBreak::DEBRIS_SECONDS = 4.0f;
const f32 ForceBreak::SHRINK_SECONDS = 0.6f;

namespace
{
	const f32 gravity = 9.8f;
	const f32 bounce = 0.3f;     // a landing keeps this much of its fall speed, upward
	const f32 restSpeed = 1.2f;  // below this a landing stops dead
	const f32 friction = 0.55f;  // sideways speed kept on each landing
}

ForceBreak::~ForceBreak()
{
	for (Burst& burst : bursts_)
		burst.node->drop();
	for (Piece& piece : pieces_)
		piece.node->drop();
}

f32 ForceBreak::random(f32 low, f32 high)
{
	seed_ = seed_ * 1103515245u + 12345u;
	return low + (high - low) * ((seed_ >> 8) & 0xFFFF) / 65535.0f;
}

std::vector<ISceneNode*> ForceBreak::debris() const
{
	std::vector<ISceneNode*> nodes;
	for (const Piece& piece : pieces_)
		nodes.push_back(piece.node);
	return nodes;
}

bool ForceBreak::settled(ISceneNode* node) const
{
	for (const Piece& piece : pieces_)
		if (piece.node == node)
			return piece.settled;
	return false;
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
	scatter(breakable);
	dust(breakable);
}

// Splintered planks from a stretched cube, each a little different, in the object's colour over a pale wood grain.
void ForceBreak::scatter(const Breakable& breakable)
{
	aabbox3df box = breakable.node->getTransformedBoundingBox();
	vector3df extent = box.getExtent();
	f32 longest = std::max(extent.X, std::max(extent.Y, extent.Z));
	vector3df centre = box.getCenter();
	vector3df away = centre - kicker_;
	away.Y = 0.0f;
	if (away.getLengthSQ() < 1e-6f)
		away = vector3df(0.0f, 0.0f, 1.0f);
	away.normalize();
	vector3df side(away.Z, 0.0f, -away.X);
	ITexture* grain = scene_->getVideoDriver()->getTexture("assets/textures/damage/plank.png");
	int count = 4 + (int)random(0.0f, 2.99f);
	for (int i = 0; i < count; ++i)
	{
		vector3df size(longest * random(0.3f, 0.5f), random(0.035f, 0.06f), random(0.1f, 0.2f));
		IMesh* mesh = scene_->getGeometryCreator()->createCubeMesh(vector3df(1.0f, 1.0f, 1.0f));
		f32 shade = random(0.8f, 1.1f);
		SColor tint(255, (u32)std::min(255.0f, breakable.debris.getRed() * shade), (u32)std::min(255.0f, breakable.debris.getGreen() * shade),
			(u32)std::min(255.0f, breakable.debris.getBlue() * shade));
		scene_->getMeshManipulator()->setVertexColors(mesh, tint);
		vector3df start = centre + vector3df(random(-0.5f, 0.5f) * extent.X, random(-0.3f, 0.3f) * extent.Y, random(-0.5f, 0.5f) * extent.Z);
		IMeshSceneNode* node = scene_->addMeshSceneNode(mesh, 0, -1, start, vector3df(random(0, 360), random(0, 360), random(0, 360)), size);
		mesh->drop();
		node->setName("force break piece");
		node->setMaterialTexture(0, grain);
		// The tint drives the ambient light too, or the bright sky's ambient washes the wood out to grey.
		node->getMaterial(0).ColorMaterial = ECM_DIFFUSE_AND_AMBIENT;
		node->setMaterialFlag(EMF_NORMALIZE_NORMALS, true);
		node->setMaterialFlag(EMF_FOG_ENABLE, true);
		node->grab();
		// Kicked away from the player: mostly forward, some sideways, all a little up.
		vector3df velocity = away * random(2.0f, 4.0f) + side * random(-1.8f, 1.8f) + vector3df(0.0f, random(1.0f, 3.0f), 0.0f);
		vector3df spin(random(-720, 720), random(-360, 360), random(-720, 720));
		pieces_.push_back(Piece{ node, velocity, spin, size, 0.0f, false });
	}
}

// A soft dust cloud at the break point: a short puff of big soft blobs that drift out, swell and fade.
void ForceBreak::dust(const Breakable& breakable)
{
	aabbox3df box = breakable.node->getTransformedBoundingBox();
	vector3df half = box.getExtent() / 2.0f;
	IParticleSystemSceneNode* ps = scene_->addParticleSystemSceneNode(false, 0, -1, box.getCenter());
	ps->setName("force break dust");
	// Paler and greyer than the object: dust, not chips.
	SColor base = breakable.debris;
	SColor light(150, (base.getRed() + 2 * 190) / 3, (base.getGreen() + 2 * 175) / 3, (base.getBlue() + 2 * 150) / 3);
	SColor dark(110, (base.getRed() + 140) / 2, (base.getGreen() + 130) / 2, (base.getBlue() + 115) / 2);
	// Irrlicht times are in milliseconds: about 25 puffs (300/s for 80ms), drifting at 0.6 m/s, living 0.6-1 s, 0.5-0.9 m across.
	IParticleEmitter* emitter = ps->createBoxEmitter(aabbox3df(-half * 0.6f, half * 0.6f), vector3df(0.0f, 0.0006f, 0.0f), 280, 340, dark, light,
		600, 1000, 90, dimension2df(0.5f, 0.5f), dimension2df(0.9f, 0.9f));
	ps->setEmitter(emitter);
	emitter->drop();
	IParticleAffector* swell = ps->createScaleParticleAffector(dimension2df(1.2f, 1.2f));
	ps->addAffector(swell);
	swell->drop();
	IParticleAffector* fade = ps->createFadeOutParticleAffector(SColor(0, light.getRed(), light.getGreen(), light.getBlue()), 800);
	ps->addAffector(fade);
	fade->drop();
	ps->setMaterialFlag(EMF_LIGHTING, false);
	ps->setMaterialFlag(EMF_ZWRITE_ENABLE, false);
	ps->setMaterialTexture(0, scene_->getVideoDriver()->getTexture("assets/textures/damage/dust.png"));
	ps->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL);
	ps->grab();
	bursts_.push_back(Burst{ ps, 0.0f });
}

void ForceBreak::update(f32 seconds)
{
	for (Burst& burst : bursts_)
	{
		burst.age += seconds;
		// A puff, not a fountain: the emitter fires once and stops.
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

	for (Piece& piece : pieces_)
	{
		if (piece.settled)
		{
			piece.restingFor += seconds;
			f32 left = 1.0f - std::min(std::max((piece.restingFor - DEBRIS_SECONDS) / SHRINK_SECONDS, 0.0f), 1.0f);
			piece.node->setScale(piece.size * std::max(left, 0.001f));
			vector3df at = piece.node->getPosition();
			piece.node->setPosition(vector3df(at.X, piece.size.Y * left / 2.0f, at.Z));
			continue;
		}
		// Plain projectile motion with a spin, as the player's gravity: no rigid-body solver.
		piece.velocity.Y -= gravity * seconds;
		piece.node->setRotation(piece.node->getRotation() + piece.spin * seconds);
		piece.node->setPosition(piece.node->getPosition() + piece.velocity * seconds);
		// The ground is the lowest corner of the tumbling piece as it now sits: lift it out if any corner went under.
		piece.node->updateAbsolutePosition();
		f32 under = -piece.node->getTransformedBoundingBox().MinEdge.Y;
		vector3df at = piece.node->getPosition();
		if (under > 0.0f)
			at.Y += under;
		// Touching the ground while falling is a landing: bounce, or stop dead if it is slow, lying flat.
		if (under >= 0.0f && piece.velocity.Y < 0.0f)
		{
			if (-piece.velocity.Y < restSpeed)
			{
				piece.settled = true;
				piece.velocity = vector3df();
				vector3df turned = piece.node->getRotation();
				piece.node->setRotation(vector3df(0.0f, turned.Y, 0.0f)); // lies flat on its widest face
				at.Y = piece.size.Y / 2.0f;
			}
			else
			{
				piece.velocity = vector3df(piece.velocity.X * friction, -piece.velocity.Y * bounce, piece.velocity.Z * friction);
				piece.spin *= 0.5f;
			}
		}
		piece.node->setPosition(at);
	}

	pieces_.erase(std::remove_if(pieces_.begin(), pieces_.end(), [](Piece& piece) {
		if (!piece.settled || piece.restingFor < DEBRIS_SECONDS + SHRINK_SECONDS)
			return false;
		piece.node->remove();
		piece.node->drop();
		return true;
	}), pieces_.end());
}
