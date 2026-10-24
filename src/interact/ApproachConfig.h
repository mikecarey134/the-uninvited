#ifndef APPROACH_CONFIG_H
#define APPROACH_CONFIG_H

#include <map>
#include <string>
#include <vector>

enum class Approach { NORMAL, FORCE, SKILL, GUILE };

struct ApproachOption
{
	Approach type;
	std::string label;     // "Try Handle", "Kick Down", "Pick Lock", etc.
	std::string iconPath;  // path relative to assets/ui/icons/
	std::string skillName; // the skill a Skill approach needs; empty for the others
};

struct ApproachSet
{
	std::string objectType;              // "locked_door", "supply_crate", "terminal", etc.
	std::vector<ApproachOption> options; // Normal plus up to four D-pad options; up to two may be Skill

	const ApproachOption* getByType(Approach type) const;
	bool hasApproach(Approach type) const;
};

class ApproachRegistry
{
public:
	void registerType(const std::string& objectType, const ApproachSet& set);
	const ApproachSet* getApproaches(const std::string& objectType) const;

private:
	std::map<std::string, ApproachSet> sets_;
};

// The starting object types and what each one offers.
ApproachRegistry defaultApproaches();

#endif
