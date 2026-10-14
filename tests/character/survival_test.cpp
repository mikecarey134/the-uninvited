#include "doctest.h"
#include "CharacterSheet.h"

using namespace character;

static const Profession chef{ "Chef", { { Skill::Cooking, 30 } }, {} };
static const float HOUR = 3600.0f;

static CharacterSheet sheet(unsigned seed = 11)
{
	std::mt19937 random(seed);
	return CharacterSheet::create("Cal", Gender::Male, chef, random, 1000000);
}

TEST_CASE("meters start full with no flags set")
{
	CharacterSheet cal = sheet();
	CHECK(cal.water() == 100.0f);
	CHECK(cal.hunger() == 100.0f);
	CHECK(cal.sanity() == 100.0f);
	CHECK_FALSE(cal.isDehydrated());
	CHECK_FALSE(cal.isStarving());
	CHECK_FALSE(cal.isInsane());
}

TEST_CASE("one game hour drains each meter by its rate, slowed by the governing attribute")
{
	CharacterSheet cal = sheet();
	int endurance = cal.attribute(Attribute::Endurance);
	int intelligence = cal.attribute(Attribute::Intelligence);
	cal.tick(HOUR, 1.0f);
	CHECK(cal.water() == doctest::Approx(100.0f - 100.0f / WATER_HOURS / (1.0f + endurance / 100.0f)));
	CHECK(cal.hunger() == doctest::Approx(100.0f - 100.0f / HUNGER_HOURS / (1.0f + endurance / 100.0f)));
	CHECK(cal.sanity() == doctest::Approx(100.0f - 100.0f / SANITY_HOURS / (1.0f + intelligence / 100.0f)));
	CHECK(drainSlowdown(0) == doctest::Approx(1.0f));
	CHECK(drainSlowdown(100) == doctest::Approx(2.0f));
}

TEST_CASE("time compression scales real seconds into game time")
{
	CharacterSheet slow = sheet();
	CharacterSheet fast = sheet();
	slow.tick(HOUR, 1.0f);
	fast.tick(60.0f, 60.0f);
	CHECK(fast.water() == doctest::Approx(slow.water()));
	CHECK(fast.hunger() == doctest::Approx(slow.hunger()));
	CHECK(fast.sanity() == doctest::Approx(slow.sanity()));
}

TEST_CASE("Endurance slows water and hunger but not sanity")
{
	CharacterSheet plain = sheet();
	CharacterSheet tough = sheet();
	REQUIRE(tough.placePoints(Attribute::Endurance, 20));
	plain.tick(10.0f * HOUR, 1.0f);
	tough.tick(10.0f * HOUR, 1.0f);
	CHECK(tough.water() > plain.water());
	CHECK(tough.hunger() > plain.hunger());
	CHECK(tough.sanity() == doctest::Approx(plain.sanity()));
}

TEST_CASE("Intelligence slows sanity but not water or hunger")
{
	CharacterSheet plain = sheet();
	CharacterSheet clever = sheet();
	REQUIRE(clever.placePoints(Attribute::Intelligence, 20));
	plain.tick(10.0f * HOUR, 1.0f);
	clever.tick(10.0f * HOUR, 1.0f);
	CHECK(clever.sanity() > plain.sanity());
	CHECK(clever.water() == doctest::Approx(plain.water()));
	CHECK(clever.hunger() == doctest::Approx(plain.hunger()));
}

TEST_CASE("each flag sets when its meter reaches zero and the meter stops at zero")
{
	CharacterSheet cal = sheet();
	REQUIRE(cal.attribute(Attribute::Endurance) < 90);
	cal.tick((HUNGER_HOURS - 4.0f) * HOUR, 1.0f);
	CHECK(cal.water() == 0.0f);
	CHECK(cal.isDehydrated());
	CHECK(cal.hunger() > 0.0f);
	CHECK_FALSE(cal.isStarving());
	CHECK_FALSE(cal.isInsane());

	cal.tick(SANITY_HOURS * 3.0f * HOUR, 1.0f);
	CHECK(cal.hunger() == 0.0f);
	CHECK(cal.sanity() == 0.0f);
	CHECK(cal.isDehydrated());
	CHECK(cal.isStarving());
	CHECK(cal.isInsane());
}

TEST_CASE("a zero or negative tick changes nothing")
{
	CharacterSheet cal = sheet();
	cal.tick(0.0f, 1.0f);
	cal.tick(-HOUR, 1.0f);
	cal.tick(HOUR, 0.0f);
	CHECK(cal.water() == 100.0f);
	CHECK(cal.hunger() == 100.0f);
	CHECK(cal.sanity() == 100.0f);
}
