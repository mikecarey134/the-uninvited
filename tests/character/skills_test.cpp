#include "doctest.h"
#include "CharacterSheet.h"

using namespace character;

static const Profession nurse{ "Nurse", { { Skill::Medicine, 20 }, { Skill::Chemistry, 10 } }, {} };

static CharacterSheet sheetFor(const Profession& profession, unsigned seed = 7, int bloodlineOneIn = 1000000)
{
	std::mt19937 random(seed);
	return CharacterSheet::create("Ben", Gender::Male, profession, random, bloodlineOneIn);
}

TEST_CASE("profession bonuses set the starting skills and every other skill starts at zero")
{
	CharacterSheet sheet = sheetFor(nurse);
	CHECK(sheet.skill(Skill::Medicine) == 20);
	CHECK(sheet.skill(Skill::Chemistry) == 10);
	for (int i = 0; i < SKILL_COUNT; ++i)
	{
		Skill skill = (Skill)i;
		if (skill != Skill::Medicine && skill != Skill::Chemistry)
			CHECK(sheet.skill(skill) == 0);
		CHECK(sheet.experience(skill) == 0.0);
	}
}

TEST_CASE("experience gained is multiplied by Intelligence")
{
	CharacterSheet sheet = sheetFor(nurse);
	int intelligence = sheet.attribute(Attribute::Intelligence);
	sheet.addExperience(Skill::Cooking, 40.0);
	CHECK(sheet.experience(Skill::Cooking) == doctest::Approx(40.0 * (1.0 + intelligence / 100.0)));
	CHECK(experienceMultiplier(0) == doctest::Approx(1.0));
	CHECK(experienceMultiplier(50) == doctest::Approx(1.5));
}

TEST_CASE("more Intelligence means more experience from the same use")
{
	CharacterSheet plain = sheetFor(nurse);
	CharacterSheet clever = sheetFor(nurse);
	REQUIRE(clever.placePoints(Attribute::Intelligence, 20));
	plain.addExperience(Skill::Speech, 50.0);
	clever.addExperience(Skill::Speech, 50.0);
	CHECK(clever.experience(Skill::Speech) == doctest::Approx(plain.experience(Skill::Speech) + 50.0 * 0.2));
}

TEST_CASE("a bloodline in Intelligence multiplies experience through the boosted value")
{
	for (unsigned seed = 0; seed < 200; ++seed)
	{
		CharacterSheet sheet = sheetFor(nurse, seed, 1);
		if (sheet.bloodlineAttribute() != Attribute::Intelligence)
			continue;
		sheet.addExperience(Skill::Speech, 10.0);
		CHECK(sheet.experience(Skill::Speech) == doctest::Approx(10.0 * (1.0 + sheet.rolledPoints(Attribute::Intelligence) * 10 / 100.0)));
		return;
	}
	FAIL("no seed rolled an Intelligence bloodline");
}

TEST_CASE("every hundred experience raises the skill one level with no cap")
{
	CharacterSheet sheet = sheetFor(nurse);
	double multiplier = experienceMultiplier(sheet.attribute(Attribute::Intelligence));
	sheet.addExperience(Skill::Medicine, 250.0 / multiplier);
	CHECK(sheet.skill(Skill::Medicine) == 22);
	sheet.addExperience(Skill::Medicine, 100000.0 / multiplier);
	CHECK(sheet.skill(Skill::Medicine) == 1022);
}

TEST_CASE("zero or negative experience changes nothing")
{
	CharacterSheet sheet = sheetFor(nurse);
	sheet.addExperience(Skill::Medicine, 0.0);
	sheet.addExperience(Skill::Medicine, -500.0);
	CHECK(sheet.experience(Skill::Medicine) == 0.0);
	CHECK(sheet.skill(Skill::Medicine) == 20);
}

TEST_CASE("each skill has exactly its governing attributes, and Cooking has none")
{
	typedef std::vector<Attribute> Attributes;
	CHECK(governingAttributes(Skill::Smithing) == Attributes{ Attribute::Strength });
	CHECK(governingAttributes(Skill::Chemistry) == Attributes{ Attribute::Intelligence });
	CHECK(governingAttributes(Skill::Medicine) == Attributes{ Attribute::Intelligence });
	CHECK(governingAttributes(Skill::Cooking).empty());
	CHECK(governingAttributes(Skill::Survivalism) == Attributes{ Attribute::Agility, Attribute::Endurance, Attribute::Guile });
	CHECK(governingAttributes(Skill::Lockpicking) == Attributes{ Attribute::Agility, Attribute::Guile });
	CHECK(governingAttributes(Skill::Electronics) == Attributes{ Attribute::Intelligence });
	CHECK(governingAttributes(Skill::Engineering) == Attributes{ Attribute::Intelligence, Attribute::Strength });
	CHECK(governingAttributes(Skill::Speech) == Attributes{ Attribute::Guile });
	CHECK(governingAttributes(Skill::Construction) == Attributes{ Attribute::Strength, Attribute::Endurance });
}
