#include "CharacterSheet.h"
#include <cstdio>

namespace character
{
	const int Aegis::RANDOM_POINTS;
	const int Aegis::BASE_POOL;
	const int Aegis::INTELLIGENCE_PER_POOL_POINT;
	const int Aegis::BLOODLINE_MULTIPLIER;
	const int Aegis::BLOODLINE_ONE_IN;
	const int CharacterSheet::RANDOM_POINTS;
	const int CharacterSheet::BLOODLINE_MULTIPLIER;
	const int CharacterSheet::BLOODLINE_ONE_IN;
	const int CharacterSheet::MAX_HEALTH;
	const int CharacterSheet::EXPERIENCE_PER_LEVEL;

	namespace
	{
		float drained(float meter, float gameSeconds, float hoursToEmpty, int attribute)
		{
			float perSecond = 100.0f / (hoursToEmpty * 3600.0f) / drainSlowdown(attribute);
			float left = meter - perSecond * gameSeconds;
			return left > 0.0f ? left : 0.0f;
		}
	}

	Aegis Aegis::roll(std::mt19937& random, int bloodlineOneIn)
	{
		Aegis aegis;
		unsigned weights[ATTRIBUTE_COUNT];
		unsigned total = 0;
		for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
		{
			aegis.rolled[i] = 1;
			weights[i] = 100 + random() % 200;
			total += weights[i];
		}
		for (int point = ATTRIBUTE_COUNT; point < RANDOM_POINTS; ++point)
		{
			unsigned pick = random() % total;
			int i = 0;
			while (pick >= weights[i])
				pick -= weights[i++];
			++aegis.rolled[i];
		}
		if (bloodlineOneIn > 0 && random() % bloodlineOneIn == 0)
		{
			aegis.hasBloodline = true;
			aegis.bloodlineAttribute = (Attribute)(random() % ATTRIBUTE_COUNT);
			aegis.bloodline = "[Placeholder]";
		}
		return aegis;
	}

	int Aegis::pool() const
	{
		return BASE_POOL + rolled[(int)Attribute::Intelligence] / INTELLIGENCE_PER_POOL_POINT;
	}

	int Aegis::unplaced() const
	{
		int left = pool();
		for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
			left -= placed[i];
		return left;
	}

	int Aegis::value(Attribute attribute) const
	{
		int value = rolled[(int)attribute] + placed[(int)attribute];
		return hasBloodline && bloodlineAttribute == attribute ? value * BLOODLINE_MULTIPLIER : value;
	}

	bool Aegis::place(Attribute attribute, int amount)
	{
		if (amount <= 0 || amount > unplaced())
			return false;
		placed[(int)attribute] += amount;
		return true;
	}

	bool Aegis::remove(Attribute attribute, int amount)
	{
		if (amount <= 0 || amount > placed[(int)attribute])
			return false;
		placed[(int)attribute] -= amount;
		return true;
	}

	std::vector<std::string> describe(const CharacterSheet& sheet)
	{
		std::vector<std::string> lines;
		lines.push_back("Name: " + sheet.name());
		lines.push_back(std::string("Gender: ") + (sheet.gender() == Gender::Female ? "Female" : "Male"));
		std::string bonuses;
		for (const std::pair<Skill, int>& bonus : sheet.profession().skillBonuses)
			bonuses += (bonuses.empty() ? "" : ", ") + std::string(skillName(bonus.first)) + " +" + std::to_string(bonus.second);
		lines.push_back("Profession: " + sheet.profession().name + " (" + bonuses + ")");
		for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
		{
			Attribute attribute = (Attribute)i;
			bool boosted = sheet.hasBloodline() && sheet.bloodlineAttribute() == attribute;
			lines.push_back(std::string(attributeName(attribute)) + ": " + std::to_string(sheet.attribute(attribute)) + " (rolled " +
				std::to_string(sheet.rolledPoints(attribute)) + " + placed " + std::to_string(sheet.placedPoints(attribute)) +
				(boosted ? ", x" + std::to_string(CharacterSheet::BLOODLINE_MULTIPLIER) + " bloodline)" : ")"));
		}
		lines.push_back("Points: " + std::to_string(sheet.unplacedPoints()) + " to place of " + std::to_string(sheet.pointPool()));
		lines.push_back(sheet.hasBloodline() ? "Bloodline: " + sheet.bloodlineName() + ", " + attributeName(sheet.bloodlineAttribute()) + " x" +
			std::to_string(CharacterSheet::BLOODLINE_MULTIPLIER) : "Bloodline: none");
		std::string skills;
		for (int i = 0; i < SKILL_COUNT; ++i)
			skills += (i ? ", " : "") + std::string(skillName((Skill)i)) + " " + std::to_string(sheet.skill((Skill)i));
		lines.push_back("Skills: " + skills);
		char gain[64];
		std::snprintf(gain, sizeof(gain), "Experience gain: x%.2f from Intelligence %d", experienceMultiplier(sheet.attribute(Attribute::Intelligence)),
			sheet.attribute(Attribute::Intelligence));
		lines.push_back(gain);
		lines.push_back("Water " + std::to_string((int)sheet.water()) + ", Hunger " + std::to_string((int)sheet.hunger()) + ", Sanity " +
			std::to_string((int)sheet.sanity()));
		return lines;
	}

	double experienceMultiplier(int intelligence)
	{
		return 1.0 + intelligence / 100.0;
	}

	float drainSlowdown(int attribute)
	{
		return 1.0f + attribute / 100.0f;
	}

	CharacterSheet CharacterSheet::create(const std::string& name, Gender gender, const Profession& profession)
	{
		std::random_device device;
		std::mt19937 random(device());
		return create(name, gender, profession, random);
	}

	CharacterSheet CharacterSheet::create(const std::string& name, Gender gender, const Profession& profession,
		std::mt19937& random, int bloodlineOneIn)
	{
		return create(name, gender, profession, Aegis::roll(random, bloodlineOneIn));
	}

	CharacterSheet CharacterSheet::create(const std::string& name, Gender gender, const Profession& profession, const Aegis& aegis)
	{
		CharacterSheet sheet;
		sheet.name_ = name;
		sheet.gender_ = gender;
		sheet.profession_ = profession;
		sheet.aegis_ = aegis;
		for (const std::pair<Skill, int>& bonus : profession.skillBonuses)
			sheet.startingSkills_[(int)bonus.first] += bonus.second;
		return sheet;
	}

	int CharacterSheet::skill(Skill skill) const
	{
		return startingSkills_[(int)skill] + (int)(experience_[(int)skill] / EXPERIENCE_PER_LEVEL);
	}

	void CharacterSheet::addExperience(Skill skill, double amount)
	{
		if (amount > 0.0)
			experience_[(int)skill] += amount * experienceMultiplier(attribute(Attribute::Intelligence));
	}

	void CharacterSheet::tick(float deltaSeconds, float timeCompression)
	{
		float gameSeconds = deltaSeconds * timeCompression;
		if (gameSeconds <= 0.0f)
			return;
		int endurance = attribute(Attribute::Endurance);
		water_ = drained(water_, gameSeconds, WATER_HOURS, endurance);
		hunger_ = drained(hunger_, gameSeconds, HUNGER_HOURS, endurance);
		sanity_ = drained(sanity_, gameSeconds, SANITY_HOURS, attribute(Attribute::Intelligence));
		dehydrated_ = water_ <= 0.0f;
		starving_ = hunger_ <= 0.0f;
		insane_ = sanity_ <= 0.0f;
	}
}
