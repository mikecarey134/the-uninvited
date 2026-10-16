#include "doctest.h"
#include "CharacterSheet.h"

using namespace character;

static const Profession paramedic{ "Paramedic",
	{ { MEDICINE, 15 }, { ELECTRONICS, 10 }, { SURVIVALISM, 5 } }, { "first_aid_kit" } };

TEST_CASE("create makes a valid character ready for the player to finish")
{
	CharacterSheet sheet = CharacterSheet::create("Dana", Gender::Female, paramedic);
	CHECK(sheet.name() == "Dana");
	CHECK(sheet.gender() == Gender::Female);
	CHECK(sheet.age() == 28.0f);
	CHECK(sheet.profession().name == "Paramedic");
	CHECK(sheet.profession().startingItems == std::vector<std::string>{ "first_aid_kit" });

	int total = 0;
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
		total += sheet.rolledPoints(i);
	CHECK(total == 100);
	CHECK(sheet.unplacedPoints() == sheet.pointPool());

	CHECK(sheet.skill(MEDICINE) == 15);
	CHECK(sheet.skill(ELECTRONICS) == 10);
	CHECK(sheet.skill(SURVIVALISM) == 5);
	CHECK(sheet.skill(COOKING) == 0);

	CHECK(sheet.water() == 100.0f);
	CHECK(sheet.hunger() == 100.0f);
	CHECK(sheet.sanity() == 100.0f);
}

TEST_CASE("a created character can be configured and ticked")
{
	std::mt19937 random(21);
	CharacterSheet sheet = CharacterSheet::create("Eli", Gender::Male, paramedic, random);
	sheet.setAge(42.5f);
	CHECK(sheet.age() == 42.5f);
	REQUIRE(sheet.placePoints(AGILITY, 10));
	REQUIRE(sheet.placePoints(ENDURANCE, 10));
	CHECK(sheet.unplacedPoints() == sheet.pointPool() - 20);
	sheet.addExperience(MEDICINE, 1000.0);
	CHECK(sheet.skill(MEDICINE) > 15);
	sheet.tick(60.0f, 60.0f);
	CHECK(sheet.water() < 100.0f);
	CHECK(sheet.hunger() < 100.0f);
	CHECK(sheet.sanity() < 100.0f);
}

TEST_CASE("the same seed makes the same character")
{
	std::mt19937 first(99);
	std::mt19937 second(99);
	CharacterSheet a = CharacterSheet::create("Fay", Gender::Female, paramedic, first, 1);
	CharacterSheet b = CharacterSheet::create("Fay", Gender::Female, paramedic, second, 1);
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
		CHECK(a.rolledPoints(i) == b.rolledPoints(i));
	CHECK(a.bloodlineAttribute() == b.bloodlineAttribute());
}

TEST_CASE("describe lists name, gender, profession, every attribute's roll and placement, bloodline and every skill")
{
	Aegis aegis;
	const int rolled[ATTRIBUTE_COUNT] = { 18, 22, 16, 30, 14 };
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
		aegis.rolled[i] = rolled[i];
	aegis.placed[(int)AGILITY] = 4;
	aegis.placed[(int)STRENGTH] = 2;
	const Profession blacksmith{ "Blacksmith", { { SMITHING, 30 } }, {} };
	std::vector<std::string> lines = describe(CharacterSheet::create("Dana", Gender::Female, blacksmith, aegis));

	const std::vector<std::string> expected = {
		"Name: Dana",
		"Gender: Female",
		"Profession: Blacksmith (Smithing +30)",
		"Agility: 22 (rolled 18 + placed 4)",
		"Endurance: 22 (rolled 22 + placed 0)",
		"Guile: 16 (rolled 16 + placed 0)",
		"Intelligence: 30 (rolled 30 + placed 0)",
		"Strength: 16 (rolled 14 + placed 2)",
		"Points: 20 to place of 26",
		"Bloodline: none",
		"Skills: Smithing 30, Chemistry 0, Medicine 0, Cooking 0, Survivalism 0, Lockpicking 0, Electronics 0, Engineering 0, Speech 0, Construction 0",
		"Experience gain: x1.30 from Intelligence 30",
		"Water 100, Hunger 100, Sanity 100" };
	CHECK(lines == expected);
}

