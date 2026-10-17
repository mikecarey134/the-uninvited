#pragma once
#include "CharacterSheet.h"
#include <cstdint>
#include <string>
#include <vector>

namespace menu
{
	const char* characterModel(character::Gender gender);
	const char* characterModel(character::Gender gender, const std::string& weapon, std::string& note);
	const std::vector<std::string>& outfitSkins(character::Gender gender);
	const std::string& outfitSkin(character::Gender gender, int outfit);
	uint8_t packAppearance(character::Gender gender, int outfit);
	void unpackAppearance(uint8_t packed, character::Gender& gender, int& outfit);
}
