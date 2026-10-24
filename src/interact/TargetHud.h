#ifndef TARGET_HUD_H
#define TARGET_HUD_H

#include "ApproachConfig.h"
#include "DpadHud.h"
#include "Interactable.h"
#include <cstddef>

// The glue between detection, the approach registry and the D-pad. Normal is not on the D-pad: it is what X (or E, or a
// left-click) does when no slot is picked. The D-pad holds the rest: Skill up, Guile right, a second Skill down, Force left.
// When the target changes the D-pad shows it with nothing picked and an "X:" prompt naming its Normal action; a pick holds while
// the target does. When the target is lost the D-pad goes idle.
class TargetHud
{
public:
	explicit TargetHud(const ApproachRegistry& registry) : registry_(registry) {}

	void update(Interactable* target, std::size_t inRange, DpadHud& hud);

	Interactable* target() const { return shown_; }
	const ApproachSet* approaches() const;
	// The first option of a kind (for Skill, the first skill).
	const ApproachOption* option(Approach approach) const;
	// What a D-pad slot holds for the target, or nullptr: UP the first Skill, RIGHT Guile, DOWN the second Skill, LEFT Force.
	const ApproachOption* optionAt(DpadDirection slot) const;
	// The picked slot's option, or nullptr when X would do Normal.
	const ApproachOption* selected() const { return selected_; }
	// Picks a filled slot, or unpicks it if it was already picked (back to Normal). Returns false for an empty slot.
	bool select(DpadDirection slot, DpadHud& hud);

	static const char* nameOf(Approach approach);

private:
	void promptNormal(DpadHud& hud) const;

	ApproachRegistry registry_;
	Interactable* shown_ = nullptr;
	bool multiple_ = false;
	const ApproachOption* selected_ = nullptr;
};

#endif
