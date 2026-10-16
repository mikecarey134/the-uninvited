#pragma once
#include "CharacterStore.h"
#include "ScreenStack.h"
#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace menu
{
	enum class Screen { Title, Settings, CharacterSelect, CharacterCreation, SkillAllocation, WorldSelect, WorldSettings, JoinGame, Playing, Quit };
	enum class Session { None, Host, Join };
	enum class TimeMode { RealTime, HalfSpeed, Fast };

	float timeScale(TimeMode mode);

	const uint16_t DEFAULT_PORT = 25515;
	const char* const DEFAULT_WORLD_NAME = "New World";
	const int DEFAULT_MAX_PLAYERS = 8;
	const int MOST_PLAYERS = 32;

	struct PendingCharacter
	{
		std::string name;
		character::Gender gender;
		int profession;
		int outfit;
	};

	class MenuFlow
	{
	public:
		MenuFlow(const CharacterStore& store, const std::vector<character::Profession>& professions,
			unsigned seed = std::random_device()(), int bloodlineOneIn = character::Aegis::BLOODLINE_ONE_IN);

		Screen screen() const { return screens_.top(); }
		const std::string& message() const { return message_; }

		void play();
		void settings();
		void quit();
		void back();

		const std::vector<character::CharacterSheet>& characters() const { return characters_; }
		void highlight(int index);
		int highlighted() const { return highlighted_; }
		bool select();
		void newCharacter();
		bool deleteHighlighted();
		bool keep(const character::CharacterSheet& played);
		void setWarning(std::function<void(const std::string&)> warn) { warn_ = warn; }

		const std::vector<character::Profession>& professions() const { return professions_; }
		const character::Aegis& draft() const { return draft_; }
		void reroll();
		bool adjust(int attribute, int change);
		bool create(const std::string& name, character::Gender gender, int profession, int outfit = 0);
		bool toSkills(const std::string& name, character::Gender gender, int profession, int outfit = 0);
		const PendingCharacter* pending() const { return hasPending_ ? &pending_ : nullptr; }
		int skillPool() const;
		int skillsUnplaced() const;
		int skillValue(int skill) const;
		int skillPlaced(int skill) const { return skillPlaced_[skill]; }
		bool adjustSkill(int skill, int change);
		bool confirmSkills();

		void host();
		void join();
		bool startServer(const std::string& worldName = DEFAULT_WORLD_NAME, TimeMode mode = TimeMode::RealTime,
			const std::string& maxPlayers = std::to_string(DEFAULT_MAX_PLAYERS));
		bool connect(const std::string& address, const std::string& port);
		void returnToTitle();

		const character::CharacterSheet* chosen() const;
		Session session() const { return session_; }
		const std::string& address() const { return address_; }
		uint16_t port() const { return port_; }
		const std::string& worldName() const { return worldName_; }
		TimeMode timeMode() const { return timeMode_; }
		float timeScale() const { return menu::timeScale(timeMode_); }
		int maxPlayers() const { return maxPlayers_; }

	private:
		void reload();
		bool ready(const std::string& name, int profession);
		bool save(const character::CharacterSheet& sheet);

		const CharacterStore& store_;
		std::vector<character::Profession> professions_;
		std::vector<character::CharacterSheet> characters_;
		ScreenStack<Screen> screens_{ Screen::Title };
		std::string message_;
		int highlighted_ = -1;
		int chosen_ = -1;
		Session session_ = Session::None;
		std::string address_;
		uint16_t port_ = DEFAULT_PORT;
		std::string worldName_ = DEFAULT_WORLD_NAME;
		TimeMode timeMode_ = TimeMode::RealTime;
		int maxPlayers_ = DEFAULT_MAX_PLAYERS;
		std::mt19937 random_;
		int bloodlineOneIn_;
		int deleting_ = -1;
		std::function<void(const std::string&)> warn_;
		character::Aegis draft_;
		PendingCharacter pending_;
		bool hasPending_ = false;
		int skillPlaced_[character::SKILL_COUNT] = {};
	};
}
