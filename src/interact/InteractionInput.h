#ifndef INTERACTION_INPUT_H
#define INTERACTION_INPUT_H

#include "ContextPanel.h"
#include "TargetHud.h"
#include <functional>
#include <irrlicht.h>
#include <string>

// Routes player input to the targeted interactable. With nothing targeted every event passes through untouched.
// Controller: the gamepad layer posts the D-pad as the arrow keys and X as R, so arrows pick an approach and R executes the
// selected one.
// PC: left-click or E runs Normal; 1-4 run Normal, Force, Skill, Guile; right-click, Q or Tab toggle the context panel, whose rows
// run on a click.
class InteractionInput
{
public:
	InteractionInput(TargetHud& targets, DpadHud& hud, ContextPanel& panel) : targets_(targets), hud_(hud), panel_(panel) {}

	void setLog(std::function<void(const std::string&)> log) { log_ = log; }
	bool onEvent(const irr::SEvent& event);
	// Call every frame after the target updates: the panel closes when its target goes.
	void update();
	// Executes an approach the target offers; for now that means logging it (outcomes are SCRUM-1005).
	bool execute(Approach approach);

private:
	bool onMouse(const irr::SEvent::SMouseInput& mouse);
	void togglePanel();

	TargetHud& targets_;
	DpadHud& hud_;
	ContextPanel& panel_;
	const Interactable* panelTarget_ = nullptr;
	std::function<void(const std::string&)> log_;
};

#endif
