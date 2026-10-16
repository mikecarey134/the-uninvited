#include "doctest.h"
#include "CharacterSheet.h"

using namespace character;

static const Profession chef{ "Chef", { { COOKING, 30 } }, {} };

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

static CharacterSheet surviving(int endurance, int water, int hunger, int health)
{
	Aegis aegis;
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
		aegis.rolled[i] = 20;
	aegis.rolled[(int)ENDURANCE] = endurance;
	std::string text = CharacterSheet::create("Cal", Gender::Male, chef, aegis).toText();
	auto set = [&text](const std::string& key, int value) {
		std::string::size_type at = text.find("\n" + key + " = ");
		REQUIRE(at != std::string::npos);
		std::string::size_type end = text.find('\n', at + 1);
		text.replace(at + 1, end - at - 1, key + " = " + std::to_string(value));
	};
	set("water", water);
	set("hunger", hunger);
	set("health", health);
	std::string problem;
	CharacterSheet loaded = CharacterSheet::fromText(text, problem);
	REQUIRE(problem.empty());
	return loaded;
}

TEST_CASE("each survival tick drains water and hunger by the base rate slowed by Endurance, and leaves sanity alone")
{
	CharacterSheet cal = surviving(20, 100, 100, 100);
	cal.survivalTick();
	float drain = SURVIVAL_BASE_DRAIN / (1.0f + 20 * 0.02f);
	CHECK(cal.water() == doctest::Approx(100.0f - drain));
	CHECK(cal.hunger() == doctest::Approx(100.0f - drain));
	CHECK(cal.sanity() == doctest::Approx(100.0f));
	CHECK(cal.health() == 100);

	CharacterSheet tough = surviving(50, 100, 100, 100);
	tough.survivalTick();
	CHECK(tough.water() == doctest::Approx(100.0f - SURVIVAL_BASE_DRAIN / 2.0f));
}

TEST_CASE("a full bar lasts eight to ten minutes of one-second ticks at Endurance 20")
{
	CharacterSheet cal = surviving(20, 100, 100, 100);
	int ticks = 0;
	while (cal.water() > 0.0f && ticks < 10000)
	{
		cal.survivalTick();
		++ticks;
	}
	CHECK(ticks >= 8 * 60);
	CHECK(ticks <= 10 * 60);
	CHECK(cal.water() == 0.0f);
	CHECK(cal.hunger() == 0.0f);
}

TEST_CASE("health holds while both meters have something left, then drains per empty meter, twice as fast with both empty")
{
	CharacterSheet fed = surviving(20, 1, 1, 100);
	fed.survivalTick();
	CHECK(fed.health() == 100);

	CharacterSheet thirsty = surviving(20, 0, 100, 100);
	thirsty.survivalTick();
	CHECK(thirsty.health() == 100 - SURVIVAL_HEALTH_DRAIN);
	CHECK(thirsty.water() == 0.0f);

	CharacterSheet starving = surviving(20, 100, 0, 100);
	starving.survivalTick();
	CHECK(starving.health() == 100 - SURVIVAL_HEALTH_DRAIN);

	CharacterSheet both = surviving(20, 0, 0, 100);
	both.survivalTick();
	CHECK(both.health() == 100 - 2 * SURVIVAL_HEALTH_DRAIN);

	float meter = SURVIVAL_BASE_DRAIN / (1.0f + 20 * 0.02f);
	CHECK((float)SURVIVAL_HEALTH_DRAIN > 3.0f * meter);
}

TEST_CASE("health stops at zero and the character is then dead")
{
	CharacterSheet cal = surviving(20, 0, 0, 3);
	CHECK_FALSE(cal.isDead());
	cal.survivalTick();
	CHECK(cal.health() == 1);
	CHECK_FALSE(cal.isDead());
	cal.survivalTick();
	CHECK(cal.health() == 0);
	CHECK(cal.isDead());
	cal.survivalTick();
	CHECK(cal.health() == 0);
}
