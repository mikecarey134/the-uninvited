#include "doctest.h"
#include "Profession.h"
#include <fstream>
#include <sstream>

using namespace character;

static const char* const header = "name, skills, items\n";

static std::vector<Profession> loaded(const std::string& csv)
{
	std::vector<Profession> professions;
	LoadError error;
	bool ok = loadProfessions(csv, professions, error);
	INFO(error.file, " line ", error.line, ": ", error.message);
	REQUIRE(ok);
	return professions;
}

static LoadError failed(const std::string& csv)
{
	std::vector<Profession> professions;
	LoadError error;
	CHECK_FALSE(loadProfessions(csv, professions, error));
	return error;
}

TEST_CASE("professions load their skill bonuses and starting items")
{
	std::vector<Profession> professions = loaded(std::string(header) +
		"Doctor, Medicine:30, -\n"
		"Nurse,  Medicine:20 | Chemistry:10, bandage | aspirin\n");

	REQUIRE(professions.size() == 2);
	CHECK(professions[0].name == "Doctor");
	CHECK(professions[0].skillBonuses == std::vector<std::pair<int, int> >{ { MEDICINE, 30 } });
	CHECK(professions[0].startingItems.empty());
	CHECK(professions[1].name == "Nurse");
	CHECK(professions[1].skillBonuses == std::vector<std::pair<int, int> >{ { MEDICINE, 20 }, { CHEMISTRY, 10 } });
	CHECK(professions[1].startingItems == std::vector<std::string>{ "bandage", "aspirin" });
}

TEST_CASE("a profession can be found by name")
{
	std::vector<Profession> professions = loaded(std::string(header) + "Doctor, Medicine:30, -\n");
	REQUIRE(findProfession(professions, "Doctor") != nullptr);
	CHECK(findProfession(professions, "Doctor")->name == "Doctor");
	CHECK(findProfession(professions, "Dentist") == nullptr);
}

TEST_CASE("blank lines and a byte order mark are ignored")
{
	std::vector<Profession> professions = loaded(std::string("\xEF\xBB\xBF") + header + "\nChef, Cooking:30, -\n\n");
	REQUIRE(professions.size() == 1);
	CHECK(professions[0].name == "Chef");
}

TEST_CASE("a missing column is reported against the header line")
{
	LoadError error = failed("\nname, items\nChef, -\n");
	CHECK(error.file == "professions.csv");
	CHECK(error.line == 2);
	CHECK(error.message == "missing column 'skills'");
}

TEST_CASE("an unknown skill is reported with its line")
{
	LoadError error = failed(std::string(header) + "Chef, Cooking:30, -\nWizard, Magic:30, -\n");
	CHECK(error.line == 3);
	CHECK(error.message == "unknown skill 'Magic'");
}

TEST_CASE("skill points must be a positive whole number")
{
	CHECK(failed(std::string(header) + "Chef, Cooking:lots, -\n").message == "bad points 'lots' for skill 'Cooking'");
	CHECK(failed(std::string(header) + "Chef, Cooking:0, -\n").message == "bad points '0' for skill 'Cooking'");
	CHECK(failed(std::string(header) + "Chef, Cooking, -\n").message == "skill 'Cooking' needs points, e.g. Cooking:30");
}

TEST_CASE("a profession needs a name, a unique name, and a skill at most once")
{
	CHECK(failed(std::string(header) + ", Cooking:30, -\n").message == "missing name");
	CHECK(failed(std::string(header) + "Chef, Cooking:30, -\nChef, Cooking:20, -\n").message == "duplicate profession 'Chef'");
	CHECK(failed(std::string(header) + "Chef, Cooking:10 | Cooking:20, -\n").message == "skill 'Cooking' listed twice");
}

