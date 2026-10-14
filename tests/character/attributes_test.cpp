#include "doctest.h"
#include "CharacterSheet.h"
#include <algorithm>
#include <set>

using namespace character;

static const Profession nurse{ "Nurse", { { Skill::Medicine, 20 }, { Skill::Chemistry, 10 } }, {} };

static CharacterSheet rolled(unsigned seed, int bloodlineOneIn = 1000000)
{
	std::mt19937 random(seed);
	return CharacterSheet::create("Ada", Gender::Female, nurse, random, bloodlineOneIn);
}

TEST_CASE("the random attribute points always sum to exactly 100")
{
	for (unsigned seed = 0; seed < 300; ++seed)
	{
		CharacterSheet sheet = rolled(seed);
		int total = 0;
		for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
		{
			CHECK(sheet.rolledPoints((Attribute)i) >= 1);
			total += sheet.rolledPoints((Attribute)i);
		}
		CHECK(total == 100);
	}
}

TEST_CASE("the random points spread across all five attributes")
{
	int totals[ATTRIBUTE_COUNT] = {};
	std::set<int> strengths;
	for (unsigned seed = 0; seed < 300; ++seed)
	{
		CharacterSheet sheet = rolled(seed);
		for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
			totals[i] += sheet.rolledPoints((Attribute)i);
		strengths.insert(sheet.rolledPoints(Attribute::Strength));
	}
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
	{
		INFO(attributeName((Attribute)i));
		CHECK(totals[i] / 300.0 == doctest::Approx(20.0).epsilon(0.1));
	}
	CHECK(strengths.size() > 5);
}

TEST_CASE("rolls are uneven, so characters differ in what they are good at")
{
	double spread = 0.0;
	for (unsigned seed = 0; seed < 300; ++seed)
	{
		CharacterSheet sheet = rolled(seed);
		int low = 100;
		int high = 0;
		for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
		{
			low = std::min(low, sheet.rolledPoints((Attribute)i));
			high = std::max(high, sheet.rolledPoints((Attribute)i));
		}
		spread += high - low;
	}
	CHECK(spread / 300.0 > 14.0);
}

TEST_CASE("the point pool is 20 plus one for every 5 rolled Intelligence")
{
	Aegis aegis;
	CHECK(aegis.pool() == 20);
	aegis.rolled[(int)Attribute::Intelligence] = 4;
	CHECK(aegis.pool() == 20);
	aegis.rolled[(int)Attribute::Intelligence] = 23;
	CHECK(aegis.pool() == 24);
	aegis.rolled[(int)Attribute::Intelligence] = 50;
	CHECK(aegis.pool() == 30);
	aegis.rolled[(int)Attribute::Strength] = 60;
	CHECK(aegis.pool() == 30);
}

TEST_CASE("points placed in Intelligence do not grow the pool")
{
	Aegis aegis;
	aegis.rolled[(int)Attribute::Intelligence] = 23;
	REQUIRE(aegis.place(Attribute::Intelligence, 10));
	CHECK(aegis.pool() == 24);
	CHECK(aegis.unplaced() == 14);
	CHECK(aegis.value(Attribute::Intelligence) == 33);
}

TEST_CASE("a new character has its whole pool left to place and none placed")
{
	CharacterSheet sheet = rolled(1);
	CHECK(sheet.unplacedPoints() == sheet.pointPool());
	CHECK(sheet.pointPool() == 20 + sheet.rolledPoints(Attribute::Intelligence) / 5);
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
	{
		CHECK(sheet.placedPoints((Attribute)i) == 0);
		CHECK(sheet.attribute((Attribute)i) == sheet.rolledPoints((Attribute)i));
	}
}

TEST_CASE("placed points add to the attribute and come out of the pool")
{
	CharacterSheet sheet = rolled(2);
	int pool = sheet.pointPool();
	int before = sheet.attribute(Attribute::Strength);
	CHECK(sheet.placePoints(Attribute::Strength, 12));
	CHECK(sheet.placedPoints(Attribute::Strength) == 12);
	CHECK(sheet.attribute(Attribute::Strength) == before + 12);
	CHECK(sheet.unplacedPoints() == pool - 12);
	CHECK(sheet.placePoints(Attribute::Guile, pool - 12));
	CHECK(sheet.unplacedPoints() == 0);
}

