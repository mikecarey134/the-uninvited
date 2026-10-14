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
	CHECK(professions[0].skillBonuses == std::vector<std::pair<Skill, int> >{ { Skill::Medicine, 30 } });
	CHECK(professions[0].startingItems.empty());
	CHECK(professions[1].name == "Nurse");
	CHECK(professions[1].skillBonuses == std::vector<std::pair<Skill, int> >{ { Skill::Medicine, 20 }, { Skill::Chemistry, 10 } });
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
		Skill skill = Skill::Smithing;
		REQUIRE(skillFromName(skillName((Skill)i), skill));
		CHECK((int)skill == i);
	}
	Skill unused = Skill::Smithing;
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
		std::vector<std::pair<Skill, int> > bonuses;
	};
	const Expected expected[] = {
		{ "Doctor", { { Skill::Medicine, 30 } } },
		{ "Chef", { { Skill::Cooking, 30 } } },
		{ "Locksmith", { { Skill::Lockpicking, 30 } } },
		{ "Hacker", { { Skill::Electronics, 30 } } },
		{ "Pharmacist", { { Skill::Chemistry, 30 } } },
		{ "Construction Worker", { { Skill::Construction, 30 } } },
		{ "Park Ranger", { { Skill::Survivalism, 30 } } },
		{ "Soldier", { { Skill::Smithing, 15 }, { Skill::Survivalism, 15 } } },
		{ "Hunter", { { Skill::Survivalism, 20 }, { Skill::Smithing, 10 } } },
		{ "Mechanic", { { Skill::Engineering, 20 }, { Skill::Smithing, 10 } } },
		{ "Engineer", { { Skill::Engineering, 15 }, { Skill::Electronics, 15 } } },
		{ "Sailor", { { Skill::Survivalism, 15 }, { Skill::Engineering, 15 } } },
		{ "Politician", { { Skill::Speech, 20 }, { Skill::Survivalism, 10 } } },
		{ "Police Officer", { { Skill::Lockpicking, 15 }, { Skill::Speech, 15 } } },
		{ "Firefighter", { { Skill::Survivalism, 15 }, { Skill::Construction, 15 } } },
		{ "Electrician", { { Skill::Electronics, 20 }, { Skill::Construction, 10 } } },
		{ "Nurse", { { Skill::Medicine, 20 }, { Skill::Chemistry, 10 } } },
		{ "Farmer", { { Skill::Cooking, 15 }, { Skill::Survivalism, 15 } } },
		{ "Veterinarian", { { Skill::Medicine, 15 }, { Skill::Chemistry, 15 } } },
		{ "Welder", { { Skill::Smithing, 20 }, { Skill::Engineering, 10 } } },
		{ "Prepper", { { Skill::Survivalism, 20 }, { Skill::Smithing, 10 } } },
		{ "Journalist", { { Skill::Speech, 15 }, { Skill::Electronics, 15 } } },
		{ "Salesman", { { Skill::Speech, 15 }, { Skill::Survivalism, 10 }, { Skill::Cooking, 5 } } },
		{ "Bartender", { { Skill::Speech, 10 }, { Skill::Cooking, 10 }, { Skill::Chemistry, 10 } } },
		{ "Entrepreneur", { { Skill::Speech, 10 }, { Skill::Electronics, 10 }, { Skill::Construction, 10 } } },
		{ "Accountant", { { Skill::Electronics, 10 }, { Skill::Speech, 10 }, { Skill::Lockpicking, 10 } } },
		{ "Paramedic", { { Skill::Medicine, 15 }, { Skill::Electronics, 10 }, { Skill::Survivalism, 5 } } },
		{ "Blacksmith", { { Skill::Smithing, 30 } } },
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