TEST_CASE("every skill name round trips")
{
	for (int i = 0; i < SKILL_COUNT; ++i)
	{
		int skill = SMITHING;
		REQUIRE(skillFromName(skillName(i), skill));
		CHECK(skill == i);
	}
	int unused = SMITHING;
	CHECK_FALSE(skillFromName("medicine", unused));
}

static std::string readConfigFile(const std::string& name)
{
	std::string path = std::string(CHARACTER_CONFIG_DIR) + "/" + name;
	std::ifstream in(path, std::ios::binary);
	INFO("cannot open ", path);
	REQUIRE(in.is_open());
	std::ostringstream text;
	text << in.rdbuf();
	return text.str();
}

TEST_CASE("the shipped professions file holds all 28 professions with their bonuses")
{
	struct Expected
	{
		const char* name;
		std::vector<std::pair<int, int> > bonuses;
	};
	const Expected expected[] = {
		{ "Doctor", { { MEDICINE, 30 } } },
		{ "Chef", { { COOKING, 30 } } },
		{ "Locksmith", { { LOCKPICKING, 30 } } },
		{ "Hacker", { { ELECTRONICS, 30 } } },
		{ "Pharmacist", { { CHEMISTRY, 30 } } },
		{ "Construction Worker", { { CONSTRUCTION, 30 } } },
		{ "Park Ranger", { { SURVIVALISM, 30 } } },
		{ "Soldier", { { SMITHING, 15 }, { SURVIVALISM, 15 } } },
		{ "Hunter", { { SURVIVALISM, 20 }, { SMITHING, 10 } } },
		{ "Mechanic", { { ENGINEERING, 20 }, { SMITHING, 10 } } },
		{ "Engineer", { { ENGINEERING, 15 }, { ELECTRONICS, 15 } } },
		{ "Sailor", { { SURVIVALISM, 15 }, { ENGINEERING, 15 } } },
		{ "Politician", { { SPEECH, 20 }, { SURVIVALISM, 10 } } },
		{ "Police Officer", { { LOCKPICKING, 15 }, { SPEECH, 15 } } },
		{ "Firefighter", { { SURVIVALISM, 15 }, { CONSTRUCTION, 15 } } },
		{ "Electrician", { { ELECTRONICS, 20 }, { CONSTRUCTION, 10 } } },
		{ "Nurse", { { MEDICINE, 20 }, { CHEMISTRY, 10 } } },
		{ "Farmer", { { COOKING, 15 }, { SURVIVALISM, 15 } } },
		{ "Veterinarian", { { MEDICINE, 15 }, { CHEMISTRY, 15 } } },
		{ "Welder", { { SMITHING, 20 }, { ENGINEERING, 10 } } },
		{ "Prepper", { { SURVIVALISM, 20 }, { SMITHING, 10 } } },
		{ "Journalist", { { SPEECH, 15 }, { ELECTRONICS, 15 } } },
		{ "Salesman", { { SPEECH, 15 }, { SURVIVALISM, 10 }, { COOKING, 5 } } },
		{ "Bartender", { { SPEECH, 10 }, { COOKING, 10 }, { CHEMISTRY, 10 } } },
		{ "Entrepreneur", { { SPEECH, 10 }, { ELECTRONICS, 10 }, { CONSTRUCTION, 10 } } },
		{ "Accountant", { { ELECTRONICS, 10 }, { SPEECH, 10 }, { LOCKPICKING, 10 } } },
		{ "Paramedic", { { MEDICINE, 15 }, { ELECTRONICS, 10 }, { SURVIVALISM, 5 } } },
		{ "Blacksmith", { { SMITHING, 30 } } },
	};

	std::vector<Profession> professions = loaded(readConfigFile("professions.csv"));
	CHECK(professions.size() == 28);
	for (const Expected& want : expected)
	{
		const Profession* profession = findProfession(professions, want.name);
		INFO("profession ", want.name);
		REQUIRE(profession != nullptr);
		CHECK(profession->skillBonuses == want.bonuses);
	}
}
