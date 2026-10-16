#include "doctest.h"
#include "CharacterStore.h"
#include <filesystem>
#include <fstream>

using namespace menu;
using namespace character;
namespace fs = std::filesystem;

static std::string freshDirectory(const char* name)
{
	fs::path directory = fs::path(MENU_TEST_DIR) / name;
	fs::remove_all(directory);
	return (directory / "saves" / "characters").string();
}

static const Profession chef{ "Chef", { { COOKING, 30 } }, {} };

static CharacterSheet sheetNamed(const std::string& name, unsigned seed = 1)
{
	std::mt19937 random(seed);
	return CharacterSheet::create(name, Gender::Female, chef, random);
}

static std::string rejection(const std::string& name)
{
	std::string problem;
	CHECK_FALSE(validName(name, problem));
	return problem;
}

TEST_CASE("ordinary names up to the length limit are accepted")
{
	std::string problem;
	CHECK(validName("Old Tom", problem));
	CHECK(validName(std::string(MAX_NAME_LENGTH, 'a'), problem));
	CHECK(problem.empty());
}

TEST_CASE("empty and over-long names are rejected")
{
	CHECK(rejection("") == "Enter a name");
	CHECK(rejection(std::string(MAX_NAME_LENGTH + 1, 'a')) == "Names can be at most 24 characters");
}

TEST_CASE("each kind of unsafe name gets its own message")
{
	CHECK(rejection("a/b") == "Use only letters, numbers, spaces, - and _");
	CHECK(rejection("Dana ") == "Names cannot start or end with a space");
	CHECK(rejection("con") == "That name is reserved, pick another");
}

TEST_CASE("saving writes saves/characters/<name>.character")
{
	std::string directory = freshDirectory("save_path");
	CharacterStore store(directory);
	std::string problem;
	REQUIRE(store.save(sheetNamed("Dana"), problem));

	fs::path expected = fs::path(directory) / "Dana.character";
	CHECK(store.pathFor("Dana") == expected.string());
	CHECK(fs::is_regular_file(expected));
	CHECK(fs::path(directory).filename() == "characters");
	CHECK(fs::path(directory).parent_path().filename() == "saves");
	int files = 0;
	for (const fs::directory_entry& entry : fs::directory_iterator(directory))
	{
		(void)entry;
		++files;
	}
	CHECK(files == 1);
}

TEST_CASE("saved characters load back sorted by name and identical to what was saved")
{
	CharacterStore store(freshDirectory("load_all"));
	std::string problem;
	CharacterSheet zed = sheetNamed("Zed", 3);
	CharacterSheet amy = sheetNamed("amy", 4);
	REQUIRE(store.save(zed, problem));
	REQUIRE(store.save(amy, problem));

	std::vector<std::string> problems;
	std::vector<CharacterSheet> loaded = store.loadAll(problems);
	CHECK(problems.empty());
	REQUIRE(loaded.size() == 2);
	CHECK(loaded[0].toText() == amy.toText());
	CHECK(loaded[1].toText() == zed.toText());
}

TEST_CASE("a missing directory lists no characters")
{
	CharacterStore store(freshDirectory("missing"));
	std::vector<std::string> problems;
	CHECK(store.loadAll(problems).empty());
	CHECK(problems.empty());
	CHECK_FALSE(store.isTaken("Dana"));
}

TEST_CASE("broken and unrelated files are skipped, broken ones reported")
{
	std::string directory = freshDirectory("broken");
	CharacterStore store(directory);
	std::string problem;
	REQUIRE(store.save(sheetNamed("Dana"), problem));
	std::ofstream(fs::path(directory) / "Broken.character") << "name = Broken\n";
	std::ofstream(fs::path(directory) / "notes.txt") << "not a character\n";

	std::vector<std::string> problems;
	std::vector<CharacterSheet> loaded = store.loadAll(problems);
	REQUIRE(loaded.size() == 1);
	CHECK(loaded[0].name() == "Dana");
	REQUIRE(problems.size() == 1);
	CHECK(problems[0].find("Broken.character: missing gender") != std::string::npos);
}

TEST_CASE("a name is taken whatever its case")
{
	CharacterStore store(freshDirectory("taken"));
	std::string problem;
	REQUIRE(store.save(sheetNamed("Dana"), problem));
	CHECK(store.isTaken("Dana"));
	CHECK(store.isTaken("dANA"));
	CHECK_FALSE(store.isTaken("Dan"));
}

TEST_CASE("the store refuses to save an invalid name")
{
	std::string directory = freshDirectory("invalid");
	CharacterStore store(directory);
	std::string problem;
	CHECK_FALSE(store.save(sheetNamed("../escape"), problem));
	CHECK(problem == "Use only letters, numbers, spaces, - and _");
	CHECK_FALSE(fs::exists(directory));
}

TEST_CASE("remove deletes a character's file and reports a character that is not there")
{
	CharacterStore store(freshDirectory("store_remove"));
	std::string problem;
	REQUIRE(store.save(sheetNamed("Old Tom"), problem));
	REQUIRE(store.save(sheetNamed("Ivy"), problem));
	REQUIRE(fs::exists(store.pathFor("Old Tom")));
	CHECK(store.remove("Old Tom", problem));
	CHECK_FALSE(fs::exists(store.pathFor("Old Tom")));
	CHECK(fs::exists(store.pathFor("Ivy")));
	CHECK_FALSE(store.remove("Old Tom", problem));
	CHECK(problem == "There is no character called Old Tom");
	fs::path outside = fs::path(store.directory()).parent_path() / "Ivy.character";
	fs::copy_file(store.pathFor("Ivy"), outside);
	CHECK_FALSE(store.remove("../Ivy", problem));
	CHECK(fs::exists(outside));
	CHECK(fs::exists(store.pathFor("Ivy")));
}

TEST_CASE("a file whose name does not match the character inside is skipped, so saving and deleting never touch the wrong file")
{
	CharacterStore store(freshDirectory("store_mismatch"));
	std::string problem;
	REQUIRE(store.save(sheetNamed("Ivy"), problem));
	fs::copy_file(store.pathFor("Ivy"), store.pathFor("Copy of Ivy"));
	fs::copy_file(store.pathFor("Ivy"), fs::path(store.directory()) / "ivy2.character");
	std::vector<std::string> problems;
	std::vector<CharacterSheet> sheets = store.loadAll(problems);
	REQUIRE(sheets.size() == 1);
	CHECK(sheets[0].name() == "Ivy");
	REQUIRE(problems.size() == 2);
	CHECK(problems[0].find("Copy of Ivy.character") != std::string::npos);
	CHECK(problems[0].find("holds Ivy") != std::string::npos);
	CHECK(problems[1].find("ivy2.character") != std::string::npos);

	fs::rename(store.pathFor("Ivy"), fs::path(store.directory()) / "IVY.character");
	problems.clear();
	CHECK(store.loadAll(problems).size() == 1);
	CHECK(problems.size() == 2);
}
