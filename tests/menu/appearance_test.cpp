#include "doctest.h"
#include "Outfits.h"

using namespace character;

TEST_CASE("gender and outfit pack into the join message's model byte and come back out unchanged")
{
	CHECK(menu::packAppearance(Gender::Male, 0) == 0);
	for (Gender gender : { Gender::Male, Gender::Female })
		for (int outfit = 0; outfit < 128; ++outfit)
		{
			uint8_t packed = menu::packAppearance(gender, outfit);
			Gender back = gender == Gender::Male ? Gender::Female : Gender::Male;
			int dressed = -1;
			menu::unpackAppearance(packed, back, dressed);
			INFO(outfit);
			CHECK(back == gender);
			CHECK(dressed == outfit);
		}
	CHECK(menu::packAppearance(Gender::Female, 0) != menu::packAppearance(Gender::Male, 0));
	CHECK(menu::packAppearance(Gender::Male, 2) != menu::packAppearance(Gender::Male, 1));
}

TEST_CASE("an unpacked appearance names the model and skin a remote player should wear")
{
	Gender gender = Gender::Male;
	int outfit = 0;
	menu::unpackAppearance(menu::packAppearance(Gender::Female, 0), gender, outfit);
	CHECK(std::string(menu::characterModel(gender)) == "assets/models/stick_dan.ms3d");
	CHECK(menu::outfitSkin(gender, outfit) == "assets/models/playerskin_d1.bmp");
	menu::unpackAppearance(menu::packAppearance(Gender::Male, 2), gender, outfit);
	CHECK(std::string(menu::characterModel(gender)) == "assets/models/stick_mike.ms3d");
	CHECK(menu::outfitSkin(gender, outfit) == "assets/models/playerskin_m3.jpg");
}

TEST_CASE("a knife swaps the male model for Mike's knife mesh, and every other weapon keeps the base model with a note")
{
	std::string note;
	CHECK(std::string(menu::characterModel(Gender::Male, "", note)) == "assets/models/stick_mike.ms3d");
	CHECK(note.empty());
	CHECK(std::string(menu::characterModel(Gender::Male, "knife", note)) == "assets/models/stick_mike_knife.ms3d");
	CHECK(note.empty());
	CHECK(std::string(menu::characterModel(Gender::Female, "", note)) == "assets/models/stick_dan.ms3d");
	CHECK(note.empty());
	CHECK(std::string(menu::characterModel(Gender::Female, "knife", note)) == "assets/models/stick_dan.ms3d");
	CHECK(note == "No weapon mesh for knife, using base model.");
	CHECK(std::string(menu::characterModel(Gender::Male, "knife", note)) == "assets/models/stick_mike_knife.ms3d");
	CHECK(note.empty());
	for (const char* weapon : { "hatchet", "pistol", "shotgun" })
	{
		INFO(weapon);
		note.clear();
		CHECK(std::string(menu::characterModel(Gender::Male, weapon, note)) == "assets/models/stick_mike.ms3d");
		CHECK(note == std::string("No weapon mesh for ") + weapon + ", using base model.");
	}
}
