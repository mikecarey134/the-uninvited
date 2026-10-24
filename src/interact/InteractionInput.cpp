#include "InteractionInput.h"

using namespace irr;

bool InteractionInput::onEvent(const SEvent& event)
{
	if (!targets_.target() || event.EventType != EET_KEY_INPUT_EVENT || !event.KeyInput.PressedDown)
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
	default:
		return false;
	}
}

bool InteractionInput::execute(Approach approach)
{
	if (!targets_.target() || !targets_.option(approach))
		return false;
	if (log_)
		log_(std::string("INTERACT: ") + TargetHud::nameOf(approach) + " on " + targets_.target()->getInfo().name);
	return true;
}
