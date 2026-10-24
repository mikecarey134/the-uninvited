#ifndef TARGET_HUD_H
#define TARGET_HUD_H

#include "ApproachConfig.h"
#include "DpadHud.h"
#include "Interactable.h"
#include <cstddef>

// The glue between detection, the approach registry and the D-pad: when the target changes, the D-pad shows that target's name,
// icon and approaches (Normal up, Force right, Skill down, Guile left); when the target is lost it goes idle.
class TargetHud
{
public:
	explicit TargetHud(const ApproachRegistry& registry) : registry_(registry) {}

	void update(const Interactable* target, std::size_t inRange, DpadHud& hud);

	static DpadDirection slotFor(Approach approach);

private:
	ApproachRegistry registry_;
	const Interactable* shown_ = nullptr;
	bool multiple_ = false;
};

#endif
