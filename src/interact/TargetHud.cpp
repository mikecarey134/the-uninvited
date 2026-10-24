#include "TargetHud.h"

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

const ApproachOption* TargetHud::optionAt(DpadDirection slot) const
{
	const ApproachSet* set = approaches();
	if (!set)
		return nullptr;
	switch (slot)
	{
	case DpadDirection::UP:
	case DpadDirection::DOWN:
	{
		int wanted = slot == DpadDirection::UP ? 0 : 1;
		for (const ApproachOption& each : set->options)
			if (each.type == Approach::SKILL && wanted-- == 0)
				return &each;
		return nullptr;
	}
	case DpadDirection::RIGHT:
		return set->getByType(Approach::GUILE);
	case DpadDirection::LEFT:
		return set->getByType(Approach::FORCE);
	case DpadDirection::NONE:
		break;
	}
	return nullptr;
}

void TargetHud::promptNormal(DpadHud& hud) const
{
	const ApproachOption* normal = option(Approach::NORMAL);
	hud.setActionPrompt(normal ? normal->label : "");
}

bool TargetHud::select(DpadDirection slot, DpadHud& hud)
{
	const ApproachOption* chosen = optionAt(slot);
	if (!chosen)
		return false;
	if (chosen == selected_)
	{
		selected_ = nullptr;
		hud.clearSelection();
		promptNormal(hud);
		return true;
	}
	selected_ = chosen;
	hud.setSelectedDirection(slot);
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
	InteractableInfo info = target->getInfo();
	hud.setTarget(info.name, info.iconPath);
	for (DpadDirection slot : { DpadDirection::UP, DpadDirection::RIGHT, DpadDirection::DOWN, DpadDirection::LEFT })
	{
		const ApproachOption* found = optionAt(slot);
		if (found)
			hud.setSlot(slot, found->label, found->iconPath);
		else
			hud.clearSlot(slot);
	}
	promptNormal(hud);
}
