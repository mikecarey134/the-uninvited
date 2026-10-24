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
	// The stats approach handlers check.
	void setPlayer(const character::CharacterSheet* player) { player_ = player; }
	// While this returns true (the console or a screen is open) every event passes through.
	void setBlocked(std::function<bool()> blocked) { blocked_ = blocked; }
	bool onEvent(const irr::SEvent& event);
	// Call every frame after the target updates: the panel closes when its target goes, and feedback runs down.
	void update(irr::f32 seconds);
	// Runs the target's handler for an approach it offers and returns whether it succeeded. Approaches the target lacks do nothing.
	bool execute(Approach approach);

	// The last handler's line, shown briefly on screen, and whether it succeeded.
	const std::string& feedback() const { return feedback_; }
	bool feedbackSucceeded() const { return succeeded_; }

	static const irr::f32 FEEDBACK_SECONDS;

private:
	bool onMouse(const irr::SEvent::SMouseInput& mouse);
	void togglePanel();

	TargetHud& targets_;
	DpadHud& hud_;
	ContextPanel& panel_;
	const Interactable* panelTarget_ = nullptr;
	std::function<void(const std::string&)> log_;
	std::function<bool()> blocked_;
	const character::CharacterSheet* player_ = nullptr;
	std::string feedback_;
	bool succeeded_ = false;
	irr::f32 feedbackLeft_ = 0.0f;
};

#endif
