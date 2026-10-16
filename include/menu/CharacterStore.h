#pragma once
#include "CharacterSheet.h"
#include <string>
#include <vector>

namespace menu
{
	const std::size_t MAX_NAME_LENGTH = 24;

	bool validName(const std::string& name, std::string& problem);

	class CharacterStore
	{
	public:
		explicit CharacterStore(const std::string& directory) : directory_(directory) {}

		const std::string& directory() const { return directory_; }
		std::string pathFor(const std::string& name) const;
		bool isTaken(const std::string& name) const;
		bool save(const character::CharacterSheet& sheet, std::string& problem) const;
		bool remove(const std::string& name, std::string& problem) const;
		std::vector<character::CharacterSheet> loadAll(std::vector<std::string>& problems) const;

	private:
		std::string directory_;
	};
}
