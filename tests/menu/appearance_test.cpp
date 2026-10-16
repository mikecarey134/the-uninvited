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
