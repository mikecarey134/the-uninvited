#include "CharacterStore.h"
#include "SafeFileName.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace menu
{
	namespace
	{
		const char* const extension = ".character";

		std::string lower(std::string text)
		{
			for (char& c : text)
				c = (char)std::tolower((unsigned char)c);
			return text;
		}
	}

	bool validName(const std::string& name, std::string& problem)
	{
		if (name.size() > MAX_NAME_LENGTH)
		{
			problem = "Names can be at most " + std::to_string(MAX_NAME_LENGTH) + " characters";
			return false;
		}
		switch (checkFileName(name))
		{
		case FileNameProblem::None: return true;
		case FileNameProblem::Empty: problem = "Enter a name"; break;
		case FileNameProblem::BadCharacter: problem = "Use only letters, numbers, spaces, - and _"; break;
		case FileNameProblem::EdgeSpace: problem = "Names cannot start or end with a space"; break;
		case FileNameProblem::Reserved: problem = "That name is reserved, pick another"; break;
		}
		return false;
	}

	std::string CharacterStore::pathFor(const std::string& name) const
	{
		return (fs::path(directory_) / (name + extension)).string();
	}

	bool CharacterStore::isTaken(const std::string& name) const
	{
		std::error_code error;
		for (fs::directory_iterator entry(directory_, error), end; !error && entry != end; entry.increment(error))
			if (entry->path().extension() == extension && lower(entry->path().stem().string()) == lower(name))
				return true;
		return false;
	}

	bool CharacterStore::save(const character::CharacterSheet& sheet, std::string& problem) const
	{
		if (!validName(sheet.name(), problem))
			return false;
		std::error_code error;
		fs::create_directories(directory_, error);
		std::ofstream out(pathFor(sheet.name()), std::ios::binary);
		out << sheet.toText();
		out.close();
		if (!out)
		{
			problem = "Unable to write " + pathFor(sheet.name());
			return false;
		}
		return true;
	}

	bool CharacterStore::remove(const std::string& name, std::string& problem) const
	{
		std::error_code error;
		if (!validName(name, problem) || !fs::remove(pathFor(name), error))
		{
			problem = "There is no character called " + name;
			return false;
		}
		return true;
	}

	std::vector<character::CharacterSheet> CharacterStore::loadAll(std::vector<std::string>& problems) const
	{
		std::vector<fs::path> files;
		std::error_code error;
		for (fs::directory_iterator entry(directory_, error), end; !error && entry != end; entry.increment(error))
			if (entry->path().extension() == extension)
				files.push_back(entry->path());
		std::sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
			return lower(a.stem().string()) < lower(b.stem().string());
		});

		std::vector<character::CharacterSheet> sheets;
		for (const fs::path& file : files)
		{
			std::ifstream in(file, std::ios::binary);
			std::ostringstream text;
			text << in.rdbuf();
			std::string problem;
			character::CharacterSheet sheet = character::CharacterSheet::fromText(text.str(), problem);
			if (problem.empty() && lower(sheet.name()) != lower(file.stem().string()))
				problem = "holds " + sheet.name() + ", so it should be called " + sheet.name() + extension;
			if (problem.empty())
				sheets.push_back(sheet);
			else
				problems.push_back(file.string() + ": " + problem);
		}
		return sheets;
	}
}
