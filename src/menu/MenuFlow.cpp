#include "MenuFlow.h"

namespace menu
{
	namespace
	{
		std::string trim(const std::string& text)
		{
			const char* space = " \t\r\n";
			std::string::size_type first = text.find_first_not_of(space);
			if (first == std::string::npos)
				return "";
			return text.substr(first, text.find_last_not_of(space) - first + 1);
		}
	}

	MenuFlow::MenuFlow(const CharacterStore& store, const std::vector<character::Profession>& professions, unsigned seed,
		int bloodlineOneIn)
		: store_(store), professions_(professions), random_(seed), bloodlineOneIn_(bloodlineOneIn)
	{
	}

	void MenuFlow::reload()
	{
		std::vector<std::string> problems;
		characters_ = store_.loadAll(problems);
		highlighted_ = -1;
		chosen_ = -1;
		if (!problems.empty())
			message_ = "Skipped " + std::to_string(problems.size()) + " unreadable character file(s)";
	}

	void MenuFlow::play()
	{
		if (screen() != Screen::Title)
			return;
		screens_.push(Screen::CharacterSelect);
		message_.clear();
		reload();
	}

	void MenuFlow::settings()
	{
		if (screen() != Screen::Title)
			return;
		screens_.push(Screen::Settings);
		message_.clear();
	}

	void MenuFlow::quit()
	{
		if (screen() != Screen::Title)
			return;
		screens_.reset(Screen::Quit);
		message_.clear();
	}

	void MenuFlow::back()
	{
		if (screens_.back())
			message_.clear();
	}

	void MenuFlow::highlight(int index)
	{
		if (screen() == Screen::CharacterSelect)
			highlighted_ = index >= 0 && index < (int)characters_.size() ? index : -1;
	}

	bool MenuFlow::select()
	{
		if (screen() != Screen::CharacterSelect)
			return false;
		if (highlighted_ < 0)
		{
			message_ = characters_.empty() ? "Create a character first" : "Pick a character from the list";
			return false;
		}
		chosen_ = highlighted_;
		screens_.push(Screen::WorldSelect);
		message_.clear();
		return true;
	}

	void MenuFlow::newCharacter()
	{
		if (screen() != Screen::CharacterSelect)
			return;
		screens_.push(Screen::CharacterCreation);
		message_.clear();
		draft_ = character::Aegis::roll(random_, bloodlineOneIn_);
	}

	void MenuFlow::reroll()
	{
		if (screen() == Screen::CharacterCreation)
			draft_ = character::Aegis::roll(random_, bloodlineOneIn_);
	}

	bool MenuFlow::adjust(int attribute, int change)
	{
		if (screen() != Screen::CharacterCreation)
			return false;
		return change > 0 ? draft_.place(attribute, change) : draft_.remove(attribute, -change);
	}

	bool MenuFlow::create(const std::string& name, character::Gender gender, int profession, int outfit)
	{
		if (screen() != Screen::CharacterCreation)
			return false;
		std::string problem;
		if (!validName(name, problem))
		{
			message_ = problem;
			return false;
		}
		if (store_.isTaken(name))
		{
			message_ = "There is already a character called " + name;
			return false;
		}
		if (profession < 0 || profession >= (int)professions_.size())
		{
			message_ = professions_.empty() ? "No professions are loaded" : "Pick a profession";
			return false;
		}
		character::CharacterSheet sheet = character::CharacterSheet::create(name, gender, professions_[profession], draft_);
		sheet.setOutfit(outfit);
		if (!store_.save(sheet, problem))
		{
			message_ = problem;
			return false;
		}
		screens_.back();
		message_.clear();
		reload();
		for (std::size_t i = 0; i < characters_.size(); ++i)
			if (characters_[i].name() == name)
				highlighted_ = (int)i;
		return true;
	}

	void MenuFlow::host()
	{
		if (screen() != Screen::WorldSelect)
			return;
		worldName_ = DEFAULT_WORLD_NAME;
		timeMode_ = TimeMode::RealTime;
		maxPlayers_ = DEFAULT_MAX_PLAYERS;
		screens_.push(Screen::WorldSettings);
		message_.clear();
	}

	bool MenuFlow::startServer(const std::string& worldName, TimeMode mode, const std::string& maxPlayers)
	{
		if (screen() != Screen::WorldSettings)
			return false;
		std::string name = trim(worldName);
		std::string count = trim(maxPlayers);
		if (name.empty())
		{
			message_ = "Enter a world name";
			return false;
		}
		if (count.empty() || count.size() > 2 || count.find_first_not_of("0123456789") != std::string::npos || std::stoi(count) < 1 ||
			std::stoi(count) > MOST_PLAYERS)
		{
			message_ = "Max players must be a number from 1 to " + std::to_string(MOST_PLAYERS);
			return false;
		}
		session_ = Session::Host;
		address_ = "127.0.0.1";
		port_ = DEFAULT_PORT;
		worldName_ = name;
		timeMode_ = mode;
		maxPlayers_ = std::stoi(count);
		screens_.reset(Screen::Playing);
		message_.clear();
		return true;
	}

	float timeScale(TimeMode mode)
	{
		switch (mode)
		{
		case TimeMode::HalfSpeed: return 2.0f;
		case TimeMode::Fast: return 72.0f;
		default: return 1.0f;
		}
	}

	void MenuFlow::join()
	{
		if (screen() != Screen::WorldSelect)
			return;
		screens_.push(Screen::JoinGame);
		message_.clear();
	}

	bool MenuFlow::connect(const std::string& address, const std::string& port)
	{
		if (screen() != Screen::JoinGame)
			return false;
		std::string host = trim(address);
		std::string digits = trim(port);
		if (host.empty())
		{
			message_ = "Enter the server address";
			return false;
		}
		if (digits.empty() || digits.size() > 5 || digits.find_first_not_of("0123456789") != std::string::npos ||
			std::stoi(digits) < 1 || std::stoi(digits) > 65535)
		{
			message_ = "The port must be a number from 1 to 65535";
			return false;
		}
		session_ = Session::Join;
		address_ = host;
		port_ = (uint16_t)std::stoi(digits);
		screens_.reset(Screen::Playing);
		message_.clear();
		return true;
	}

	void MenuFlow::returnToTitle()
	{
		session_ = Session::None;
		screens_.reset(Screen::Title);
		message_.clear();
	}

	const character::CharacterSheet* MenuFlow::chosen() const
	{
		return chosen_ >= 0 && chosen_ < (int)characters_.size() ? &characters_[chosen_] : nullptr;
	}
}
