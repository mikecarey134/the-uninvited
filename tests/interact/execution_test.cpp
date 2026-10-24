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
		InputSource source;
		character::CharacterSheet sheet;
		std::vector<std::string> log;

		explicit Rig(const character::CharacterSheet& sheet) : sheet(sheet)
		{
			input.setPlayer(&this->sheet);
			input.setSource(&source);
			input.setLog([this](const std::string& line) { log.push_back(line); });
		}

		void aim(Interactable& target) { targets.update(&target, 1, hud); }
		void press(EKEY_CODE code) { input.onEvent(key(code)); }
		// A press the gamepad layer made up: arrows from its D-pad, R from its X button.
		void pad(EKEY_CODE code)
		{
			source.beginPad(code == KEY_KEY_R ? PadPart::BUTTON : PadPart::DPAD);
			input.onEvent(key(code));
			source.endPad();
		}
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

TEST_CASE("the test door's first skill checks Lockpicking and its second Electronics, its handle is locked, and Guile sends the companion")
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
	thief.pad(KEY_DOWN);
	thief.pad(KEY_KEY_R);
	CHECK(thief.input.feedback() == "Keypad is too advanced.");

	Rig hacker(player(10, 0, TestDoor::ELECTRONICS_TO_HACK));
	hacker.aim(door);
	hacker.press(KEY_KEY_3);
	CHECK(hacker.input.feedback() == "You hack the keypad.");
	CHECK(hacker.input.feedbackSucceeded());
	hacker.press(KEY_KEY_2);
	CHECK(hacker.input.feedback() == "Lock is too complex.");
}

TEST_CASE("the test terminal opens for anyone and its security falls to enough Electronics")
{
	TestTerminal terminal = made<TestTerminal>("Test Terminal", "terminal");
	Rig novice(player(10, 0, TestTerminal::ELECTRONICS_TO_BYPASS - 1));
	novice.aim(terminal);
	CHECK(novice.input.execute(Approach::NORMAL));
	CHECK(novice.input.feedback() == "You access the terminal.");
	novice.pad(KEY_UP);
	novice.pad(KEY_KEY_R);
	CHECK(novice.input.feedback() == "Too advanced.");
	CHECK_FALSE(novice.input.feedbackSucceeded());

	Rig hacker(player(10, 0, TestTerminal::ELECTRONICS_TO_BYPASS));
	hacker.aim(terminal);
	hacker.press(KEY_KEY_2);
	CHECK(hacker.input.feedback() == "You bypass the security.");
	CHECK(hacker.input.feedbackSucceeded());
}

TEST_CASE("a container is a supply_crate whose Normal (X with nothing picked) and Force (1) both open the loot screen on that container; the screen is the feedback, so only the console gets a line")
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
	rig.pad(KEY_KEY_R);
	CHECK(opened == std::vector<loot::Container*>{ &chest });
	CHECK(rig.input.feedback().empty());
	rig.press(KEY_KEY_1);
	CHECK(opened == std::vector<loot::Container*>{ &chest, &chest });
	CHECK(rig.input.feedback().empty());

	Actor actor{ rig.sheet, "", "" };
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

#include <fstream>

namespace
{
	struct Heard
	{
		std::vector<Cue> cues;
		void attach(InteractionInput& input)
		{
			input.setCue([this](Cue cue) { cues.push_back(cue); });
		}
	};

	// A WAV's length from its header: data bytes over bytes per second.
	float seconds(const std::string& path)
	{
		std::ifstream in(path, std::ios::binary);
		std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		if (bytes.size() < 44 || std::string(bytes.begin(), bytes.begin() + 4) != "RIFF")
			return -1.0f;
		auto u32 = [&bytes](size_t at) { return (uint32_t)(unsigned char)bytes[at] | (uint32_t)(unsigned char)bytes[at + 1] << 8 |
			(uint32_t)(unsigned char)bytes[at + 2] << 16 | (uint32_t)(unsigned char)bytes[at + 3] << 24; };
		uint32_t rate = 0;
		for (size_t at = 12; at + 8 <= bytes.size();)
		{
			std::string id(bytes.begin() + at, bytes.begin() + at + 4);
			uint32_t size = u32(at + 4);
			if (id == "fmt ")
				rate = u32(at + 16);
			if (id == "data")
				return rate ? (float)size / rate : -1.0f;
			at += 8 + size + (size & 1);
		}
		return -1.0f;
	}
}

