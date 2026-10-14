#include "doctest.h"
#include "CharacterSheet.h"

using namespace character;

static const Profession paramedic{ "Paramedic",
	{ { Skill::Medicine, 15 }, { Skill::Electronics, 10 }, { Skill::Survivalism, 5 } }, { "first_aid_kit" } };

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
		total += sheet.rolledPoints((Attribute)i);
	CHECK(total == 100);
	CHECK(sheet.unplacedPoints() == sheet.pointPool());

	CHECK(sheet.skill(Skill::Medicine) == 15);
	CHECK(sheet.skill(Skill::Electronics) == 10);
	CHECK(sheet.skill(Skill::Survivalism) == 5);
	CHECK(sheet.skill(Skill::Cooking) == 0);

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
	REQUIRE(sheet.placePoints(Attribute::Agility, 10));
	REQUIRE(sheet.placePoints(Attribute::Endurance, 10));
	CHECK(sheet.unplacedPoints() == sheet.pointPool() - 20);
	sheet.addExperience(Skill::Medicine, 1000.0);
	CHECK(sheet.skill(Skill::Medicine) > 15);
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
		CHECK(a.rolledPoints((Attribute)i) == b.rolledPoints((Attribute)i));
	CHECK(a.bloodlineAttribute() == b.bloodlineAttribute());
}
