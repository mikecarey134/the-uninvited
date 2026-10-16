#pragma once
#include "CharacterSheet.h"
#include <cstdint>
#include <vector>

namespace character
{
	std::vector<uint8_t> packProfile(const CharacterSheet& sheet);
	bool unpackProfile(const std::vector<uint8_t>& profile, const std::string& name, Gender gender, int outfit, CharacterSheet& sheet);
}
