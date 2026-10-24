#include "ApproachConfig.h"

const ApproachOption* ApproachSet::getByType(Approach type) const
{
	for (const ApproachOption& option : options)
		if (option.type == type)
			return &option;
	return nullptr;
}

bool ApproachSet::hasApproach(Approach type) const
{
	return getByType(type) != nullptr;
}

void ApproachRegistry::registerType(const std::string& objectType, const ApproachSet& set)
{
	ApproachSet stored = set;
	stored.objectType = objectType;
	sets_[objectType] = stored;
}

const ApproachSet* ApproachRegistry::getApproaches(const std::string& objectType) const
{
	std::map<std::string, ApproachSet>::const_iterator found = sets_.find(objectType);
	return found == sets_.end() ? nullptr : &found->second;
}

ApproachRegistry defaultApproaches()
{
	const char* const normal = "approach/normal.png";
	const char* const force = "approach/force.png";
	ApproachRegistry registry;
	registry.registerType("locked_door", { "", { { Approach::NORMAL, "Try Handle", normal, "" }, { Approach::FORCE, "Kick Down", force, "" },
		{ Approach::SKILL, "Pick Lock", "skills/lockpicking.png", "lockpicking" },
		{ Approach::GUILE, "Send Companion", "approach/companion-delegate.png", "" } } });
	registry.registerType("unlocked_door", { "", { { Approach::NORMAL, "Open Door", normal, "" } } });
	registry.registerType("supply_crate", { "", { { Approach::NORMAL, "Open Crate", normal, "" }, { Approach::FORCE, "Smash Open", force, "" } } });
	registry.registerType("terminal", { "", { { Approach::NORMAL, "Use Terminal", normal, "" },
		{ Approach::SKILL, "Hack Terminal", "skills/electronics.png", "electronics" } } });
	registry.registerType("workbench", { "", { { Approach::NORMAL, "Use Workbench", normal, "" },
		{ Approach::SKILL, "Craft Item", "skills/engineering.png", "engineering" } } });
	registry.registerType("npc_friendly", { "", { { Approach::NORMAL, "Talk", normal, "" }, { Approach::GUILE, "Trade", "approach/trade.png", "" } } });
	registry.registerType("barricaded_door", { "", { { Approach::NORMAL, "Examine", normal, "" }, { Approach::FORCE, "Break Through", force, "" },
		{ Approach::SKILL, "Dismantle", "skills/construction.png", "construction" } } });
	registry.registerType("generator", { "", { { Approach::NORMAL, "Turn On", normal, "" },
		{ Approach::SKILL, "Repair", "skills/engineering.png", "engineering" } } });
	return registry;
}
