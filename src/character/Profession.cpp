#include "Profession.h"
#include <sstream>

namespace character
{
	namespace
	{
		const char* const attributeNames[ATTRIBUTE_COUNT] = { "Agility", "Endurance", "Guile", "Intelligence", "Strength" };

		const char* const skillNames[SKILL_COUNT] = { "Smithing", "Chemistry", "Medicine", "Cooking", "Survivalism",
			"Lockpicking", "Electronics", "Engineering", "Speech", "Construction" };

		std::string trim(const std::string& text)
		{
			const char* space = " \t\r\n";
			std::string::size_type first = text.find_first_not_of(space);
			if (first == std::string::npos)
				return "";
			return text.substr(first, text.find_last_not_of(space) - first + 1);
		}

		std::vector<std::string> split(const std::string& text, char separator)
		{
			std::vector<std::string> parts;
			std::string::size_type start = 0;
			while (true)
			{
				std::string::size_type end = text.find(separator, start);
				parts.push_back(trim(text.substr(start, end == std::string::npos ? std::string::npos : end - start)));
				if (end == std::string::npos)
					return parts;
				start = end + 1;
			}
		}

		std::vector<std::string> list(const std::string& cell)
		{
			if (cell.empty() || cell == "-")
				return std::vector<std::string>();
			return split(cell, '|');
		}

		bool positiveNumber(const std::string& text, int& value)
		{
			if (text.empty() || text.size() > 6 || text.find_first_not_of("0123456789") != std::string::npos)
				return false;
			value = std::stoi(text);
			return value > 0;
		}

		bool fail(LoadError& error, int line, const std::string& message)
		{
			error.file = "professions.csv";
			error.line = line;
			error.message = message;
			return false;
		}

		bool readSkills(const std::string& cell, Profession& profession, std::string& problem)
		{
			for (const std::string& entry : list(cell))
			{
				std::string::size_type colon = entry.find(':');
				std::string name = trim(entry.substr(0, colon));
				Skill skill = Skill::Smithing;
				if (!skillFromName(name, skill))
				{
					problem = "unknown skill '" + name + "'";
					return false;
				}
				if (colon == std::string::npos)
				{
					problem = "skill '" + name + "' needs points, e.g. " + name + ":30";
					return false;
				}
				std::string points = trim(entry.substr(colon + 1));
				int value = 0;
				if (!positiveNumber(points, value))
				{
					problem = "bad points '" + points + "' for skill '" + name + "'";
					return false;
				}
				for (const std::pair<Skill, int>& existing : profession.skillBonuses)
					if (existing.first == skill)
					{
						problem = "skill '" + name + "' listed twice";
						return false;
					}
				profession.skillBonuses.push_back(std::make_pair(skill, value));
			}
			return true;
		}
	}

	const char* attributeName(Attribute attribute)
	{
		return attributeNames[(int)attribute];
	}

	const char* skillName(Skill skill)
	{
		return skillNames[(int)skill];
	}

	bool skillFromName(const std::string& name, Skill& skill)
	{
		for (int i = 0; i < SKILL_COUNT; ++i)
			if (name == skillNames[i])
			{
				skill = (Skill)i;
				return true;
			}
		return false;
	}

	const std::vector<Attribute>& governingAttributes(Skill skill)
	{
		static const std::vector<Attribute> table[SKILL_COUNT] = {
			{ Attribute::Strength },
			{ Attribute::Intelligence },
			{ Attribute::Intelligence },
			{},
			{ Attribute::Agility, Attribute::Endurance, Attribute::Guile },
			{ Attribute::Agility, Attribute::Guile },
			{ Attribute::Intelligence },
			{ Attribute::Intelligence, Attribute::Strength },
			{ Attribute::Guile },
			{ Attribute::Strength, Attribute::Endurance },
		};
		return table[(int)skill];
	}

	bool loadProfessions(const std::string& csv, std::vector<Profession>& professions, LoadError& error)
	{
		std::vector<Profession> loaded;
		std::istringstream in(csv);
		std::string line;
		int lineNumber = 0;
		int nameColumn = -1;
		int skillsColumn = -1;
		int itemsColumn = -1;
		bool haveHeader = false;
		while (std::getline(in, line))
		{
			++lineNumber;
			if (lineNumber == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0)
				line.erase(0, 3);
			if (trim(line).empty())
				continue;
			std::vector<std::string> cells = split(line, ',');
			if (!haveHeader)
			{
				for (std::string::size_type i = 0; i < cells.size(); ++i)
				{
					if (cells[i] == "name") nameColumn = (int)i;
					if (cells[i] == "skills") skillsColumn = (int)i;
					if (cells[i] == "items") itemsColumn = (int)i;
				}
				if (nameColumn < 0) return fail(error, lineNumber, "missing column 'name'");
				if (skillsColumn < 0) return fail(error, lineNumber, "missing column 'skills'");
				if (itemsColumn < 0) return fail(error, lineNumber, "missing column 'items'");
				haveHeader = true;
				continue;
			}
			Profession profession;
			profession.name = (std::size_t)nameColumn < cells.size() ? cells[nameColumn] : "";
			if (profession.name.empty())
				return fail(error, lineNumber, "missing name");
			if (findProfession(loaded, profession.name))
				return fail(error, lineNumber, "duplicate profession '" + profession.name + "'");
			std::string problem;
			if (!readSkills((std::size_t)skillsColumn < cells.size() ? cells[skillsColumn] : "", profession, problem))
				return fail(error, lineNumber, problem);
			profession.startingItems = list((std::size_t)itemsColumn < cells.size() ? cells[itemsColumn] : "");
			loaded.push_back(profession);
		}
		professions = loaded;
		return true;
	}

	const Profession* findProfession(const std::vector<Profession>& professions, const std::string& name)
	{
		for (const Profession& profession : professions)
			if (profession.name == name)
				return &profession;
		return nullptr;
	}
}
