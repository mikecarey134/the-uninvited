#include "Outfits.h"

namespace menu
{
	const char* characterModel(character::Gender gender)
	{
		return gender == character::Gender::Female ? "assets/models/stick_dan.ms3d" : "assets/models/stick_mike.ms3d";
	}

	const std::vector<std::string>& outfitSkins(character::Gender gender)
	{
		static const std::vector<std::string> male = { "assets/models/playerskin_m1.jpg", "assets/models/playerskin_m2.jpg",
			"assets/models/playerskin_m3.jpg" };
		static const std::vector<std::string> female = { "assets/models/playerskin_d1.bmp" };
		return gender == character::Gender::Female ? female : male;
	}

	const std::string& outfitSkin(character::Gender gender, int outfit)
	{
		const std::vector<std::string>& skins = outfitSkins(gender);
		int count = (int)skins.size();
		return skins[((outfit % count) + count) % count];
	}

	uint8_t packAppearance(character::Gender gender, int outfit)
	{
		return (uint8_t)((gender == character::Gender::Female ? 0x80 : 0) | (outfit & 0x7F));
	}

	void unpackAppearance(uint8_t packed, character::Gender& gender, int& outfit)
	{
		gender = packed & 0x80 ? character::Gender::Female : character::Gender::Male;
		outfit = packed & 0x7F;
	}
}
