#include "CharacterSheet.h"
#include <algorithm>
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
			aegis.bloodlineAttribute = (random() % ATTRIBUTE_COUNT);
			aegis.bloodline = "[Placeholder]";
		}
		return aegis;
	}

	int Aegis::pool() const
	{
		return BASE_POOL + rolled[(int)INTELLIGENCE] / INTELLIGENCE_PER_POOL_POINT;
	}

	int Aegis::unplaced() const
	{
		int left = pool();
		for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
			left -= placed[i];
		return left;
	}

	int Aegis::value(int attribute) const
	{
		int value = rolled[attribute] + placed[attribute];
		return hasBloodline && bloodlineAttribute == attribute ? value * BLOODLINE_MULTIPLIER : value;
	}

	bool Aegis::place(int attribute, int amount)
	{
		if (amount <= 0 || amount > unplaced())
			return false;
		placed[attribute] += amount;
		return true;
	}

	bool Aegis::remove(int attribute, int amount)
	{
		if (amount <= 0 || amount > placed[attribute])
			return false;
		placed[attribute] -= amount;
		return true;
	}

	std::vector<std::string> describe(const CharacterSheet& sheet)
	{
		std::vector<std::string> lines;
		lines.push_back("Name: " + sheet.name());
		lines.push_back(std::string("Gender: ") + (sheet.gender() == Gender::Female ? "Female" : "Male"));
		std::string bonuses;
		for (const std::pair<int, int>& bonus : sheet.profession().skillBonuses)
			bonuses += (bonuses.empty() ? "" : ", ") + std::string(skillName(bonus.first)) + " +" + std::to_string(bonus.second);
		lines.push_back("Profession: " + sheet.profession().name + " (" + bonuses + ")");
		for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
		{
			int attribute = i;
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
			skills += (i ? ", " : "") + std::string(skillName(i)) + " " + std::to_string(sheet.skill(i));
		lines.push_back("Skills: " + skills);
		char gain[64];
		std::snprintf(gain, sizeof(gain), "Experience gain: x%.2f from Intelligence %d", experienceMultiplier(sheet.getIntelligence()),
			sheet.getIntelligence());
		lines.push_back(gain);
		lines.push_back("Water " + std::to_string((int)sheet.water()) + ", Hunger " + std::to_string((int)sheet.hunger()) + ", Sanity " +
			std::to_string((int)sheet.sanity()));
		return lines;
	}

	double experienceMultiplier(int intelligence)
	{
		return 1.0 + intelligence / 100.0;
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
		for (const std::pair<int, int>& bonus : profession.skillBonuses)
			sheet.startingSkills_[bonus.first] += bonus.second;
		return sheet;
	}

	int CharacterSheet::skill(int skill) const
	{
		return startingSkills_[skill] + (int)(experience_[skill] / EXPERIENCE_PER_LEVEL);
	}

	void CharacterSheet::addExperience(int skill, double amount)
	{
		if (amount > 0.0)
			experience_[skill] += amount * experienceMultiplier(getIntelligence());
	}

	void CharacterSheet::survivalTick(double gameSeconds)
	{
		double water = waterDrainPerSecond(getEndurance());
		double hunger = hungerDrainPerSecond(getEndurance());
		wounds_ += ((water_ <= 0.0 ? water : 0.0) + (hunger_ <= 0.0 ? hunger : 0.0)) * gameSeconds;
		water_ = std::max(water_ - water * gameSeconds, 0.0);
		hunger_ = std::max(hunger_ - hunger * gameSeconds, 0.0);
		int lost = (int)wounds_;
		wounds_ -= lost;
		health_ = std::max(health_ - lost, 0);
	}

	namespace
	{
		double drainPerSecond(double emptyDays, int endurance)
		{
			double base = 2.0 * 100.0 / (emptyDays * GAME_DAY_SECONDS);
			return base / (1.0 + endurance / 20.0);
		}
	}

	double waterDrainPerSecond(int endurance)
	{
		return drainPerSecond(WATER_EMPTY_DAYS, endurance);
	}

	double hungerDrainPerSecond(int endurance)
	{
		return drainPerSecond(HUNGER_EMPTY_DAYS, endurance);
	}

}
