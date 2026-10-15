#include "doctest.h"
#include "CharacterSheet.h"

using namespace character;

static const Profession bartender{ "Bartender",
	{ { Skill::Speech, 10 }, { Skill::Cooking, 10 }, { Skill::Chemistry, 10 } }, { "shaker", "towel" } };

static void checkSame(const CharacterSheet& a, const CharacterSheet& b)
{
	CHECK(a.name() == b.name());
	CHECK(a.gender() == b.gender());
	CHECK(a.outfit() == b.outfit());
	CHECK(a.age() == b.age());
	CHECK(a.profession().name == b.profession().name);
	CHECK(a.profession().skillBonuses == b.profession().skillBonuses);
	CHECK(a.profession().startingItems == b.profession().startingItems);
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
	{
		CHECK(a.rolledPoints((Attribute)i) == b.rolledPoints((Attribute)i));
		CHECK(a.placedPoints((Attribute)i) == b.placedPoints((Attribute)i));
		CHECK(a.attribute((Attribute)i) == b.attribute((Attribute)i));
	}
	CHECK(a.hasBloodline() == b.hasBloodline());
	CHECK(a.bloodlineName() == b.bloodlineName());
	CHECK(a.pointPool() == b.pointPool());
	CHECK(a.unplacedPoints() == b.unplacedPoints());
	if (a.hasBloodline())
		CHECK(a.bloodlineAttribute() == b.bloodlineAttribute());
	for (int i = 0; i < SKILL_COUNT; ++i)
	{
		CHECK(a.skill((Skill)i) == b.skill((Skill)i));
		CHECK(a.experience((Skill)i) == b.experience((Skill)i));
	}
	CHECK(a.water() == b.water());
	CHECK(a.hunger() == b.hunger());
	CHECK(a.sanity() == b.sanity());
	CHECK(a.isDehydrated() == b.isDehydrated());
	CHECK(a.isStarving() == b.isStarving());
	CHECK(a.isInsane() == b.isInsane());
	CHECK(a.toText() == b.toText());
}

static CharacterSheet reloaded(const CharacterSheet& sheet)
{
	std::string problem;
	CharacterSheet loaded = CharacterSheet::fromText(sheet.toText(), problem);
	INFO(problem);
	REQUIRE(problem.empty());
	return loaded;
}

TEST_CASE("a fresh character round trips through its text")
{
	std::mt19937 random(5);
	CharacterSheet sheet = CharacterSheet::create("Gus Ward", Gender::Female, bartender, random, 1000000);
	checkSame(sheet, reloaded(sheet));
}

TEST_CASE("a played character round trips exactly, fractions and bloodline included")
{
	std::mt19937 random(11);
	CharacterSheet sheet = CharacterSheet::create("Ivy", Gender::Male, bartender, random, 1);
	REQUIRE(sheet.hasBloodline());
	sheet.setAge(33.3f);
	REQUIRE(sheet.placePoints(Attribute::Guile, 7));
	REQUIRE(sheet.placePoints(Attribute::Strength, 4));
	sheet.addExperience(Skill::Speech, 123.456789);
	sheet.addExperience(Skill::Smithing, 1.0 / 3.0);
	sheet.tick(1.0f, 3600.0f * 50.0f);
	checkSame(sheet, reloaded(sheet));
}

TEST_CASE("empty meters come back with their flags set")
{
	std::mt19937 random(2);
	CharacterSheet sheet = CharacterSheet::create("Jo", Gender::Female, bartender, random, 1000000);
	sheet.tick(1.0f, 3600.0f * 10000.0f);
	REQUIRE(sheet.isDehydrated());
	REQUIRE(sheet.isStarving());
	REQUIRE(sheet.isInsane());
	checkSame(sheet, reloaded(sheet));
}

TEST_CASE("a profession without items round trips")
{
	std::mt19937 random(3);
	CharacterSheet sheet = CharacterSheet::create("Kim", Gender::Male, Profession{ "Doctor", { { Skill::Medicine, 30 } }, {} }, random);
	CharacterSheet loaded = reloaded(sheet);
	CHECK(loaded.profession().startingItems.empty());
	checkSame(sheet, loaded);
}

TEST_CASE("broken character text is reported, not loaded")
{
	std::mt19937 random(4);
	std::string good = CharacterSheet::create("Lu", Gender::Male, bartender, random, 1000000).toText();
	std::string problem;

	CharacterSheet::fromText("", problem);
	CHECK(problem == "missing name");

	std::string badGender = good;
	badGender.replace(badGender.find("Male"), 4, "Moose");
	CharacterSheet::fromText(badGender, problem);
	CHECK(problem == "bad gender: Moose");

	std::string badSkill = good;
	badSkill.replace(badSkill.find("Speech:10"), 6, "Poetry");
	CharacterSheet::fromText(badSkill, problem);
	CHECK(problem == "bad profession skill: Poetry:10");

	std::string badBloodline = good;
	badBloodline.replace(badBloodline.find("bloodline = none"), 16, "bloodline = Luck");
	CharacterSheet::fromText(badBloodline, problem);
	CHECK(problem == "bad bloodline: Luck");

	CharacterSheet::fromText(good, problem);
	CHECK(problem.empty());
}

TEST_CASE("the chosen outfit round trips, saves from before outfits load as the first one, and a negative outfit is refused")
{
	CharacterSheet dressed = CharacterSheet::create("Ada", Gender::Female, bartender);
	dressed.setOutfit(2);
	std::string problem;
	CharacterSheet loaded = CharacterSheet::fromText(dressed.toText(), problem);
	CHECK(problem.empty());
	CHECK(loaded.outfit() == 2);

	std::string old = dressed.toText();
	std::string::size_type at = old.find("outfit = 2\n");
	REQUIRE(at != std::string::npos);
	old.erase(at, std::string("outfit = 2\n").size());
	CharacterSheet older = CharacterSheet::fromText(old, problem);
	CHECK(problem.empty());
	CHECK(older.outfit() == 0);

	std::string negative = dressed.toText();
	negative.replace(negative.find("outfit = 2"), 10, "outfit = -1");
	CharacterSheet::fromText(negative, problem);
	CHECK(problem == "bad outfit: -1");
}

TEST_CASE("a character starts at full health, keeps it through a save, and saves from before health load at full")
{
	CharacterSheet sheet = CharacterSheet::create("Ada", Gender::Female, bartender);
	CHECK(sheet.maxHealth() == 100);
	CHECK(sheet.health() == 100);
	std::string text = sheet.toText();
	std::string::size_type at = text.find("health = 100");
	REQUIRE(at != std::string::npos);
	std::string hurt = text;
	hurt.replace(at, 12, "health = 37");
	std::string problem;
	CHECK(CharacterSheet::fromText(hurt, problem).health() == 37);
	CHECK(problem.empty());
	std::string old = text;
	old.erase(at, std::string("health = 100\n").size());
	CHECK(CharacterSheet::fromText(old, problem).health() == 100);
	CHECK(problem.empty());
	std::string broken = text;
	broken.replace(at, 12, "health = 150");
	CharacterSheet::fromText(broken, problem);
	CHECK(problem == "bad health: 150");
}
