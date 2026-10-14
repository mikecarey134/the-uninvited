#pragma once
#include "Profession.h"
#include <random>
#include <string>

namespace character
{
	enum class Gender { Male, Female };

	class CharacterSheet
	{
	public:
		static const int RANDOM_POINTS = 100;
		static const int PLAYER_POINTS = 20;
		static const int BLOODLINE_MULTIPLIER = 10;
		static const int BLOODLINE_ONE_IN = 100;
		static const int EXPERIENCE_PER_LEVEL = 100;

		static CharacterSheet create(const std::string& name, Gender gender, const Profession& profession);
		static CharacterSheet create(const std::string& name, Gender gender, const Profession& profession,
			std::mt19937& random, int bloodlineOneIn = BLOODLINE_ONE_IN);
		static CharacterSheet fromText(const std::string& text, std::string& problem);
		std::string toText() const;

		const std::string& name() const { return name_; }
		Gender gender() const { return gender_; }
		float age() const { return age_; }
		void setAge(float age) { age_ = age; }
		const Profession& profession() const { return profession_; }

		int attribute(Attribute attribute) const;
		int rolledPoints(Attribute attribute) const { return rolled_[(int)attribute]; }
		int placedPoints(Attribute attribute) const { return placed_[(int)attribute]; }
		int unplacedPoints() const;
		bool placePoints(Attribute attribute, int amount);

		bool hasBloodline() const { return hasBloodline_; }
		Attribute bloodlineAttribute() const { return bloodline_; }

		int skill(Skill skill) const;
		double experience(Skill skill) const { return experience_[(int)skill]; }
		void addExperience(Skill skill, double amount);

		float water() const { return water_; }
		float hunger() const { return hunger_; }
		float sanity() const { return sanity_; }
		bool isDehydrated() const { return dehydrated_; }
		bool isStarving() const { return starving_; }
		bool isInsane() const { return insane_; }
		void tick(float deltaSeconds, float timeCompression);

	private:
		CharacterSheet() {}

		std::string name_;
		Gender gender_ = Gender::Male;
		float age_ = 28.0f;
		Profession profession_;
		int rolled_[ATTRIBUTE_COUNT] = {};
		int placed_[ATTRIBUTE_COUNT] = {};
		bool hasBloodline_ = false;
		Attribute bloodline_ = Attribute::Agility;
		int startingSkills_[SKILL_COUNT] = {};
		double experience_[SKILL_COUNT] = {};
		float water_ = 100.0f;
		float hunger_ = 100.0f;
		float sanity_ = 100.0f;
		bool dehydrated_ = false;
		bool starving_ = false;
		bool insane_ = false;
	};

	const float WATER_HOURS = 72.0f;
	const float HUNGER_HOURS = 144.0f;
	const float SANITY_HOURS = 336.0f;

	double experienceMultiplier(int intelligence);
	float drainSlowdown(int attribute);
}
