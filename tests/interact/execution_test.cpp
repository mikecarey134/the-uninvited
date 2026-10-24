#include "doctest.h"
#include "CharacterSheet.h"
#include "ContainerInteractable.h"
#include "InteractionInput.h"
#include "TestInteractables.h"
#include <string>
#include <vector>

using namespace irr;
using namespace core;

namespace
{
	const character::Profession drifter{ "Drifter", {}, {} };

	character::CharacterSheet player(int strength, int lockpicking = 0, int electronics = 0)
	{
		character::Aegis aegis;
		for (int i = 0; i < character::ATTRIBUTE_COUNT; ++i)
			aegis.rolled[i] = 10;
		aegis.rolled[character::STRENGTH] = strength;
		character::CharacterSheet sheet = character::CharacterSheet::create("Ivy", character::Gender::Female, drifter, aegis);
		sheet.setLockpicking(lockpicking);
		sheet.setElectronics(electronics);
		return sheet;
	}

	SEvent key(EKEY_CODE code)
	{
		SEvent event;
		event.EventType = EET_KEY_INPUT_EVENT;
		event.KeyInput.Key = code;
		event.KeyInput.PressedDown = true;
		event.KeyInput.Char = 0;
		event.KeyInput.Shift = false;
		event.KeyInput.Control = false;
		return event;
	}

	struct Rig
	{
		TargetHud targets{ defaultApproaches() };
		DpadHud hud;
		ContextPanel panel;
		InteractionInput input{ targets, hud, panel };
		character::CharacterSheet sheet;
		std::vector<std::string> log;

		explicit Rig(const character::CharacterSheet& sheet) : sheet(sheet)
		{
			input.setPlayer(&this->sheet);
			input.setLog([this](const std::string& line) { log.push_back(line); });
		}

		void aim(Interactable& target) { targets.update(&target, 1, hud); }
		void press(EKEY_CODE code) { input.onEvent(key(code)); }
	};

	template <typename T> T made(const char* name, const char* type)
	{
		T object;
		object.info.name = name;
		object.type = type;
		return object;
	}
}

TEST_CASE("kicking the test door checks Strength: one short of the threshold fails, the threshold succeeds")
{
	TestDoor door = made<TestDoor>("Test Door", "locked_door");
	Rig weak(player(TestDoor::STRENGTH_TO_KICK - 1));
	weak.aim(door);
	CHECK_FALSE(weak.input.execute(Approach::FORCE));
	CHECK(weak.input.feedback() == "Not strong enough.");
	CHECK_FALSE(weak.input.feedbackSucceeded());
	CHECK(weak.log == std::vector<std::string>{ "FAILED: Not strong enough." });

	Rig strong(player(TestDoor::STRENGTH_TO_KICK));
	strong.aim(door);
	CHECK(strong.input.execute(Approach::FORCE));
	CHECK(strong.input.feedback() == "You kick the door open!");
	CHECK(strong.input.feedbackSucceeded());
	CHECK(strong.log == std::vector<std::string>{ "OK: You kick the door open!" });
}

TEST_CASE("picking the test door's lock checks Lockpicking, its handle is locked, and Guile sends the companion")
{
	TestDoor door = made<TestDoor>("Test Door", "locked_door");
	Rig clumsy(player(10, TestDoor::LOCKPICKING_TO_PICK - 1));
	clumsy.aim(door);
	CHECK_FALSE(clumsy.input.execute(Approach::SKILL));
	CHECK(clumsy.input.feedback() == "Lock is too complex.");
	CHECK_FALSE(clumsy.input.execute(Approach::NORMAL));
	CHECK(clumsy.input.feedback() == "The door is locked.");
	CHECK(clumsy.input.execute(Approach::GUILE));
	CHECK(clumsy.input.feedback() == "Your companion moves to the door.");

	Rig thief(player(10, TestDoor::LOCKPICKING_TO_PICK));
	thief.aim(door);
	CHECK(thief.input.execute(Approach::SKILL));
	CHECK(thief.input.feedback() == "You pick the lock.");
}

TEST_CASE("the test terminal opens for anyone and its security falls to enough Electronics")
{
	TestTerminal terminal = made<TestTerminal>("Test Terminal", "terminal");
	Rig novice(player(10, 0, TestTerminal::ELECTRONICS_TO_BYPASS - 1));
	novice.aim(terminal);
	CHECK(novice.input.execute(Approach::NORMAL));
	CHECK(novice.input.feedback() == "You access the terminal.");
	novice.press(KEY_DOWN);
	novice.press(KEY_KEY_R);
	CHECK(novice.input.feedback() == "Too advanced.");
	CHECK_FALSE(novice.input.feedbackSucceeded());

	Rig hacker(player(10, 0, TestTerminal::ELECTRONICS_TO_BYPASS));
	hacker.aim(terminal);
	hacker.press(KEY_KEY_3);
	CHECK(hacker.input.feedback() == "You bypass the security.");
	CHECK(hacker.input.feedbackSucceeded());
}

TEST_CASE("a container is a supply_crate whose Normal and Force both open the loot screen on that container, with different lines")
{
	loot::Container chest{ "Chest", 7.0f, -5.0f, 8, {} };
	std::vector<loot::Container*> opened;
	ContainerInteractable crate(chest, [&opened](loot::Container& container) { opened.push_back(&container); });
	CHECK(crate.getObjectType() == "supply_crate");
	CHECK(crate.getInfo().name == "Chest");
	CHECK(crate.getInfo().interactionRange == doctest::Approx(loot::REACH));
	CHECK(crate.getPosition().X == doctest::Approx(7.0f));
	CHECK(crate.getPosition().Z == doctest::Approx(-5.0f));

	Rig rig(player(10));
	rig.aim(crate);
	rig.press(KEY_UP);
	rig.press(KEY_KEY_R);
	CHECK(opened == std::vector<loot::Container*>{ &chest });
	CHECK(rig.input.feedback() == "You open the Chest.");
	rig.press(KEY_KEY_2);
	CHECK(opened == std::vector<loot::Container*>{ &chest, &chest });
	CHECK(rig.input.feedback() == "You smash the Chest open.");

	Actor actor{ rig.sheet, "" };
	CHECK_FALSE(crate.onSkill(actor));
	CHECK_FALSE(crate.onGuile(actor));
	CHECK(rig.log == std::vector<std::string>{ "OK: You open the Chest.", "OK: You smash the Chest open." });
}

TEST_CASE("a handler that fails without a line says Can't do that, and feedback clears after a few seconds")
{
	struct Stuck : public TestObject
	{
	};
	Stuck stuck;
	stuck.info.name = "Stuck";
	stuck.type = "unlocked_door";
	Rig rig(player(10));
	rig.aim(stuck);
	CHECK_FALSE(rig.input.execute(Approach::NORMAL));
	CHECK(rig.input.feedback() == "Can't do that.");
	rig.input.update(InteractionInput::FEEDBACK_SECONDS - 0.1f);
	CHECK(rig.input.feedback() == "Can't do that.");
	rig.input.update(0.2f);
	CHECK(rig.input.feedback().empty());
}

TEST_CASE("an approach the target lacks runs no handler and changes no feedback")
{
	TestTerminal terminal = made<TestTerminal>("Test Terminal", "terminal");
	Rig rig(player(10));
	rig.aim(terminal);
	CHECK_FALSE(rig.input.execute(Approach::FORCE));
	CHECK_FALSE(rig.input.execute(Approach::GUILE));
	CHECK(rig.input.feedback().empty());
	CHECK(rig.log.empty());
}
