#pragma once
#include <string>
#include <utility>
#include <vector>

namespace character
{
	enum class Attribute { Agility, Endurance, Guile, Intelligence, Strength };
	const int ATTRIBUTE_COUNT = 5;

	enum class Skill { Smithing, Chemistry, Medicine, Cooking, Survivalism, Lockpicking, Electronics, Engineering, Speech, Construction };
	const int SKILL_COUNT = 10;

	const char* attributeName(Attribute attribute);
	const char* skillName(Skill skill);
	bool skillFromName(const std::string& name, Skill& skill);
	const std::vector<Attribute>& governingAttributes(Skill skill);

	struct Profession
	{
		std::string name;
		std::vector<std::pair<Skill, int> > skillBonuses;
		std::vector<std::string> startingItems;
	};

	struct LoadError
	{
		std::string file;
		int line = 0;
		std::string message;
	};

	bool loadProfessions(const std::string& csv, std::vector<Profession>& professions, LoadError& error);
	const Profession* findProfession(const std::vector<Profession>& professions, const std::string& name);
}
