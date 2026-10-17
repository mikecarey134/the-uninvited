#include "doctest.h"
#include "CharacterSheet.h"

using namespace character;

static const Profession bartender{ "Bartender",
	{ { SPEECH, 10 }, { COOKING, 10 }, { CHEMISTRY, 10 } }, { "shaker", "towel" } };

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
		CHECK(a.rolledPoints(i) == b.rolledPoints(i));
		CHECK(a.placedPoints(i) == b.placedPoints(i));
		CHECK(a.attribute(i) == b.attribute(i));
	}
	CHECK(a.hasBloodline() == b.hasBloodline());
	CHECK(a.bloodlineName() == b.bloodlineName());
	CHECK(a.pointPool() == b.pointPool());
	CHECK(a.unplacedPoints() == b.unplacedPoints());
	if (a.hasBloodline())
		CHECK(a.bloodlineAttribute() == b.bloodlineAttribute());
	for (int i = 0; i < SKILL_COUNT; ++i)
	{
		CHECK(a.skill(i) == b.skill(i));
		CHECK(a.experience(i) == b.experience(i));
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
	REQUIRE(sheet.placePoints(GUILE, 7));
	REQUIRE(sheet.placePoints(STRENGTH, 4));
	sheet.addExperience(SPEECH, 123.456789);
	sheet.addExperience(SMITHING, 1.0 / 3.0);
	for (int i = 0; i < 7; ++i)
		sheet.survivalTick(1234.5);
	REQUIRE(sheet.water() != (float)(int)sheet.water());
	checkSame(sheet, reloaded(sheet));
}

TEST_CASE("empty meters come back with their flags set")
{
	std::mt19937 random(2);
	CharacterSheet sheet = CharacterSheet::create("Jo", Gender::Female, bartender, random, 1000000);
	for (int i = 0; i < 1000; ++i)
		sheet.survivalTick(GAME_DAY_SECONDS);
	std::string text = sheet.toText();
	std::string::size_type at = text.find("\nsanity = ");
	REQUIRE(at != std::string::npos);
	text.replace(at + 1, text.find('\n', at + 1) - at - 1, "sanity = 0");
	std::string problem;
	CharacterSheet empty = CharacterSheet::fromText(text, problem);
	REQUIRE(problem.empty());
	REQUIRE(empty.isDehydrated());
	REQUIRE(empty.isStarving());
	REQUIRE(empty.isInsane());
	checkSame(empty, reloaded(empty));
}

TEST_CASE("a profession without items round trips")
{
	std::mt19937 random(3);
	CharacterSheet sheet = CharacterSheet::create("Kim", Gender::Male, Profession{ "Doctor", { { MEDICINE, 30 } }, {} }, random);
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

static Carried packed()
{
	Carried carried;
	carried.columns = 8;
	carried.rows = 7;
	carried.items = { CarriedItem{ "knife", 0, 0, false, 1 }, CarriedItem{ "bandage", 2, 0, false, 3 }, CarriedItem{ "body_armor", 4, 5, true, 1 } };
	carried.worn = { WornItem{ "main_hand", "pistol" }, WornItem{ "back", "small_backpack" } };
	return carried;
}

TEST_CASE("the carried items and worn gear go into the character file as readable inv lines and come back the same")
{
	std::mt19937 random(5);
	CharacterSheet sheet = CharacterSheet::create("Ivy", Gender::Male, bartender, random);
	sheet.setCarried(packed());
	std::string text = sheet.toText();
	CHECK(text.find("\ninv.grid = 8,7\n") != std::string::npos);
	CHECK(text.find("\ninv.item.0 = knife,0,0,false,1\n") != std::string::npos);
	CHECK(text.find("\ninv.item.1 = bandage,2,0,false,3\n") != std::string::npos);
	CHECK(text.find("\ninv.item.2 = body_armor,4,5,true,1\n") != std::string::npos);
	CHECK(text.find("\ninv.equip.main_hand = pistol\n") != std::string::npos);
	CHECK(text.find("\ninv.equip.back = small_backpack\n") != std::string::npos);
	CharacterSheet back = reloaded(sheet);
	checkSame(sheet, back);
	const Carried& carried = back.carried();
	CHECK(carried.columns == 8);
	CHECK(carried.rows == 7);
	REQUIRE(carried.items.size() == 3);
	CHECK(carried.items[2].type == "body_armor");
	CHECK(carried.items[2].column == 4);
	CHECK(carried.items[2].row == 5);
	CHECK(carried.items[2].rotated);
	CHECK_FALSE(carried.items[1].rotated);
	CHECK(carried.items[1].count == 3);
	REQUIRE(carried.worn.size() == 2);
	CHECK(carried.worn[0].slot == "main_hand");
	CHECK(carried.worn[0].type == "pistol");
	CHECK(carried.worn[1].slot == "back");
	CHECK(carried.problems.empty());
}

TEST_CASE("a character file from before inventories loads with nothing carried, and a fresh character writes no inv lines")
{
	std::mt19937 random(6);
	CharacterSheet sheet = CharacterSheet::create("Ivy", Gender::Male, bartender, random);
	std::string text = sheet.toText();
	CHECK(text.find("inv.") == std::string::npos);
	std::string problem;
	CharacterSheet loaded = CharacterSheet::fromText(text, problem);
	REQUIRE(problem.empty());
	CHECK(loaded.carried().items.empty());
	CHECK(loaded.carried().worn.empty());
	CHECK(loaded.carried().columns == 0);
	CHECK(loaded.carried().problems.empty());
}

TEST_CASE("a garbled inv line is set aside with a note while the rest of the character and inventory still load")
{
	std::mt19937 random(7);
	CharacterSheet sheet = CharacterSheet::create("Ivy", Gender::Male, bartender, random);
	sheet.setCarried(packed());
	std::string text = sheet.toText();
	text += "inv.item.3 = shotgun,1,x,false,1\ninv.item.4 = pills,1\ninv.item.5 = pills,1,1,maybe,2\ninv.equip. = knife\ninv.item.6 = key,0,0,false,0\ninv.wobble = 1\n";
	std::string problem;
	CharacterSheet loaded = CharacterSheet::fromText(text, problem);
	REQUIRE(problem.empty());
	CHECK(loaded.name() == "Ivy");
	CHECK(loaded.carried().items.size() == 3);
	CHECK(loaded.carried().worn.size() == 2);
	REQUIRE(loaded.carried().problems.size() == 6);
	CHECK(loaded.carried().problems[0].find("inv.item.3") != std::string::npos);
	CHECK(loaded.carried().problems[4].find("inv.item.6") != std::string::npos);
	CHECK(loaded.carried().problems[5].find("inv.wobble") != std::string::npos);
}
