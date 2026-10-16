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

TEST_CASE("drain is the base rate over one plus Endurance over twenty, so Endurance 20 halves it")
{
	const double day = GAME_DAY_SECONDS;
	CHECK(GAME_DAY_SECONDS == 86400.0);
	const double waterBase = 100.0 / (3.0 * day) * 2.0;
	const double hungerBase = 100.0 / (7.0 * day) * 2.0;
	const int endurances[] = { 20, 40, 60, 100, 200 };
	const double divisors[] = { 2.0, 3.0, 4.0, 6.0, 11.0 };
	for (int i = 0; i < 5; ++i)
	{
		INFO(endurances[i]);
		CHECK(waterDrainPerSecond(endurances[i]) == doctest::Approx(waterBase / divisors[i]).epsilon(1e-9));
		CHECK(hungerDrainPerSecond(endurances[i]) == doctest::Approx(hungerBase / divisors[i]).epsilon(1e-9));
	}
	CHECK(waterDrainPerSecond(0) == doctest::Approx(waterBase).epsilon(1e-9));
	CHECK(waterDrainPerSecond(30) == doctest::Approx(waterBase / 2.5).epsilon(1e-9));
}

TEST_CASE("each tick drains water and hunger for the game-seconds it covers, slowed by Endurance, and leaves sanity alone")
{
	CharacterSheet cal = surviving(20, 100, 100, 100);
	cal.survivalTick(3600.0);
	CHECK(cal.water() == doctest::Approx(100.0 - waterDrainPerSecond(20) * 3600.0).epsilon(1e-6));
	CHECK(cal.hunger() == doctest::Approx(100.0 - hungerDrainPerSecond(20) * 3600.0).epsilon(1e-6));
	CHECK(cal.water() < cal.hunger());
	CHECK(cal.sanity() == doctest::Approx(100.0f));
	CHECK(cal.health() == 100);

	CharacterSheet tough = surviving(200, 100, 100, 100);
	tough.survivalTick(3600.0);
	CHECK(tough.water() == doctest::Approx(100.0 - waterDrainPerSecond(200) * 3600.0).epsilon(1e-6));
	CHECK(tough.water() > cal.water());
}

TEST_CASE("at Endurance 20 water empties in three game-days and hunger in seven, one-second ticks or hour-long ones")
{
	for (double step : { 1.0, 3600.0 })
	{
		INFO(step);
		CharacterSheet cal = surviving(20, 100, 100, 100);
		double elapsed = 0.0;
		while (cal.water() > 0.0f && elapsed < 10.0 * GAME_DAY_SECONDS)
		{
			cal.survivalTick(step);
			elapsed += step;
		}
		CHECK(elapsed > 3.0 * GAME_DAY_SECONDS - step);
		CHECK(elapsed <= 3.0 * GAME_DAY_SECONDS + step);
		CHECK(cal.hunger() > 0.0f);
		CharacterSheet fed = surviving(20, 100, 100, 100);
		elapsed = 0.0;
		while (fed.hunger() > 0.0f && elapsed < 10.0 * GAME_DAY_SECONDS)
		{
			fed.survivalTick(step);
			elapsed += step;
		}
		CHECK(elapsed > 7.0 * GAME_DAY_SECONDS - step);
		CHECK(elapsed <= 7.0 * GAME_DAY_SECONDS + step);
	}
}

TEST_CASE("an empty meter drains health at that meter's own scaled rate, and both empty drain it at the two rates added")
{
	CharacterSheet fed = surviving(20, 1, 1, 100);
	fed.survivalTick(1000.0);
	CHECK(fed.health() == 100);

	const double hour = 3600.0;
	CharacterSheet thirsty = surviving(20, 0, 100, 100);
	thirsty.survivalTick(10.0 * hour);
	CHECK(thirsty.health() == 100 - (int)(waterDrainPerSecond(20) * 10.0 * hour));
	CHECK(thirsty.health() == 87);
	CHECK(thirsty.water() == 0.0f);

	CharacterSheet starving = surviving(20, 100, 0, 100);
	starving.survivalTick(10.0 * hour);
	CHECK(starving.health() == 100 - (int)(hungerDrainPerSecond(20) * 10.0 * hour));
	CHECK(starving.health() == 95);

	CharacterSheet both = surviving(20, 0, 0, 100);
	both.survivalTick(10.0 * hour);
	CHECK(both.health() == 100 - (int)((waterDrainPerSecond(20) + hungerDrainPerSecond(20)) * 10.0 * hour));
	CHECK(both.health() == 81);

	CharacterSheet tough = surviving(200, 0, 100, 100);
	tough.survivalTick(10.0 * hour);
	CHECK(tough.health() == 100 - (int)(waterDrainPerSecond(200) * 10.0 * hour));
	CHECK(tough.health() == 98);
}

TEST_CASE("health lost in slivers adds up across many short ticks")
{
	CharacterSheet thirsty = surviving(20, 0, 100, 100);
	for (int i = 0; i < 36000; ++i)
		thirsty.survivalTick(1.0);
	CHECK(thirsty.health() == 87);
}

TEST_CASE("health stops at zero and the character is then dead")
{
	CharacterSheet cal = surviving(20, 0, 0, 3);
	const double perPoint = 1.0 / (waterDrainPerSecond(20) + hungerDrainPerSecond(20));
	CHECK_FALSE(cal.isDead());
	cal.survivalTick(2.5 * perPoint);
	CHECK(cal.health() == 1);
	CHECK_FALSE(cal.isDead());
	cal.survivalTick(perPoint);
	CHECK(cal.health() == 0);
	CHECK(cal.isDead());
	cal.survivalTick(perPoint * 10.0);
	CHECK(cal.health() == 0);
}
