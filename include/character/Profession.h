#pragma once
#include <string>
#include <utility>
#include <vector>

namespace character
{
	const int AGILITY = 0;
	const int ENDURANCE = 1;
	const int GUILE = 2;
	const int INTELLIGENCE = 3;
	const int STRENGTH = 4;
	const int ATTRIBUTE_COUNT = 5;

	const int SMITHING = 0;
	const int CHEMISTRY = 1;
	const int MEDICINE = 2;
	const int COOKING = 3;
	const int SURVIVALISM = 4;
	const int LOCKPICKING = 5;
	const int ELECTRONICS = 6;
	const int ENGINEERING = 7;
	const int SPEECH = 8;
	const int CONSTRUCTION = 9;
	const int SKILL_COUNT = 10;

	const char* attributeName(int attribute);
	const char* skillName(int skill);
	bool skillFromName(const std::string& name, int& skill);
	const std::vector<int>& governingAttributes(int skill);

	struct Profession
	{
		std::string name;
		std::vector<std::pair<int, int> > skillBonuses;
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
