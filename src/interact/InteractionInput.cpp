#include "InteractionInput.h"

using namespace irr;

namespace
{
	const Approach byNumber[4] = { Approach::NORMAL, Approach::FORCE, Approach::SKILL, Approach::GUILE };
	const char* const modeIcons[4] = { "modes/normal.png", "modes/force.png", "modes/skill.png", "modes/guile.png" };
}

bool InteractionInput::onEvent(const SEvent& event)
{
	if (!targets_.target())
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

void InteractionInput::update()
{
	if (panel_.isOpen() && targets_.target() != panelTarget_)
		panel_.close();
}

bool InteractionInput::execute(Approach approach)
{
	if (!targets_.target() || !targets_.option(approach))
		return false;
	panel_.close();
	if (log_)
		log_(std::string("INTERACT: ") + TargetHud::nameOf(approach) + " on " + targets_.target()->getInfo().name);
	return true;
}
