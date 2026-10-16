#include "doctest.h"
#include "CharacterSheet.h"

using namespace character;

static const Profession nurse{ "Nurse", { { MEDICINE, 20 }, { CHEMISTRY, 10 } }, {} };

static CharacterSheet sheetFor(const Profession& profession, unsigned seed = 7, int bloodlineOneIn = 1000000)
{
	std::mt19937 random(seed);
	return CharacterSheet::create("Ben", Gender::Male, profession, random, bloodlineOneIn);
}

TEST_CASE("profession bonuses set the starting skills and every other skill starts at zero")
{
	CharacterSheet sheet = sheetFor(nurse);
	CHECK(sheet.skill(MEDICINE) == 20);
	CHECK(sheet.skill(CHEMISTRY) == 10);
	for (int i = 0; i < SKILL_COUNT; ++i)
	{
		int skill = i;
		if (skill != MEDICINE && skill != CHEMISTRY)
			CHECK(sheet.skill(skill) == 0);
		CHECK(sheet.experience(skill) == 0.0);
	}
}

TEST_CASE("experience gained is multiplied by Intelligence")
{
	CharacterSheet sheet = sheetFor(nurse);
	int intelligence = sheet.attribute(INTELLIGENCE);
	sheet.addExperience(COOKING, 40.0);
	CHECK(sheet.experience(COOKING) == doctest::Approx(40.0 * (1.0 + intelligence / 100.0)));
	CHECK(experienceMultiplier(0) == doctest::Approx(1.0));
	CHECK(experienceMultiplier(50) == doctest::Approx(1.5));
}

TEST_CASE("more Intelligence means more experience from the same use")
{
	CharacterSheet plain = sheetFor(nurse);
	CharacterSheet clever = sheetFor(nurse);
	REQUIRE(clever.placePoints(INTELLIGENCE, 20));
	plain.addExperience(SPEECH, 50.0);
	clever.addExperience(SPEECH, 50.0);
	CHECK(clever.experience(SPEECH) == doctest::Approx(plain.experience(SPEECH) + 50.0 * 0.2));
}

TEST_CASE("a bloodline in Intelligence multiplies experience through the boosted value")
{
	for (unsigned seed = 0; seed < 200; ++seed)
	{
		CharacterSheet sheet = sheetFor(nurse, seed, 1);
		if (sheet.bloodlineAttribute() != INTELLIGENCE)
			continue;
		sheet.addExperience(SPEECH, 10.0);
		CHECK(sheet.experience(SPEECH) == doctest::Approx(10.0 * (1.0 + sheet.rolledPoints(INTELLIGENCE) * 10 / 100.0)));
		return;
	}
	FAIL("no seed rolled an Intelligence bloodline");
}

TEST_CASE("every hundred experience raises the skill one level with no cap")
{
	CharacterSheet sheet = sheetFor(nurse);
	double multiplier = experienceMultiplier(sheet.attribute(INTELLIGENCE));
	sheet.addExperience(MEDICINE, 250.0 / multiplier);
	CHECK(sheet.skill(MEDICINE) == 22);
	sheet.addExperience(MEDICINE, 100000.0 / multiplier);
	CHECK(sheet.skill(MEDICINE) == 1022);
}

TEST_CASE("zero or negative experience changes nothing")
{
	CharacterSheet sheet = sheetFor(nurse);
	sheet.addExperience(MEDICINE, 0.0);
	sheet.addExperience(MEDICINE, -500.0);
	CHECK(sheet.experience(MEDICINE) == 0.0);
	CHECK(sheet.skill(MEDICINE) == 20);
}

TEST_CASE("each skill has exactly its governing attributes, and Cooking has none")
{
	typedef std::vector<int> Attributes;
	CHECK(governingAttributes(SMITHING) == Attributes{ STRENGTH });
	CHECK(governingAttributes(CHEMISTRY) == Attributes{ INTELLIGENCE });
	CHECK(governingAttributes(MEDICINE) == Attributes{ INTELLIGENCE });
	CHECK(governingAttributes(COOKING).empty());
	CHECK(governingAttributes(SURVIVALISM) == Attributes{ AGILITY, ENDURANCE, GUILE });
	CHECK(governingAttributes(LOCKPICKING) == Attributes{ AGILITY, GUILE });
	CHECK(governingAttributes(ELECTRONICS) == Attributes{ INTELLIGENCE });
	CHECK(governingAttributes(ENGINEERING) == Attributes{ INTELLIGENCE, STRENGTH });
	CHECK(governingAttributes(SPEECH) == Attributes{ GUILE });
	CHECK(governingAttributes(CONSTRUCTION) == Attributes{ STRENGTH, ENDURANCE });
}

TEST_CASE("the starting skill pool is forty times Intelligence over Intelligence plus twenty, to the nearest point")
{
	CHECK(skillPointPool(20) == 20);
	CHECK(skillPointPool(50) == 29);
	CHECK(skillPointPool(100) == 33);
	CHECK(skillPointPool(250) == 37);
	CHECK(skillPointPool(10) == 13);
	CHECK(skillPointPool(0) == 0);
	CHECK(skillPointPool(1000000) == 40);
}

TEST_CASE("setSkill sets a skill's starting value by its id, the same as its named setter")
{
	CharacterSheet sheet = CharacterSheet::create("Ivy", Gender::Female, Profession{ "Chef", { { COOKING, 30 } }, {} });
	sheet.setSkill(COOKING, 37);
	sheet.setSkill(SPEECH, 4);
	CHECK(sheet.getCooking() == 37);
	CHECK(sheet.getSpeech() == 4);
	CHECK(sheet.skill(MEDICINE) == 0);
}
