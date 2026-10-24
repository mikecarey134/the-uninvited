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

const char* TargetHud::nameOf(Approach approach)
{
	switch (approach)
	{
	case Approach::NORMAL:
		return "Normal";
	case Approach::FORCE:
		return "Force";
	case Approach::SKILL:
		return "Skill";
	case Approach::GUILE:
		return "Guile";
	}
	return "";
}

const ApproachSet* TargetHud::approaches() const
{
	return shown_ ? registry_.getApproaches(shown_->getObjectType()) : nullptr;
}

const ApproachOption* TargetHud::option(Approach approach) const
{
	const ApproachSet* set = approaches();
	return set ? set->getByType(approach) : nullptr;
}

bool TargetHud::select(Approach approach, DpadHud& hud)
{
	const ApproachOption* chosen = option(approach);
	if (!chosen)
		return false;
	selected_ = chosen;
	hud.setSelectedDirection(slotFor(approach));
	hud.setActionPrompt(chosen->label);
	return true;
}

void TargetHud::update(Interactable* target, std::size_t inRange, DpadHud& hud)
{
	bool multiple = target && inRange > 1;
	if (target == shown_ && multiple == multiple_)
		return;
	bool changed = target != shown_;
	shown_ = target;
	multiple_ = multiple;
	if (!target)
	{
		selected_ = nullptr;
		hud.clearSelection();
		hud.setActionPrompt("");
		hud.setState(DpadState::IDLE);
		return;
	}
	hud.setState(multiple ? DpadState::MULTI_OBJECT : DpadState::ACTIVE);
	if (!changed)
		return;
	selected_ = nullptr;
	hud.clearSelection();
	hud.setActionPrompt("");
	InteractableInfo info = target->getInfo();
	hud.setTarget(info.name, info.iconPath);
	const ApproachSet* set = approaches();
	const Approach all[4] = { Approach::NORMAL, Approach::FORCE, Approach::SKILL, Approach::GUILE };
	for (Approach approach : all)
	{
		const ApproachOption* found = set ? set->getByType(approach) : nullptr;
		if (found)
			hud.setSlot(slotFor(approach), found->label, found->iconPath);
		else
			hud.clearSlot(slotFor(approach));
	}
	if (set && set->options.size() == 1)
		select(set->options.front().type, hud);
}
