#include "InteractionInput.h"
#include "CharacterSheet.h"

using namespace irr;

const f32 InteractionInput::FEEDBACK_SECONDS = 2.5f;

namespace
{
	const Approach byNumber[4] = { Approach::NORMAL, Approach::FORCE, Approach::SKILL, Approach::GUILE };
	const char* const modeIcons[4] = { "modes/normal.png", "modes/force.png", "modes/skill.png", "modes/guile.png" };
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
		targets_.select(Approach::NORMAL, hud_);
		return true;
	case KEY_RIGHT:
		targets_.select(Approach::FORCE, hud_);
		return true;
	case KEY_DOWN:
		targets_.select(Approach::SKILL, hud_);
		return true;
	case KEY_LEFT:
		targets_.select(Approach::GUILE, hud_);
		return true;
	case KEY_KEY_R:
		if (targets_.selected())
			execute(targets_.selected()->type);
		return true;
	case KEY_KEY_E:
		execute(Approach::NORMAL);
		return true;
	case KEY_KEY_1:
	case KEY_KEY_2:
	case KEY_KEY_3:
	case KEY_KEY_4:
		execute(byNumber[event.KeyInput.Key - KEY_KEY_1]);
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
			execute(Approach::NORMAL);
		else if (panel_.rowAt(at) >= 0)
			execute(byNumber[panel_.rows()[panel_.rowAt(at)].number - 1]);
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
		if (const ApproachOption* option = targets_.option(byNumber[i]))
			rows.push_back(ContextRow{ i + 1, modeIcons[i], option->label });
	panelTarget_ = targets_.target();
	panel_.open(rows);
}

void InteractionInput::update(f32 seconds)
{
	if (panel_.isOpen() && targets_.target() != panelTarget_)
		panel_.close();
	if (feedbackLeft_ > 0.0f)
	{
		feedbackLeft_ -= seconds;
		if (feedbackLeft_ <= 0.0f)
			feedback_.clear();
	}
}

bool InteractionInput::execute(Approach approach)
{
	Interactable* target = targets_.target();
	if (!target || !targets_.option(approach) || !player_)
		return false;
	panel_.close();
	Actor actor{ *player_, "" };
	bool done = false;
	switch (approach)
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
	feedback_ = !actor.message.empty() ? actor.message : (done ? "" : "Can't do that.");
	feedbackLeft_ = feedback_.empty() ? 0.0f : FEEDBACK_SECONDS;
	if (log_ && !feedback_.empty())
		log_(std::string(done ? "OK: " : "FAILED: ") + feedback_);
	return done;
}
