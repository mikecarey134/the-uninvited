#include "doctest.h"
#include "MenuFlow.h"
#include "server/GameServer.h"
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

using namespace menu;
using namespace character;
namespace fs = std::filesystem;

namespace
{
	std::vector<Profession> shippedProfessions()
	{
		std::ifstream in(std::string(CHARACTER_CONFIG_DIR) + "/professions.csv", std::ios::binary);
		std::ostringstream text;
		text << in.rdbuf();
		std::vector<Profession> professions;
		LoadError error;
		bool loaded = loadProfessions(text.str(), professions, error);
		INFO(error.file, " line ", error.line, ": ", error.message);
		REQUIRE(loaded);
		return professions;
	}

	struct Fixture
	{
		explicit Fixture(const char* name, std::vector<Profession> professions = shippedProfessions(), int bloodlineOneIn = 1000000)
			: directory(freshDirectory(name)), store(directory), flow(store, professions, 9, bloodlineOneIn) {}

		static std::string freshDirectory(const char* name)
		{
			fs::path directory = fs::path(MENU_TEST_DIR) / name;
			fs::remove_all(directory);
			return directory.string();
		}

		int professionNamed(const std::string& name) const
		{
			for (std::size_t i = 0; i < flow.professions().size(); ++i)
				if (flow.professions()[i].name == name)
					return (int)i;
			return -1;
		}

		void createAndSelect(const std::string& name)
		{
			flow.play();
			flow.newCharacter();
			REQUIRE(flow.create(name, Gender::Male, professionNamed("Doctor")));
			REQUIRE(flow.select());
		}

		std::string directory;
		CharacterStore store;
		MenuFlow flow;
	};
}

TEST_CASE("the menu starts on the title screen with no session")
{
	Fixture fixture("start");
	CHECK(fixture.flow.screen() == Screen::Title);
	CHECK(fixture.flow.session() == Session::None);
	CHECK(fixture.flow.chosen() == nullptr);
}

TEST_CASE("title to settings and back, and quit")
{
	Fixture fixture("settings");
	fixture.flow.settings();
	CHECK(fixture.flow.screen() == Screen::Settings);
	fixture.flow.back();
	CHECK(fixture.flow.screen() == Screen::Title);
	fixture.flow.quit();
	CHECK(fixture.flow.screen() == Screen::Quit);
}

TEST_CASE("title to select to create and back to select, then back to title")
{
	Fixture fixture("create_back");
	fixture.flow.play();
	CHECK(fixture.flow.screen() == Screen::CharacterSelect);
	fixture.flow.newCharacter();
	CHECK(fixture.flow.screen() == Screen::CharacterCreation);
	fixture.flow.back();
	CHECK(fixture.flow.screen() == Screen::CharacterSelect);
	fixture.flow.back();
	CHECK(fixture.flow.screen() == Screen::Title);
}

TEST_CASE("creating a character saves it, returns to select and highlights it")
{
	Fixture fixture("create");
	fixture.flow.play();
	CHECK(fixture.flow.characters().empty());
	fixture.flow.newCharacter();
	REQUIRE(fixture.flow.create("Dana", Gender::Female, fixture.professionNamed("Paramedic")));

	CHECK(fixture.flow.screen() == Screen::CharacterSelect);
	REQUIRE(fixture.flow.characters().size() == 1);
	const CharacterSheet& dana = fixture.flow.characters()[0];
	CHECK(dana.name() == "Dana");
	CHECK(dana.gender() == Gender::Female);
	CHECK(dana.profession().name == "Paramedic");
	CHECK(dana.skill(Skill::Medicine) == 15);
	CHECK(dana.unplacedPoints() == dana.pointPool());
	CHECK(fixture.flow.highlighted() == 0);
	CHECK(fs::is_regular_file(fs::path(fixture.directory) / "Dana.character"));
	fixture.flow.back();
	CHECK(fixture.flow.screen() == Screen::Title);
}

TEST_CASE("characters persist: a new menu over the same folder lists them")
{
	Fixture first("persist");
	first.flow.play();
	first.flow.newCharacter();
	REQUIRE(first.flow.create("Bo", Gender::Male, first.professionNamed("Chef")));
	std::string saved = first.flow.characters()[0].toText();

	MenuFlow again(first.store, shippedProfessions());
	again.play();
	REQUIRE(again.characters().size() == 1);
	CHECK(again.characters()[0].toText() == saved);
}

TEST_CASE("starting a new character rolls a fresh draft with its whole pool to place")
{
	Fixture fixture("draft");
	fixture.flow.play();
	fixture.flow.newCharacter();
	const Aegis& draft = fixture.flow.draft();
	int total = 0;
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
	{
		CHECK(draft.rolled[i] >= 1);
		CHECK(draft.placed[i] == 0);
		total += draft.rolled[i];
	}
	CHECK(total == 100);
	CHECK(draft.unplaced() == draft.pool());
}

