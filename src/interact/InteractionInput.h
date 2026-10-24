#ifndef INTERACTION_INPUT_H
#define INTERACTION_INPUT_H

#include "TargetHud.h"
#include <functional>
#include <irrlicht.h>
#include <string>

// Routes player input to the targeted interactable. With nothing targeted every event passes through untouched.
// Controller: the gamepad layer posts the D-pad as the arrow keys and X as R, so arrows pick an approach and R executes the
// selected one.
class InteractionInput
{
public:
	InteractionInput(TargetHud& targets, DpadHud& hud) : targets_(targets), hud_(hud) {}

	void setLog(std::function<void(const std::string&)> log) { log_ = log; }
	bool onEvent(const irr::SEvent& event);
	// Executes an approach the target offers; for now that means logging it (outcomes are SCRUM-1005).
	bool execute(Approach approach);

private:
	TargetHud& targets_;
	DpadHud& hud_;
	std::function<void(const std::string&)> log_;
};

#endif
