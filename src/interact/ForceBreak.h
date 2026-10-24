#ifndef FORCE_BREAK_H
#define FORCE_BREAK_H

#include "Interactable.h"
#include <functional>
#include <irrlicht.h>
#include <map>
#include <vector>

enum class BreakResult { NotBreakable, Cracked, Broken, AlreadyBroken };

// Force-breaking, in two stages (Mike's design): each successful Force hit takes one point of an object's durability and lays
// crack textures over it, worse as it weakens; the last hit shatters it in a burst of debris the colour of its material, hides
// it for the rest of the session, and tells its owner so the gameplay can follow (a door stops blocking, a crate spills).
// The cracks are a copy of the object's mesh drawn just outside it with an alpha crack texture: Irrlicht 1.8 has no decals.
class ForceBreak
{
public:
	explicit ForceBreak(irr::scene::ISceneManager* scene) : scene_(scene) {}
	~ForceBreak();

	// Makes an object breakable. node is what is seen (and hidden on breaking); broken runs once, on the final hit.
	void add(const Interactable* object, irr::scene::ISceneNode* node, int durability, irr::video::SColor debris, std::function<void()> broken);
	// One successful Force hit. The strength check is the caller's: only call this for a hit that landed.
	BreakResult applyDamage(const Interactable* object);
	// Runs the bursts: their emitters stop after a short puff, and they are removed once the debris has faded.
	void update(irr::f32 seconds);

	int hits(const Interactable* object) const;
	int durability(const Interactable* object) const;
	bool isBroken(const Interactable* object) const { return hits(object) >= durability(object) && durability(object) > 0; }
	// 0 for no cracks, else 1-3: a third, two thirds or nearly all of the durability gone.
	int crackStage(const Interactable* object) const;
	irr::scene::ISceneNode* cracks(const Interactable* object) const;
	std::size_t bursts() const { return bursts_.size(); }

	static const char* crackTexture(int stage);
	static const irr::f32 PUFF_SECONDS;
	static const irr::f32 BURST_SECONDS;

private:
	struct Breakable
	{
		irr::scene::ISceneNode* node;
		int durability;
		int hits;
		irr::video::SColor debris;
		std::function<void()> broken;
		irr::scene::IMeshSceneNode* cracks;
	};
	struct Burst
	{
		irr::scene::IParticleSystemSceneNode* node;
		irr::f32 age;
	};

	void showCracks(Breakable& breakable);
	void burst(const Breakable& breakable);

	irr::scene::ISceneManager* scene_;
	std::map<const Interactable*, Breakable> breakables_;
	std::vector<Burst> bursts_;
};

#endif
