#ifndef INTERACTION_INPUT_H
#define INTERACTION_INPUT_H

#include "ContextPanel.h"
#include "InteractionSounds.h"
#include "TargetHud.h"
#include <functional>
#include <irrlicht.h>
#include <string>

// Routes player input to the targeted interactable. With nothing targeted every event passes through untouched.
// Controller: the gamepad layer posts the D-pad as the arrow keys and X as R, so arrows pick a slot (Skill up, Guile right, second
// Skill down, Force left) and R runs the picked one, or Normal when none is picked.
// PC: E or left-click do the same as X (the picked slot, else Normal), matching the prompt; 1-4 run Force, Skill, second Skill,
// Guile directly; right-click, Q or Tab toggle the context panel, which lists those same four, and its rows run on a click.
class InteractionInput
{
public:
	InteractionInput(TargetHud& targets, DpadHud& hud, ContextPanel& panel) : targets_(targets), hud_(hud), panel_(panel) {}

	void setLog(std::function<void(const std::string&)> log) { log_ = log; }
	// Hears the sound cues: a tick on picking a slot, success or failure (or a locked-door rattle) on running an approach, and a
	// faint ping when a target appears out of nothing.
	void setCue(std::function<void(Cue)> cue) { cue_ = cue; }
	// The stats approach handlers check.
	void setPlayer(const character::CharacterSheet* player) { player_ = player; }
	// While this returns true (the console or a screen is open) every event passes through.
	void setBlocked(std::function<bool()> blocked) { blocked_ = blocked; }
	bool onEvent(const irr::SEvent& event);
	// Call every frame after the target updates: the panel closes when its target goes, and feedback runs down.
	void update(irr::f32 seconds);
	// Runs the target's handler for the first option of an approach it offers and returns whether it succeeded. Approaches the
	// target lacks do nothing.
	bool execute(Approach approach);
	// The same for one particular option, such as the target's second skill.
	bool run(const ApproachOption* option);
	// What the interact key does: the picked slot, or Normal when nothing is picked.
	bool runCurrent();

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
	std::function<void(Cue)> cue_;
	const Interactable* heard_ = nullptr;
	void play(Cue cue) const
	{
		if (cue_)
			cue_(cue);
	}
	const character::CharacterSheet* player_ = nullptr;
	std::string feedback_;
	bool succeeded_ = false;
	irr::f32 feedbackLeft_ = 0.0f;
};

#endif
