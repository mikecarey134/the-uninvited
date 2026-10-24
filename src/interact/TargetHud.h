#ifndef TARGET_HUD_H
#define TARGET_HUD_H

#include "ApproachConfig.h"
#include "DpadHud.h"
#include "Interactable.h"
#include <cstddef>

// The glue between detection, the approach registry and the D-pad. Normal is not on the D-pad: it is what X (or E, or a
// left-click) does when no slot is picked. The D-pad holds the rest: Skill up, Guile right, a second Skill down, Force left.
// When the target changes the D-pad shows it with nothing picked; the interact prompt by the target always names the Normal verb
// ("E Open"), whatever is picked, because the D-pad's own labels say what each slot does. A pick holds while the target does, and
// the interact key runs it instead of Normal. When the target is lost the D-pad goes idle.
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
	// The interact prompt's verb: always the target's Normal action ("Open", "Search"), never the picked slot; empty with no
	// target or no Normal action.
	std::string promptAction() const;

	static const char* nameOf(Approach approach);

private:
	ApproachRegistry registry_;
	Interactable* shown_ = nullptr;
	bool multiple_ = false;
	const ApproachOption* selected_ = nullptr;
};

#endif
