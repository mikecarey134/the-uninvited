#pragma once
#include "CharacterSheet.h"
#include <string>
#include <vector>

namespace menu
{
	const char* characterModel(character::Gender gender);
	const std::vector<std::string>& outfitSkins(character::Gender gender);
	const std::string& outfitSkin(character::Gender gender, int outfit);
}
