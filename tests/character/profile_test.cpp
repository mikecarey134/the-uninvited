#include "doctest.h"
#include "CharacterProfile.h"

using namespace character;

namespace
{
	CharacterSheet blooded()
	{
		Aegis aegis;
		const int rolled[ATTRIBUTE_COUNT] = { 17, 300, 21, 9, 25 };
		for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
			aegis.rolled[i] = rolled[i];
		aegis.placed[AGILITY] = 4;
		aegis.placed[STRENGTH] = 2;
		aegis.hasBloodline = true;
		aegis.bloodlineAttribute = GUILE;
		aegis.bloodline = "[Placeholder]";
		const Profession paramedic{ "Paramedic", { { MEDICINE, 15 }, { ELECTRONICS, 10 }, { SURVIVALISM, 5 } }, { "Bandage" } };
		CharacterSheet sheet = CharacterSheet::create("Dana", Gender::Female, paramedic, aegis);
		sheet.setCooking(7);
		sheet.setConstruction(1000);
		sheet.setAge(41.5f);
		return sheet;
	}
}

TEST_CASE("a character's stats survive packing into a profile and unpacking on the other side")
{
	CharacterSheet sent = blooded();
	std::vector<uint8_t> profile = packProfile(sent);
	CHECK(profile.size() <= 255);
	CharacterSheet received = CharacterSheet::create("", Gender::Male, Profession());
	REQUIRE(unpackProfile(profile, "Dana", Gender::Female, 0, received));
	CHECK(describe(received) == describe(sent));
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
	{
		CHECK(received.rolledPoints(i) == sent.rolledPoints(i));
		CHECK(received.placedPoints(i) == sent.placedPoints(i));
		CHECK(received.attribute(i) == sent.attribute(i));
	}
	for (int i = 0; i < SKILL_COUNT; ++i)
		CHECK(received.skill(i) == sent.skill(i));
	CHECK(received.getConstruction() == 1000);
	CHECK(received.profession().name == "Paramedic");
	CHECK(received.profession().skillBonuses == sent.profession().skillBonuses);
	CHECK(received.hasBloodline());
	CHECK(received.bloodlineAttribute() == GUILE);
	CHECK(received.bloodlineName() == "[Placeholder]");
	CHECK(received.getGuile() == 210);
	CHECK(received.age() == 41.5f);
	CHECK(received.name() == "Dana");
	CHECK(received.gender() == Gender::Female);
}

TEST_CASE("a character without a bloodline unpacks without one, wearing the outfit it joined in")
{
	CharacterSheet plain = CharacterSheet::create("Eli", Gender::Male, Profession{ "Chef", { { COOKING, 30 } }, {} });
	CharacterSheet received = blooded();
	REQUIRE(unpackProfile(packProfile(plain), "Eli", Gender::Male, 2, received));
	CHECK_FALSE(received.hasBloodline());
	CHECK(received.getCooking() == 30);
	CHECK(received.outfit() == 2);
	CHECK(describe(received) == describe(plain));
}

TEST_CASE("an empty, cut-short or overlong profile is refused and leaves the sheet alone")
{
	std::vector<uint8_t> profile = packProfile(blooded());
	CharacterSheet untouched = CharacterSheet::create("Eli", Gender::Male, Profession{ "Chef", { { COOKING, 30 } }, {} });
	std::vector<std::string> before = describe(untouched);
	CHECK_FALSE(unpackProfile(std::vector<uint8_t>(), "x", Gender::Male, 0, untouched));
	CHECK_FALSE(unpackProfile(std::vector<uint8_t>(profile.begin(), profile.end() - 1), "x", Gender::Male, 0, untouched));
	std::vector<uint8_t> longer = profile;
	longer.push_back(0);
	CHECK_FALSE(unpackProfile(longer, "x", Gender::Male, 0, untouched));
	CHECK(describe(untouched) == before);
}
