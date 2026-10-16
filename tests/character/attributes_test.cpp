#include "doctest.h"
#include "CharacterSheet.h"
#include <algorithm>
#include <set>

using namespace character;

static const Profession nurse{ "Nurse", { { MEDICINE, 20 }, { CHEMISTRY, 10 } }, {} };

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
			CHECK(sheet.rolledPoints(i) >= 1);
			total += sheet.rolledPoints(i);
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
			totals[i] += sheet.rolledPoints(i);
		strengths.insert(sheet.rolledPoints(STRENGTH));
	}
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
	{
		INFO(attributeName(i));
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
			low = std::min(low, sheet.rolledPoints(i));
			high = std::max(high, sheet.rolledPoints(i));
		}
		spread += high - low;
	}
	CHECK(spread / 300.0 > 14.0);
}

TEST_CASE("the point pool is 20 plus one for every 5 rolled Intelligence")
{
	Aegis aegis;
	CHECK(aegis.pool() == 20);
	aegis.rolled[(int)INTELLIGENCE] = 4;
	CHECK(aegis.pool() == 20);
	aegis.rolled[(int)INTELLIGENCE] = 23;
	CHECK(aegis.pool() == 24);
	aegis.rolled[(int)INTELLIGENCE] = 50;
	CHECK(aegis.pool() == 30);
	aegis.rolled[(int)STRENGTH] = 60;
	CHECK(aegis.pool() == 30);
}

TEST_CASE("points placed in Intelligence do not grow the pool")
{
	Aegis aegis;
	aegis.rolled[(int)INTELLIGENCE] = 23;
	REQUIRE(aegis.place(INTELLIGENCE, 10));
	CHECK(aegis.pool() == 24);
	CHECK(aegis.unplaced() == 14);
	CHECK(aegis.value(INTELLIGENCE) == 33);
}

TEST_CASE("a new character has its whole pool left to place and none placed")
{
	CharacterSheet sheet = rolled(1);
	CHECK(sheet.unplacedPoints() == sheet.pointPool());
	CHECK(sheet.pointPool() == 20 + sheet.rolledPoints(INTELLIGENCE) / 5);
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
	{
		CHECK(sheet.placedPoints(i) == 0);
		CHECK(sheet.attribute(i) == sheet.rolledPoints(i));
	}
}

TEST_CASE("placed points add to the attribute and come out of the pool")
{
	CharacterSheet sheet = rolled(2);
	int pool = sheet.pointPool();
	int before = sheet.attribute(STRENGTH);
	CHECK(sheet.placePoints(STRENGTH, 12));
	CHECK(sheet.placedPoints(STRENGTH) == 12);
	CHECK(sheet.attribute(STRENGTH) == before + 12);
	CHECK(sheet.unplacedPoints() == pool - 12);
	CHECK(sheet.placePoints(GUILE, pool - 12));
	CHECK(sheet.unplacedPoints() == 0);
}

TEST_CASE("placing more points than are left is refused and changes nothing")
{
	CharacterSheet sheet = rolled(3);
	int pool = sheet.pointPool();
	REQUIRE(sheet.placePoints(AGILITY, pool - 5));
	int agility = sheet.attribute(AGILITY);
	int endurance = sheet.attribute(ENDURANCE);
	CHECK_FALSE(sheet.placePoints(ENDURANCE, 6));
	CHECK(sheet.attribute(ENDURANCE) == endurance);
	CHECK(sheet.attribute(AGILITY) == agility);
	CHECK(sheet.unplacedPoints() == 5);
	CHECK(sheet.placePoints(ENDURANCE, 5));
	CHECK_FALSE(sheet.placePoints(ENDURANCE, 1));
	CHECK(sheet.unplacedPoints() == 0);
}

TEST_CASE("placing zero or negative points is refused")
{
	CharacterSheet sheet = rolled(4);
	CHECK_FALSE(sheet.placePoints(GUILE, 0));
	CHECK_FALSE(sheet.placePoints(GUILE, -5));
	CHECK(sheet.unplacedPoints() == sheet.pointPool());
	CHECK(sheet.placedPoints(GUILE) == 0);
}

TEST_CASE("removing placed points gives them back, but never below the rolled value")
{
	CharacterSheet sheet = rolled(6);
	int rolledGuile = sheet.rolledPoints(GUILE);
	REQUIRE(sheet.placePoints(GUILE, 3));
	CHECK(sheet.removePoints(GUILE, 2));
	CHECK(sheet.placedPoints(GUILE) == 1);
	CHECK(sheet.unplacedPoints() == sheet.pointPool() - 1);
	CHECK_FALSE(sheet.removePoints(GUILE, 2));
	CHECK_FALSE(sheet.removePoints(GUILE, 0));
	CHECK(sheet.removePoints(GUILE, 1));
	CHECK_FALSE(sheet.removePoints(GUILE, 1));
	CHECK(sheet.attribute(GUILE) == rolledGuile);
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
	int boosted = sheet.bloodlineAttribute();
	REQUIRE(sheet.placePoints(boosted, 7));
	CHECK(sheet.attribute(boosted) == (sheet.rolledPoints(boosted) + 7) * 10);
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
	{
		int other = i;
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
	REQUIRE(sheet.placePoints(INTELLIGENCE, 20));
	CHECK(sheet.attribute(INTELLIGENCE) == sheet.rolledPoints(INTELLIGENCE) + 20);
}