TEST_CASE("re-rolling gives a new draft and forgets placed points, as often as the player likes")
{
	Fixture fixture("reroll");
	fixture.flow.play();
	fixture.flow.newCharacter();
	REQUIRE(fixture.flow.adjust(Attribute::Strength, 1));
	std::set<std::vector<int> > seen;
	for (int roll = 0; roll < 20; ++roll)
	{
		const Aegis& draft = fixture.flow.draft();
		seen.insert(std::vector<int>(draft.rolled, draft.rolled + ATTRIBUTE_COUNT));
		fixture.flow.reroll();
		CHECK(fixture.flow.draft().placed[(int)Attribute::Strength] == 0);
	}
	CHECK(seen.size() > 15);
	CHECK(fixture.flow.screen() == Screen::CharacterCreation);
}

TEST_CASE("plus and minus move points between the pool and an attribute, within the pool and the rolled value")
{
	Fixture fixture("adjust");
	fixture.flow.play();
	fixture.flow.newCharacter();
	int pool = fixture.flow.draft().pool();
	CHECK_FALSE(fixture.flow.adjust(Attribute::Guile, -1));
	for (int point = 0; point < pool; ++point)
		REQUIRE(fixture.flow.adjust(Attribute::Guile, 1));
	CHECK_FALSE(fixture.flow.adjust(Attribute::Agility, 1));
	CHECK(fixture.flow.draft().unplaced() == 0);
	CHECK(fixture.flow.adjust(Attribute::Guile, -1));
	CHECK(fixture.flow.adjust(Attribute::Agility, 1));
	CHECK(fixture.flow.draft().placed[(int)Attribute::Guile] == pool - 1);
	CHECK(fixture.flow.draft().placed[(int)Attribute::Agility] == 1);
}

TEST_CASE("the draft can only be changed while creating a character")
{
	Fixture fixture("adjust_elsewhere");
	fixture.flow.play();
	CHECK_FALSE(fixture.flow.adjust(Attribute::Guile, 1));
	fixture.flow.newCharacter();
	std::vector<int> rolled(fixture.flow.draft().rolled, fixture.flow.draft().rolled + ATTRIBUTE_COUNT);
	fixture.flow.back();
	fixture.flow.reroll();
	CHECK_FALSE(fixture.flow.adjust(Attribute::Guile, 1));
	CHECK(std::vector<int>(fixture.flow.draft().rolled, fixture.flow.draft().rolled + ATTRIBUTE_COUNT) == rolled);
}

TEST_CASE("creating a character saves exactly the draft's roll, placements and bloodline")
{
	Fixture fixture("create_draft", shippedProfessions(), 1);
	fixture.flow.play();
	fixture.flow.newCharacter();
	REQUIRE(fixture.flow.adjust(Attribute::Strength, 1));
	REQUIRE(fixture.flow.adjust(Attribute::Strength, 1));
	REQUIRE(fixture.flow.adjust(Attribute::Intelligence, 1));
	Aegis draft = fixture.flow.draft();
	REQUIRE(fixture.flow.create("Dana", Gender::Female, fixture.professionNamed("Doctor")));

	MenuFlow reopened(fixture.store, shippedProfessions());
	reopened.play();
	REQUIRE(reopened.characters().size() == 1);
	const CharacterSheet& dana = reopened.characters()[0];
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
	{
		CHECK(dana.rolledPoints((Attribute)i) == draft.rolled[i]);
		CHECK(dana.placedPoints((Attribute)i) == draft.placed[i]);
	}
	CHECK(dana.placedPoints(Attribute::Strength) == 2);
	CHECK(dana.hasBloodline());
	CHECK(dana.bloodlineName() == "[Placeholder]");
	CHECK(dana.bloodlineAttribute() == draft.bloodlineAttribute);
}

TEST_CASE("creation refuses invalid and duplicate names and a missing profession, staying on the screen")
{
	Fixture fixture("create_refused");
	fixture.flow.play();
	fixture.flow.newCharacter();
	int doctor = fixture.professionNamed("Doctor");

	CHECK_FALSE(fixture.flow.create("", Gender::Male, doctor));
	CHECK(fixture.flow.message() == "Enter a name");
	CHECK_FALSE(fixture.flow.create("a/b", Gender::Male, doctor));
	CHECK(fixture.flow.message() == "Use only letters, numbers, spaces, - and _");
	CHECK_FALSE(fixture.flow.create(std::string(MAX_NAME_LENGTH + 1, 'x'), Gender::Male, doctor));
	CHECK_FALSE(fixture.flow.create("Dana", Gender::Male, -1));
	CHECK(fixture.flow.message() == "Pick a profession");
	CHECK(fixture.flow.screen() == Screen::CharacterCreation);
	CHECK_FALSE(fs::exists(fixture.directory));

	REQUIRE(fixture.flow.create("Dana", Gender::Male, doctor));
	fixture.flow.newCharacter();
	CHECK_FALSE(fixture.flow.create("DANA", Gender::Female, doctor));
	CHECK(fixture.flow.message() == "There is already a character called DANA");
	CHECK(fixture.flow.screen() == Screen::CharacterCreation);
}

TEST_CASE("creation without loaded professions says so")
{
	Fixture fixture("no_professions", std::vector<Profession>());
	fixture.flow.play();
	fixture.flow.newCharacter();
	CHECK_FALSE(fixture.flow.create("Dana", Gender::Male, 0));
	CHECK(fixture.flow.message() == "No professions are loaded");
}

