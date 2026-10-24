#ifndef INTERACTION_DETECTOR_H
#define INTERACTION_DETECTOR_H

#include "Interactable.h"
#include <vector>

// Picks the one interactable the player means. One in range is targeted outright; with several, the one nearest the centre of a
// cone around the camera's facing wins, falling back to the nearest in range. Angles are measured in the ground plane because the
// camera only turns, it never pitches.
class InteractionDetector
{
public:
	static const float CONE_DEGREES;   // half-angle of the aim cone
	static const float STICKY_DEGREES; // bias toward the current target so the choice doesn't flicker between two

	// Call every frame.
	void update(const irr::core::vector3df& playerPos, const irr::core::vector3df& cameraForward, const irr::core::vector3df& cameraPos);

	// Register/unregister interactables (called when objects spawn/despawn).
	void addInteractable(Interactable* obj);
	void removeInteractable(Interactable* obj);

	Interactable* getTarget() const { return target_; }
	bool hasTarget() const { return target_ != nullptr; }
	std::vector<Interactable*> getInRange() const { return inRange_; }

private:
	std::vector<Interactable*> all_;
	std::vector<Interactable*> inRange_;
	Interactable* target_ = nullptr;
};

#endif
