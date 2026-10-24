#ifndef TARGET_HUD_H
#define TARGET_HUD_H

#include "ApproachConfig.h"
#include "DpadHud.h"
#include "Interactable.h"
#include <cstddef>

// The glue between detection, the approach registry and the D-pad: when the target changes, the D-pad shows that target's name,
// icon and approaches (Normal up, Force right, Skill down, Guile left) with nothing selected, unless the target has only one
// approach, which starts selected. The selection holds while the target does. When the target is lost the D-pad goes idle.
class TargetHud
{
public:
	explicit TargetHud(const ApproachRegistry& registry) : registry_(registry) {}

	void update(Interactable* target, std::size_t inRange, DpadHud& hud);

	Interactable* target() const { return shown_; }
	const ApproachSet* approaches() const;
	const ApproachOption* option(Approach approach) const;
	const ApproachOption* selected() const { return selected_; }
	// Selects an approach the target offers and returns true; anything else changes nothing and returns false.
	bool select(Approach approach, DpadHud& hud);

	static DpadDirection slotFor(Approach approach);
	static const char* nameOf(Approach approach);

private:
	ApproachRegistry registry_;
	Interactable* shown_ = nullptr;
	bool multiple_ = false;
	const ApproachOption* selected_ = nullptr;
};

#endif