TEST_CASE("select needs a highlighted character")
{
	Fixture fixture("select_needs");
	fixture.flow.play();
	CHECK_FALSE(fixture.flow.select());
	CHECK(fixture.flow.message() == "Create a character first");
	fixture.flow.newCharacter();
	REQUIRE(fixture.flow.create("Dana", Gender::Male, 0));
	fixture.flow.highlight(-1);
	CHECK_FALSE(fixture.flow.select());
	CHECK(fixture.flow.message() == "Pick a character from the list");
	fixture.flow.highlight(5);
	CHECK(fixture.flow.highlighted() == -1);
	CHECK(fixture.flow.screen() == Screen::CharacterSelect);
}

TEST_CASE("title to select to world and back to select keeps the character highlighted")
{
	Fixture fixture("world_back");
	fixture.createAndSelect("Eli");
	CHECK(fixture.flow.screen() == Screen::WorldSelect);
	REQUIRE(fixture.flow.chosen() != nullptr);
	CHECK(fixture.flow.chosen()->name() == "Eli");
	fixture.flow.back();
	CHECK(fixture.flow.screen() == Screen::CharacterSelect);
	CHECK(fixture.flow.highlighted() == 0);
}

TEST_CASE("host game plays on the local server at the default port")
{
	Fixture fixture("host");
	fixture.createAndSelect("Fay");
	fixture.flow.host();
	CHECK(fixture.flow.screen() == Screen::Playing);
	CHECK(fixture.flow.session() == Session::Host);
	CHECK(fixture.flow.address() == "127.0.0.1");
	CHECK(fixture.flow.port() == DEFAULT_PORT);
	CHECK(fixture.flow.chosen()->name() == "Fay");
	fixture.flow.back();
	CHECK(fixture.flow.screen() == Screen::Playing);
}

TEST_CASE("the menu's default port is the server's")
{
	CHECK(DEFAULT_PORT == GameServer::DEFAULT_PORT);
}

TEST_CASE("join game asks for an address and port, and back returns to world select")
{
	Fixture fixture("join_back");
	fixture.createAndSelect("Gil");
	fixture.flow.join();
	CHECK(fixture.flow.screen() == Screen::JoinGame);
	fixture.flow.back();
	CHECK(fixture.flow.screen() == Screen::WorldSelect);
}

TEST_CASE("connect validates the address and port before playing")
{
	Fixture fixture("connect");
	fixture.createAndSelect("Hal");
	fixture.flow.join();

	CHECK_FALSE(fixture.flow.connect("  ", "25515"));
	CHECK(fixture.flow.message() == "Enter the server address");
	const char* badPorts[] = { "", "0", "65536", "99999999", "12a", "-5" };
	for (const char* port : badPorts)
	{
		INFO(port);
		CHECK_FALSE(fixture.flow.connect("example.org", port));
		CHECK(fixture.flow.message() == "The port must be a number from 1 to 65535");
	}
	CHECK(fixture.flow.screen() == Screen::JoinGame);

	REQUIRE(fixture.flow.connect(" 10.0.0.7 ", " 4000 "));
	CHECK(fixture.flow.screen() == Screen::Playing);
	CHECK(fixture.flow.session() == Session::Join);
	CHECK(fixture.flow.address() == "10.0.0.7");
	CHECK(fixture.flow.port() == 4000);
	fixture.flow.back();
	CHECK(fixture.flow.screen() == Screen::Playing);
}

TEST_CASE("leaving the game returns to the title with no session")
{
	Fixture fixture("leave");
	fixture.createAndSelect("Ike");
	fixture.flow.host();
	fixture.flow.returnToTitle();
	CHECK(fixture.flow.screen() == Screen::Title);
	CHECK(fixture.flow.session() == Session::None);
	fixture.flow.back();
	CHECK(fixture.flow.screen() == Screen::Title);
	fixture.flow.play();
	CHECK(fixture.flow.screen() == Screen::CharacterSelect);
	CHECK(fixture.flow.characters().size() == 1);
}

TEST_CASE("buttons from other screens do nothing")
{
	Fixture fixture("other_screens");
	fixture.flow.newCharacter();
	fixture.flow.host();
	fixture.flow.join();
	CHECK_FALSE(fixture.flow.select());
	CHECK_FALSE(fixture.flow.connect("a", "1"));
	CHECK_FALSE(fixture.flow.create("Dana", Gender::Male, 0));
	fixture.flow.back();
	CHECK(fixture.flow.screen() == Screen::Title);
	fixture.flow.play();
	fixture.flow.quit();
	fixture.flow.settings();
	CHECK(fixture.flow.screen() == Screen::CharacterSelect);
}

TEST_CASE("the creation screen offers every shipped profession")
{
	Fixture fixture("professions");
	REQUIRE(fixture.flow.professions().size() == 27);
	CHECK(fixture.flow.professions()[0].name == "Doctor");
	CHECK(fixture.flow.professions()[26].name == "Paramedic");
	CHECK(fixture.professionNamed("Construction Worker") >= 0);
}
