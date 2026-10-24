#include "doctest.h"
#include "ApproachConfig.h"
#include "RecordingCanvas.h"
#include "TargetHud.h"
#include <fstream>
#include <string>

using namespace irr;
using namespace core;

namespace
{
	struct Thing : public Interactable
	{
		std::string name;
		std::string type;

		Thing(const std::string& name, const std::string& type) : name(name), type(type) {}
		InteractableInfo getInfo() const override
		{
			InteractableInfo info;
			info.name = name;
			info.iconPath = "interactables/locked-door.png";
			return info;
		}
		vector3df getPosition() const override { return vector3df(); }
		std::string getObjectType() const override { return type; }
	};

	bool says(DpadHud& hud, const std::wstring& text)
	{
		RecordingCanvas canvas;
		hud.render(canvas);
		for (const RecordingCanvas::Call& call : canvas.calls)
			if (call.kind == RecordingCanvas::TEXT && call.text == text)
				return true;
		return false;
	}

	std::string row(const ApproachSet& set, Approach type)
	{
		const ApproachOption* option = set.getByType(type);
		return option ? option->label + " | " + option->iconPath + " | " + option->skillName : "-";
	}
}

TEST_CASE("the registry carries the eight starting object types with their labels, icons and skills")
{
	ApproachRegistry registry = defaultApproaches();
	struct Expected
	{
		const char* type;
		const char* normal;
		const char* force;
		const char* skill;
		const char* guile;
	};
	const Expected table[] = {
		{ "locked_door", "Try Handle | approach/normal.png | ", "Kick Down | approach/force.png | ", "Pick Lock | skills/lockpicking.png | lockpicking",
			"Send Companion | approach/companion-delegate.png | " },
		{ "unlocked_door", "Open Door | approach/normal.png | ", "-", "-", "-" },
		{ "supply_crate", "Open Crate | approach/normal.png | ", "Smash Open | approach/force.png | ", "-", "-" },
		{ "terminal", "Use Terminal | approach/normal.png | ", "-", "Hack Terminal | skills/electronics.png | electronics", "-" },
		{ "workbench", "Use Workbench | approach/normal.png | ", "-", "Craft Item | skills/engineering.png | engineering", "-" },
		{ "npc_friendly", "Talk | approach/normal.png | ", "-", "-", "Trade | approach/trade.png | " },
		{ "barricaded_door", "Examine | approach/normal.png | ", "Break Through | approach/force.png | ", "Dismantle | skills/construction.png | construction", "-" },
		{ "generator", "Turn On | approach/normal.png | ", "-", "Repair | skills/engineering.png | engineering", "-" } };
	for (const Expected& expected : table)
	{
		CAPTURE(expected.type);
		const ApproachSet* set = registry.getApproaches(expected.type);
		REQUIRE(set != nullptr);
		CHECK(set->objectType == expected.type);
		CHECK(row(*set, Approach::NORMAL) == expected.normal);
		CHECK(row(*set, Approach::FORCE) == expected.force);
		CHECK(row(*set, Approach::SKILL) == expected.skill);
		CHECK(row(*set, Approach::GUILE) == expected.guile);
		for (const ApproachOption& option : set->options)
		{
			CAPTURE(option.iconPath);
			CHECK(std::ifstream(std::string(ASSETS_DIR) + "/ui/icons/" + option.iconPath).good());
		}
	}
	CHECK(registry.getApproaches("toaster") == nullptr);
}

TEST_CASE("hasApproach answers from the options")
{
	ApproachRegistry registry = defaultApproaches();
	const ApproachSet* crate = registry.getApproaches("supply_crate");
	REQUIRE(crate != nullptr);
	CHECK(crate->hasApproach(Approach::NORMAL));
	CHECK(crate->hasApproach(Approach::FORCE));
	CHECK_FALSE(crate->hasApproach(Approach::SKILL));
	CHECK_FALSE(crate->hasApproach(Approach::GUILE));
}

TEST_CASE("each approach has its own D-pad slot: Normal up, Force right, Skill down, Guile left")
{
	CHECK(TargetHud::slotFor(Approach::NORMAL) == DpadDirection::UP);
	CHECK(TargetHud::slotFor(Approach::FORCE) == DpadDirection::RIGHT);
	CHECK(TargetHud::slotFor(Approach::SKILL) == DpadDirection::DOWN);
	CHECK(TargetHud::slotFor(Approach::GUILE) == DpadDirection::LEFT);
}

TEST_CASE("targeting fills the D-pad from the registry, a new target clears the slots it lacks, and losing it goes idle")
{
	TargetHud glue(defaultApproaches());
	DpadHud hud;
	Thing door("Test Door", "locked_door");
	Thing crate("Test Crate", "supply_crate");
	glue.update(&door, 1, hud);
	CHECK(hud.state() == DpadState::ACTIVE);
	CHECK(says(hud, L"Test Door"));
	CHECK(says(hud, L"Try Handle"));
	CHECK(says(hud, L"Kick Down"));
	CHECK(says(hud, L"Pick Lock"));
	CHECK(says(hud, L"Send Companion"));

	glue.update(&crate, 2, hud);
	CHECK(hud.state() == DpadState::MULTI_OBJECT);
	CHECK(says(hud, L"Test Crate"));
	CHECK(says(hud, L"Open Crate"));
	CHECK(says(hud, L"Smash Open"));
	CHECK_FALSE(says(hud, L"Pick Lock"));
	CHECK_FALSE(says(hud, L"Send Companion"));

	glue.update(&crate, 1, hud);
	CHECK(hud.state() == DpadState::ACTIVE);

	glue.update(nullptr, 0, hud);
	CHECK(hud.state() == DpadState::IDLE);
}

TEST_CASE("a target whose type is not registered shows its name with every slot empty")
{
	TargetHud glue(defaultApproaches());
	DpadHud hud;
	Thing door("Test Door", "locked_door");
	Thing odd("Odd Thing", "toaster");
	glue.update(&door, 1, hud);
	glue.update(&odd, 1, hud);
	CHECK(hud.state() == DpadState::ACTIVE);
	CHECK(says(hud, L"Odd Thing"));
	CHECK_FALSE(says(hud, L"Try Handle"));
	CHECK_FALSE(says(hud, L"Kick Down"));
}