TEST_CASE("picking a filled slot ticks, unpicking it ticks, and a direction with nothing in it is silent")
{
	TestDoor door = made<TestDoor>("Test Door", "locked_door");
	Rig rig(player(10));
	Heard heard;
	heard.attach(rig.input);
	rig.aim(door);
	rig.pad(KEY_LEFT);
	rig.pad(KEY_LEFT);
	CHECK(heard.cues == std::vector<Cue>{ Cue::Select, Cue::Select });
	ContainerInteractable crate(*new loot::Container{ "Crate", 0.0f, 0.0f, 4, {} }, nullptr);
	rig.aim(crate);
	heard.cues.clear();
	rig.pad(KEY_UP);
	rig.pad(KEY_RIGHT);
	rig.pad(KEY_DOWN);
	CHECK(heard.cues.empty());
	delete &crate.container();
}

TEST_CASE("running an approach sounds its outcome: success, failure, or the rattle for Normal on a locked door")
{
	TestDoor door = made<TestDoor>("Test Door", "locked_door");
	TestTerminal terminal = made<TestTerminal>("Test Terminal", "terminal");
	Rig rig(player(TestDoor::STRENGTH_TO_KICK - 1));
	Heard heard;
	heard.attach(rig.input);
	rig.aim(door);
	rig.press(KEY_KEY_E);
	rig.press(KEY_KEY_1);
	rig.press(KEY_KEY_4);
	rig.aim(terminal);
	rig.press(KEY_KEY_E);
	rig.press(KEY_KEY_2);
	CHECK(heard.cues == std::vector<Cue>{ Cue::Locked, Cue::Failure, Cue::Success, Cue::Success, Cue::Failure });
}

TEST_CASE("the target ping sounds once when a target appears from nothing, not while it stays or when aim moves to another")
{
	TestDoor door = made<TestDoor>("Test Door", "locked_door");
	TestTerminal terminal = made<TestTerminal>("Test Terminal", "terminal");
	Rig rig(player(10));
	Heard heard;
	heard.attach(rig.input);
	rig.input.update(0.1f);
	CHECK(heard.cues.empty());
	rig.aim(door);
	for (int i = 0; i < 5; ++i)
		rig.input.update(0.1f);
	CHECK(heard.cues == std::vector<Cue>{ Cue::Target });
	rig.aim(terminal);
	rig.input.update(0.1f);
	CHECK(heard.cues.size() == 1);
	rig.targets.update(nullptr, 0, rig.hud);
	rig.input.update(0.1f);
	rig.aim(door);
	rig.input.update(0.1f);
	CHECK(heard.cues == std::vector<Cue>{ Cue::Target, Cue::Target });
}

TEST_CASE("every cue's sound file is in the assets and short, the ping and tick quietest by design")
{
	for (Cue cue : { Cue::Select, Cue::Success, Cue::Failure, Cue::Locked, Cue::Target })
	{
		CueSound sound = cueSound(cue);
		std::string path = std::string(ASSETS_DIR) + std::string(sound.file).substr(std::string("assets").size());
		CAPTURE(path);
		float length = seconds(path);
		CHECK(length > 0.05f);
		CHECK(length <= 0.25f);
		CHECK(sound.gain > 0.0f);
		CHECK(sound.gain <= 1.0f);
	}
	CHECK(cueSound(Cue::Select).gain < cueSound(Cue::Success).gain);
}
