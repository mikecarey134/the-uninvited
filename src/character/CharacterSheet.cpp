#include "CharacterSheet.h"

namespace character
{
	const int CharacterSheet::RANDOM_POINTS;
	const int CharacterSheet::PLAYER_POINTS;
	const int CharacterSheet::BLOODLINE_MULTIPLIER;
	const int CharacterSheet::BLOODLINE_ONE_IN;
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
		CharacterSheet sheet;
		sheet.name_ = name;
		sheet.gender_ = gender;
		sheet.profession_ = profession;
		for (int point = 0; point < RANDOM_POINTS; ++point)
			++sheet.rolled_[random() % ATTRIBUTE_COUNT];
		if (bloodlineOneIn > 0 && random() % bloodlineOneIn == 0)
		{
			sheet.hasBloodline_ = true;
			sheet.bloodline_ = (Attribute)(random() % ATTRIBUTE_COUNT);
		}
		for (const std::pair<Skill, int>& bonus : profession.skillBonuses)
			sheet.startingSkills_[(int)bonus.first] += bonus.second;
		return sheet;
	}

	int CharacterSheet::attribute(Attribute attribute) const
	{
		int value = rolled_[(int)attribute] + placed_[(int)attribute];
		if (hasBloodline_ && bloodline_ == attribute)
			value *= BLOODLINE_MULTIPLIER;
		return value;
	}

	int CharacterSheet::unplacedPoints() const
	{
		int left = PLAYER_POINTS;
		for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
			left -= placed_[i];
		return left;
	}

	bool CharacterSheet::placePoints(Attribute attribute, int amount)
	{
		if (amount <= 0 || amount > unplacedPoints())
			return false;
		placed_[(int)attribute] += amount;
		return true;
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
