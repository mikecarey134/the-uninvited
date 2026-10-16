#pragma once
#include "Profession.h"
#include <random>
#include <string>
#include <vector>

namespace character
{
	// 100 / (0.26 / (1 + 20 * 0.02)) = 538 one-second ticks, about 9 minutes for a full bar at Endurance 20
	const float SURVIVAL_BASE_DRAIN = 0.26f;
	const int SURVIVAL_HEALTH_DRAIN = 1;

	enum class Gender { Male, Female };

	struct Aegis
	{
		static const int RANDOM_POINTS = 100;
		static const int BASE_POOL = 20;
		static const int INTELLIGENCE_PER_POOL_POINT = 5;
		static const int BLOODLINE_MULTIPLIER = 10;
		static const int BLOODLINE_ONE_IN = 1000;

		static Aegis roll(std::mt19937& random, int bloodlineOneIn = BLOODLINE_ONE_IN);

		int pool() const;
		int unplaced() const;
		int value(int attribute) const;
		bool place(int attribute, int amount);
		bool remove(int attribute, int amount);

		int rolled[ATTRIBUTE_COUNT] = {};
		int placed[ATTRIBUTE_COUNT] = {};
		bool hasBloodline = false;
		int bloodlineAttribute = AGILITY;
		std::string bloodline;
	};

	class CharacterSheet
	{
	public:
		static const int RANDOM_POINTS = Aegis::RANDOM_POINTS;
		static const int BLOODLINE_MULTIPLIER = Aegis::BLOODLINE_MULTIPLIER;
		static const int BLOODLINE_ONE_IN = Aegis::BLOODLINE_ONE_IN;
		static const int EXPERIENCE_PER_LEVEL = 100;
		static const int MAX_HEALTH = 100;

		static CharacterSheet create(const std::string& name, Gender gender, const Profession& profession);
		static CharacterSheet create(const std::string& name, Gender gender, const Profession& profession,
			std::mt19937& random, int bloodlineOneIn = BLOODLINE_ONE_IN);
		static CharacterSheet create(const std::string& name, Gender gender, const Profession& profession, const Aegis& aegis);
		static CharacterSheet fromText(const std::string& text, std::string& problem);
		std::string toText() const;

		const std::string& name() const { return name_; }
		Gender gender() const { return gender_; }
		int outfit() const { return outfit_; }
		void setOutfit(int outfit) { outfit_ = outfit; }
		float age() const { return age_; }
		void setAge(float age) { age_ = age; }
		const Profession& profession() const { return profession_; }

		const Aegis& aegis() const { return aegis_; }
		int attribute(int attribute) const { return aegis_.value(attribute); }
		int rolledPoints(int attribute) const { return aegis_.rolled[attribute]; }
		int placedPoints(int attribute) const { return aegis_.placed[attribute]; }
		int pointPool() const { return aegis_.pool(); }
		int unplacedPoints() const { return aegis_.unplaced(); }
		bool placePoints(int attribute, int amount) { return aegis_.place(attribute, amount); }
		bool removePoints(int attribute, int amount) { return aegis_.remove(attribute, amount); }

		bool hasBloodline() const { return aegis_.hasBloodline; }
		int bloodlineAttribute() const { return aegis_.bloodlineAttribute; }
		int getAgility() const { return attribute(AGILITY); }
		void setAgility(int value) { aegis_.rolled[AGILITY] = value; }
		int getEndurance() const { return attribute(ENDURANCE); }
		void setEndurance(int value) { aegis_.rolled[ENDURANCE] = value; }
		int getGuile() const { return attribute(GUILE); }
		void setGuile(int value) { aegis_.rolled[GUILE] = value; }
		int getIntelligence() const { return attribute(INTELLIGENCE); }
		void setIntelligence(int value) { aegis_.rolled[INTELLIGENCE] = value; }
		int getStrength() const { return attribute(STRENGTH); }
		void setStrength(int value) { aegis_.rolled[STRENGTH] = value; }
		const std::string& bloodlineName() const { return aegis_.bloodline; }

		int skill(int skill) const;
		int getSmithing() const { return skill(SMITHING); }
		void setSmithing(int value) { startingSkills_[SMITHING] = value; }
		int getChemistry() const { return skill(CHEMISTRY); }
		void setChemistry(int value) { startingSkills_[CHEMISTRY] = value; }
		int getMedicine() const { return skill(MEDICINE); }
		void setMedicine(int value) { startingSkills_[MEDICINE] = value; }
		int getCooking() const { return skill(COOKING); }
		void setCooking(int value) { startingSkills_[COOKING] = value; }
		int getSurvivalism() const { return skill(SURVIVALISM); }
		void setSurvivalism(int value) { startingSkills_[SURVIVALISM] = value; }
		int getLockpicking() const { return skill(LOCKPICKING); }
		void setLockpicking(int value) { startingSkills_[LOCKPICKING] = value; }
		int getElectronics() const { return skill(ELECTRONICS); }
		void setElectronics(int value) { startingSkills_[ELECTRONICS] = value; }
		int getEngineering() const { return skill(ENGINEERING); }
		void setEngineering(int value) { startingSkills_[ENGINEERING] = value; }
		int getSpeech() const { return skill(SPEECH); }
		void setSpeech(int value) { startingSkills_[SPEECH] = value; }
		int getConstruction() const { return skill(CONSTRUCTION); }
		void setConstruction(int value) { startingSkills_[CONSTRUCTION] = value; }
		double experience(int skill) const { return experience_[skill]; }
		void addExperience(int skill, double amount);

		int health() const { return health_; }
		int maxHealth() const { return MAX_HEALTH; }
		bool isDead() const { return health_ <= 0; }
		void survivalTick();
		float water() const { return water_; }
		float hunger() const { return hunger_; }
		float sanity() const { return sanity_; }
		bool isDehydrated() const { return dehydrated_; }
		bool isStarving() const { return starving_; }
		bool isInsane() const { return insane_; }

	private:
		CharacterSheet() {}

		std::string name_;
		Gender gender_ = Gender::Male;
		int outfit_ = 0;
		float age_ = 28.0f;
		Profession profession_;
		Aegis aegis_;
		int startingSkills_[SKILL_COUNT] = {};
		double experience_[SKILL_COUNT] = {};
		int health_ = MAX_HEALTH;
		float water_ = 100.0f;
		float hunger_ = 100.0f;
		float sanity_ = 100.0f;
		bool dehydrated_ = false;
		bool starving_ = false;
		bool insane_ = false;
	};


	std::vector<std::string> describe(const CharacterSheet& sheet);
	double experienceMultiplier(int intelligence);
}
