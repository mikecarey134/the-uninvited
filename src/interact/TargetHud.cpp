#include "TargetHud.h"

DpadDirection TargetHud::slotFor(Approach approach)
{
	switch (approach)
	{
	case Approach::NORMAL:
		return DpadDirection::UP;
	case Approach::FORCE:
		return DpadDirection::RIGHT;
	case Approach::SKILL:
		return DpadDirection::DOWN;
	case Approach::GUILE:
		return DpadDirection::LEFT;
	}
	return DpadDirection::NONE;
}

void TargetHud::update(const Interactable* target, std::size_t inRange, DpadHud& hud)
{
	bool multiple = target && inRange > 1;
	if (target == shown_ && multiple == multiple_)
		return;
	shown_ = target;
	multiple_ = multiple;
	hud.clearSelection();
	hud.setActionPrompt("");
	if (!target)
	{
		hud.setState(DpadState::IDLE);
		return;
	}
	InteractableInfo info = target->getInfo();
	hud.setTarget(info.name, info.iconPath);
	const ApproachSet* set = registry_.getApproaches(target->getObjectType());
	const Approach all[4] = { Approach::NORMAL, Approach::FORCE, Approach::SKILL, Approach::GUILE };
	for (Approach approach : all)
	{
		const ApproachOption* option = set ? set->getByType(approach) : nullptr;
		if (option)
			hud.setSlot(slotFor(approach), option->label, option->iconPath);
		else
			hud.clearSlot(slotFor(approach));
	}
	hud.setState(multiple ? DpadState::MULTI_OBJECT : DpadState::ACTIVE);
}
