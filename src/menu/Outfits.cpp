#include "Outfits.h"

namespace menu
{
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
}
