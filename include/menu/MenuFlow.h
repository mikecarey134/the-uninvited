#pragma once
#include "CharacterStore.h"
#include "ScreenStack.h"
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace menu
{
	enum class Screen { Title, Settings, CharacterSelect, CharacterCreation, WorldSelect, JoinGame, Playing, Quit };
	enum class Session { None, Host, Join };

	const uint16_t DEFAULT_PORT = 25515;

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

		const std::vector<character::Profession>& professions() const { return professions_; }
		const character::Aegis& draft() const { return draft_; }
		void reroll();
		bool adjust(character::Attribute attribute, int change);
		bool create(const std::string& name, character::Gender gender, int profession);

		void host();
		void join();
		bool connect(const std::string& address, const std::string& port);
		void returnToTitle();

		const character::CharacterSheet* chosen() const;
		Session session() const { return session_; }
		const std::string& address() const { return address_; }
		uint16_t port() const { return port_; }

	private:
		void reload();

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
		std::mt19937 random_;
		int bloodlineOneIn_;
		character::Aegis draft_;
	};
}
