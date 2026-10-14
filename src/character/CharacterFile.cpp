#include "CharacterSheet.h"
#include "KeyValueFile.h"
#include <cstdlib>

namespace character
{
	namespace
	{
		std::string trim(const std::string& text)
		{
			const char* space = " \t\r\n";
			std::string::size_type first = text.find_first_not_of(space);
			if (first == std::string::npos)
				return "";
			return text.substr(first, text.find_last_not_of(space) - first + 1);
		}

		std::string joined(const std::vector<std::string>& items)
		{
			if (items.empty())
				return "-";
			std::string text;
			for (const std::string& item : items)
				text += (text.empty() ? "" : " | ") + item;
			return text;
		}

		std::vector<std::string> split(const std::string& text)
		{
			std::vector<std::string> parts;
			if (text == "-")
				return parts;
			std::string::size_type start = 0;
			while (true)
			{
				std::string::size_type end = text.find('|', start);
				parts.push_back(trim(text.substr(start, end == std::string::npos ? std::string::npos : end - start)));
				if (end == std::string::npos)
					return parts;
				start = end + 1;
			}
		}

		bool attributeFromName(const std::string& name, Attribute& attribute)
		{
			for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
				if (name == attributeName((Attribute)i))
				{
					attribute = (Attribute)i;
					return true;
				}
			return false;
		}
	}

	std::string CharacterSheet::toText() const
	{
		KeyValueWriter out;
		out.add("name", name_);
		out.add("gender", std::string(gender_ == Gender::Female ? "Female" : "Male"));
		out.add("age", age_, 9);
		out.add("profession", profession_.name);
		std::vector<std::string> bonuses;
		for (const std::pair<Skill, int>& bonus : profession_.skillBonuses)
			bonuses.push_back(std::string(skillName(bonus.first)) + ":" + std::to_string(bonus.second));
		out.add("profession_skills", joined(bonuses));
		out.add("profession_items", joined(profession_.startingItems));
		for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
		{
			out.add(std::string("rolled.") + attributeName((Attribute)i), rolled_[i]);
			out.add(std::string("placed.") + attributeName((Attribute)i), placed_[i]);
		}
		out.add("bloodline", std::string(hasBloodline_ ? attributeName(bloodline_) : "none"));
		for (int i = 0; i < SKILL_COUNT; ++i)
		{
			out.add(std::string("skill.") + skillName((Skill)i), startingSkills_[i]);
			out.add(std::string("experience.") + skillName((Skill)i), experience_[i], 17);
		}
		out.add("water", water_, 9);
		out.add("hunger", hunger_, 9);
		out.add("sanity", sanity_, 9);
		return out.text();
	}

	CharacterSheet CharacterSheet::fromText(const std::string& text, std::string& problem)
	{
		CharacterSheet sheet;
		KeyValueReader fields(text);
		std::string gender;
		double age = 0.0;
		std::string bonuses;
		std::string items;
		std::string bloodline;
		double water = 0.0;
		double hunger = 0.0;
		double sanity = 0.0;
		bool read = fields.problem().empty() && fields.text("name", sheet.name_) && fields.text("gender", gender) &&
			fields.real("age", age) && fields.text("profession", sheet.profession_.name) &&
			fields.text("profession_skills", bonuses) && fields.text("profession_items", items);
		for (int i = 0; read && i < ATTRIBUTE_COUNT; ++i)
		{
			std::string name = attributeName((Attribute)i);
			read = fields.whole("rolled." + name, sheet.rolled_[i]) && fields.whole("placed." + name, sheet.placed_[i]);
		}
		read = read && fields.text("bloodline", bloodline);
		for (int i = 0; read && i < SKILL_COUNT; ++i)
		{
			std::string name = skillName((Skill)i);
			read = fields.whole("skill." + name, sheet.startingSkills_[i]) && fields.real("experience." + name, sheet.experience_[i]);
		}
		read = read && fields.real("water", water) && fields.real("hunger", hunger) && fields.real("sanity", sanity);

		if (read && gender != "Male" && gender != "Female")
			read = fields.fail("bad gender: " + gender);
		for (const std::string& entry : read ? split(bonuses) : std::vector<std::string>())
		{
			std::string::size_type colon = entry.find(':');
			Skill skill = Skill::Smithing;
			if (colon == std::string::npos || !skillFromName(trim(entry.substr(0, colon)), skill))
			{
				read = fields.fail("bad profession skill: " + entry);
				break;
			}
			sheet.profession_.skillBonuses.push_back(std::make_pair(skill, std::atoi(entry.c_str() + colon + 1)));
		}
		sheet.hasBloodline_ = bloodline != "none";
		if (read && sheet.hasBloodline_ && !attributeFromName(bloodline, sheet.bloodline_))
			read = fields.fail("bad bloodline: " + bloodline);

		problem = fields.problem();
		if (!read)
			return sheet;
		sheet.gender_ = gender == "Female" ? Gender::Female : Gender::Male;
		sheet.age_ = (float)age;
		sheet.profession_.startingItems = split(items);
		sheet.water_ = (float)water;
		sheet.hunger_ = (float)hunger;
		sheet.sanity_ = (float)sanity;
		sheet.dehydrated_ = sheet.water_ <= 0.0f;
		sheet.starving_ = sheet.hunger_ <= 0.0f;
		sheet.insane_ = sheet.sanity_ <= 0.0f;
		return sheet;
	}
}