TEST_CASE("placing more points than are left is refused and changes nothing")
{
	CharacterSheet sheet = rolled(3);
	int pool = sheet.pointPool();
	REQUIRE(sheet.placePoints(Attribute::Agility, pool - 5));
	int agility = sheet.attribute(Attribute::Agility);
	int endurance = sheet.attribute(Attribute::Endurance);
	CHECK_FALSE(sheet.placePoints(Attribute::Endurance, 6));
	CHECK(sheet.attribute(Attribute::Endurance) == endurance);
	CHECK(sheet.attribute(Attribute::Agility) == agility);
	CHECK(sheet.unplacedPoints() == 5);
	CHECK(sheet.placePoints(Attribute::Endurance, 5));
	CHECK_FALSE(sheet.placePoints(Attribute::Endurance, 1));
	CHECK(sheet.unplacedPoints() == 0);
}

TEST_CASE("placing zero or negative points is refused")
{
	CharacterSheet sheet = rolled(4);
	CHECK_FALSE(sheet.placePoints(Attribute::Guile, 0));
	CHECK_FALSE(sheet.placePoints(Attribute::Guile, -5));
	CHECK(sheet.unplacedPoints() == sheet.pointPool());
	CHECK(sheet.placedPoints(Attribute::Guile) == 0);
}

TEST_CASE("removing placed points gives them back, but never below the rolled value")
{
	CharacterSheet sheet = rolled(6);
	int rolledGuile = sheet.rolledPoints(Attribute::Guile);
	REQUIRE(sheet.placePoints(Attribute::Guile, 3));
	CHECK(sheet.removePoints(Attribute::Guile, 2));
	CHECK(sheet.placedPoints(Attribute::Guile) == 1);
	CHECK(sheet.unplacedPoints() == sheet.pointPool() - 1);
	CHECK_FALSE(sheet.removePoints(Attribute::Guile, 2));
	CHECK_FALSE(sheet.removePoints(Attribute::Guile, 0));
	CHECK(sheet.removePoints(Attribute::Guile, 1));
	CHECK_FALSE(sheet.removePoints(Attribute::Guile, 1));
	CHECK(sheet.attribute(Attribute::Guile) == rolledGuile);
	CHECK(sheet.unplacedPoints() == sheet.pointPool());
}

TEST_CASE("a rolled bloodline carries a placeholder name until bloodlines exist, and no bloodline has no name")
{
	CharacterSheet blessed = rolled(7, 1);
	REQUIRE(blessed.hasBloodline());
	CHECK(blessed.bloodlineName() == "[Placeholder]");
	CharacterSheet plain = rolled(7);
	REQUIRE_FALSE(plain.hasBloodline());
	CHECK(plain.bloodlineName().empty());
}

TEST_CASE("a bloodline multiplies one attribute by ten after every point is placed")
{
	CharacterSheet sheet = rolled(5, 1);
	REQUIRE(sheet.hasBloodline());
	Attribute boosted = sheet.bloodlineAttribute();
	REQUIRE(sheet.placePoints(boosted, 7));
	CHECK(sheet.attribute(boosted) == (sheet.rolledPoints(boosted) + 7) * 10);
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
	{
		Attribute other = (Attribute)i;
		if (other == boosted)
			continue;
		REQUIRE(sheet.placePoints(other, 1));
		CHECK(sheet.attribute(other) == sheet.rolledPoints(other) + 1);
	}
}

TEST_CASE("the bloodline attribute is chosen at random")
{
	std::set<int> chosen;
	for (unsigned seed = 0; seed < 100; ++seed)
		chosen.insert((int)rolled(seed, 1).bloodlineAttribute());
	CHECK(chosen.size() == (size_t)ATTRIBUTE_COUNT);
}

TEST_CASE("bloodlines are rare at the default odds")
{
	int count = 0;
	for (unsigned seed = 0; seed < 50000; ++seed)
	{
		std::mt19937 random(seed);
		if (CharacterSheet::create("Ada", Gender::Female, nurse, random).hasBloodline())
			++count;
	}
	CHECK(count > 25);
	CHECK(count < 80);
}

TEST_CASE("without a bloodline every attribute is just rolled plus placed")
{
	CharacterSheet sheet = rolled(6);
	CHECK_FALSE(sheet.hasBloodline());
	REQUIRE(sheet.placePoints(Attribute::Intelligence, 20));
	CHECK(sheet.attribute(Attribute::Intelligence) == sheet.rolledPoints(Attribute::Intelligence) + 20);
}