TEST_CASE("describe shows a bloodline's attribute multiplied, with the plain value beside it")
{
	Aegis aegis;
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
		aegis.rolled[i] = 20;
	aegis.placed[(int)STRENGTH] = 3;
	aegis.hasBloodline = true;
	aegis.bloodlineAttribute = STRENGTH;
	aegis.bloodline = "[Placeholder]";
	std::vector<std::string> lines = describe(CharacterSheet::create("Ike", Gender::Male, paramedic, aegis));
	REQUIRE(lines.size() == 13);
	CHECK(lines[1] == "Gender: Male");
	CHECK(lines[2] == "Profession: Paramedic (Medicine +15, Electronics +10, Survivalism +5)");
	CHECK(lines[7] == "Strength: 230 (rolled 20 + placed 3, x10 bloodline)");
	CHECK(lines[3] == "Agility: 20 (rolled 20 + placed 0)");
	CHECK(lines[9] == "Bloodline: [Placeholder], Strength x10");
}

TEST_CASE("each attribute has a getter for its value and a setter for its rolled base, with placed points and bloodline still applied")
{
	struct Named { int id; int (CharacterSheet::*get)() const; void (CharacterSheet::*set)(int); };
	const Named attributes[] = {
		{ AGILITY, &CharacterSheet::getAgility, &CharacterSheet::setAgility },
		{ ENDURANCE, &CharacterSheet::getEndurance, &CharacterSheet::setEndurance },
		{ GUILE, &CharacterSheet::getGuile, &CharacterSheet::setGuile },
		{ INTELLIGENCE, &CharacterSheet::getIntelligence, &CharacterSheet::setIntelligence },
		{ STRENGTH, &CharacterSheet::getStrength, &CharacterSheet::setStrength } };
	for (const Named& named : attributes)
	{
		INFO(attributeName(named.id));
		Aegis aegis;
		for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
			aegis.rolled[i] = 10 + i;
		aegis.placed[named.id] = 3;
		CharacterSheet sheet = CharacterSheet::create("Ada", Gender::Female, paramedic, aegis);
		CHECK((sheet.*named.get)() == 13 + named.id);
		(sheet.*named.set)(40);
		CHECK(sheet.rolledPoints(named.id) == 40);
		CHECK((sheet.*named.get)() == 43);
		CHECK(sheet.attribute(named.id) == 43);
		for (int other = 0; other < ATTRIBUTE_COUNT; ++other)
			if (other != named.id)
				CHECK(sheet.attribute(other) == 10 + other);

		aegis.hasBloodline = true;
		aegis.bloodlineAttribute = named.id;
		CharacterSheet blooded = CharacterSheet::create("Ada", Gender::Female, paramedic, aegis);
		CHECK((blooded.*named.get)() == (13 + named.id) * CharacterSheet::BLOODLINE_MULTIPLIER);
	}
}

TEST_CASE("each skill has a getter for its level and a setter for its starting level, experience still on top")
{
	struct Named { int id; int (CharacterSheet::*get)() const; void (CharacterSheet::*set)(int); };
	const Named skills[] = {
		{ SMITHING, &CharacterSheet::getSmithing, &CharacterSheet::setSmithing },
		{ CHEMISTRY, &CharacterSheet::getChemistry, &CharacterSheet::setChemistry },
		{ MEDICINE, &CharacterSheet::getMedicine, &CharacterSheet::setMedicine },
		{ COOKING, &CharacterSheet::getCooking, &CharacterSheet::setCooking },
		{ SURVIVALISM, &CharacterSheet::getSurvivalism, &CharacterSheet::setSurvivalism },
		{ LOCKPICKING, &CharacterSheet::getLockpicking, &CharacterSheet::setLockpicking },
		{ ELECTRONICS, &CharacterSheet::getElectronics, &CharacterSheet::setElectronics },
		{ ENGINEERING, &CharacterSheet::getEngineering, &CharacterSheet::setEngineering },
		{ SPEECH, &CharacterSheet::getSpeech, &CharacterSheet::setSpeech },
		{ CONSTRUCTION, &CharacterSheet::getConstruction, &CharacterSheet::setConstruction } };
	for (const Named& named : skills)
	{
		INFO(skillName(named.id));
		CharacterSheet sheet = CharacterSheet::create("Ada", Gender::Female, paramedic);
		CHECK((sheet.*named.get)() == sheet.skill(named.id));
		(sheet.*named.set)(25 + named.id);
		CHECK((sheet.*named.get)() == 25 + named.id);
		CHECK(sheet.skill(named.id) == 25 + named.id);
		sheet.addExperience(named.id, 1000.0);
		CHECK((sheet.*named.get)() > 25 + named.id);
		for (int other = 0; other < SKILL_COUNT; ++other)
			if (other != named.id)
				CHECK(sheet.skill(other) != 25 + named.id);
	}
}
