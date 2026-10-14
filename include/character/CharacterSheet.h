#pragma once
#include "Profession.h"
#include <random>
#include <string>

namespace character
{
	enum class Gender { Male, Female };

	struct Aegis
	{
		static const int RANDOM_POINTS = 100;
		static const int BASE_POOL = 20;
		static const int INTELLIGENCE_PER_POOL_POINT = 5;
		static const int BLOODLINE_MULTIPLIER = 10;
		static const int BLOODLINE_ONE_IN = 100;

		static Aegis roll(std::mt19937& random, int bloodlineOneIn = BLOODLINE_ONE_IN);

		int pool() const;
		int unplaced() const;
		int value(Attribute attribute) const;
		bool place(Attribute attribute, int amount);
		bool remove(Attribute attribute, int amount);

		int rolled[ATTRIBUTE_COUNT] = {};
		int placed[ATTRIBUTE_COUNT] = {};
		bool hasBloodline = false;
		Attribute bloodlineAttribute = Attribute::Agility;
		std::string bloodline;
	};

	class CharacterSheet
	{
	public:
		static const int RANDOM_POINTS = Aegis::RANDOM_POINTS;
		static const int BLOODLINE_MULTIPLIER = Aegis::BLOODLINE_MULTIPLIER;
		static const int BLOODLINE_ONE_IN = Aegis::BLOODLINE_ONE_IN;
		static const int EXPERIENCE_PER_LEVEL = 100;

		static CharacterSheet create(const std::string& name, Gender gender, const Profession& profession);
		static CharacterSheet create(const std::string& name, Gender gender, const Profession& profession,
			std::mt19937& random, int bloodlineOneIn = BLOODLINE_ONE_IN);
		static CharacterSheet create(const std::string& name, Gender gender, const Profession& profession, const Aegis& aegis);
		static CharacterSheet fromText(const std::string& text, std::string& problem);
		std::string toText() const;

		const std::string& name() const { return name_; }
		Gender gender() const { return gender_; }
		float age() const { return age_; }
		void setAge(float age) { age_ = age; }
		const Profession& profession() const { return profession_; }

		const Aegis& aegis() const { return aegis_; }
		int attribute(Attribute attribute) const { return aegis_.value(attribute); }
		int rolledPoints(Attribute attribute) const { return aegis_.rolled[(int)attribute]; }
		int placedPoints(Attribute attribute) const { return aegis_.placed[(int)attribute]; }
		int pointPool() const { return aegis_.pool(); }
		int unplacedPoints() const { return aegis_.unplaced(); }
		bool placePoints(Attribute attribute, int amount) { return aegis_.place(attribute, amount); }
		bool removePoints(Attribute attribute, int amount) { return aegis_.remove(attribute, amount); }

		bool hasBloodline() const { return aegis_.hasBloodline; }
		Attribute bloodlineAttribute() const { return aegis_.bloodlineAttribute; }
		const std::string& bloodlineName() const { return aegis_.bloodline; }

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
		Aegis aegis_;
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
