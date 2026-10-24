#include "InteractionInput.h"
#include "CharacterSheet.h"

using namespace irr;

const f32 InteractionInput::FEEDBACK_SECONDS = 2.5f;

namespace
{
	// 1-4 and the panel follow the D-pad: 1 Force (left), 2 Skill (up), 3 second Skill (down), 4 Guile (right).
	const DpadDirection byNumber[4] = { DpadDirection::LEFT, DpadDirection::UP, DpadDirection::DOWN, DpadDirection::RIGHT };
	const char* const modeIcons[4] = { "modes/force.png", "modes/skill.png", "modes/skill.png", "modes/guile.png" };
}

bool InteractionInput::onEvent(const SEvent& event)
{
	if (!targets_.target() || (blocked_ && blocked_()))
		return false;
	if (event.EventType == EET_MOUSE_INPUT_EVENT)
		return onMouse(event.MouseInput);
	if (event.EventType != EET_KEY_INPUT_EVENT || !event.KeyInput.PressedDown)
		return false;
	switch (event.KeyInput.Key)
	{
	case KEY_UP:
		if (padPart() != PadPart::DPAD)
			return false;
		if (targets_.select(DpadDirection::UP, hud_))
			play(Cue::Select);
		return true;
	case KEY_RIGHT:
		if (padPart() != PadPart::DPAD)
			return false;
		if (targets_.select(DpadDirection::RIGHT, hud_))
			play(Cue::Select);
		return true;
	case KEY_DOWN:
		if (padPart() != PadPart::DPAD)
			return false;
		if (targets_.select(DpadDirection::DOWN, hud_))
			play(Cue::Select);
		return true;
	case KEY_LEFT:
		if (padPart() != PadPart::DPAD)
			return false;
		if (targets_.select(DpadDirection::LEFT, hud_))
			play(Cue::Select);
		return true;
	case KEY_KEY_R:
		if (padPart() != PadPart::BUTTON)
			return false;
		runCurrent();
		return true;
	case KEY_KEY_E:
		runCurrent();
		return true;
	case KEY_KEY_1:
	case KEY_KEY_2:
	case KEY_KEY_3:
	case KEY_KEY_4:
		run(targets_.optionAt(byNumber[event.KeyInput.Key - KEY_KEY_1]));
		return true;
	case KEY_KEY_Q:
	case KEY_TAB:
		togglePanel();
		return true;
	case KEY_ESCAPE:
		if (!panel_.isOpen())
			return false;
		panel_.close();
		return true;
	default:
		return false;
	}
}

bool InteractionInput::onMouse(const SEvent::SMouseInput& mouse)
{
	core::position2d<s32> at(mouse.X, mouse.Y);
	switch (mouse.Event)
	{
	case EMIE_MOUSE_MOVED:
		panel_.hover(at);
		return panel_.isOpen();
	case EMIE_LMOUSE_PRESSED_DOWN:
		if (!panel_.isOpen())
			runCurrent();
		else if (panel_.rowAt(at) >= 0)
			run(targets_.optionAt(byNumber[panel_.rows()[panel_.rowAt(at)].number - 1]));
		return true;
	case EMIE_RMOUSE_PRESSED_DOWN:
		togglePanel();
		return true;
	default:
		return panel_.isOpen();
	}
}

void InteractionInput::togglePanel()
{
	if (panel_.isOpen())
	{
		panel_.close();
		return;
	}
	std::vector<ContextRow> rows;
	for (int i = 0; i < 4; ++i)
		if (const ApproachOption* option = targets_.optionAt(byNumber[i]))
			rows.push_back(ContextRow{ i + 1, modeIcons[i], option->label });
	panelTarget_ = targets_.target();
	panel_.open(rows);
}

void InteractionInput::update(f32 seconds)
{
	// The ping is for a target appearing from nothing, not for aim moving from one target to the next.
	if (targets_.target() && !heard_)
		play(Cue::Target);
	heard_ = targets_.target();
	if (panel_.isOpen() && targets_.target() != panelTarget_)
		panel_.close();
	if (feedbackLeft_ > 0.0f)
	{
		feedbackLeft_ -= seconds;
		if (feedbackLeft_ <= 0.0f)
			feedback_.clear();
	}
}

bool InteractionInput::runCurrent()
{
	return run(targets_.selected() ? targets_.selected() : targets_.option(Approach::NORMAL));
}

bool InteractionInput::execute(Approach approach)
{
	return run(targets_.option(approach));
}

bool InteractionInput::run(const ApproachOption* option)
{
	Interactable* target = targets_.target();
	if (!target || !option || !player_)
		return false;
	panel_.close();
	Actor actor{ *player_, "", option->skillName };
	bool done = false;
	switch (option->type)
	{
	case Approach::NORMAL:
		done = target->onNormal(actor);
		break;
	case Approach::FORCE:
		done = target->onForce(actor);
		break;
	case Approach::SKILL:
		done = target->onSkill(actor);
		break;
	case Approach::GUILE:
		done = target->onGuile(actor);
		break;
	}
	succeeded_ = done;
	if (done)
		play(Cue::Success);
	else
		play(option->type == Approach::NORMAL && target->getObjectType() == "locked_door" ? Cue::Locked : Cue::Failure);
	std::string said = !actor.message.empty() ? actor.message : (done ? "" : "Can't do that.");
	// A handler that opened a screen has shown its result; only the console hears about it.
	feedback_ = actor.openedScreen ? "" : said;
	feedbackLeft_ = feedback_.empty() ? 0.0f : FEEDBACK_SECONDS;
	if (log_ && !said.empty())
		log_(std::string(done ? "OK: " : "FAILED: ") + said);
	return done;
}
