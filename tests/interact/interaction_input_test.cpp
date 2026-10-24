#include "doctest.h"
#include "CharacterSheet.h"
#include "InteractionInput.h"
#include "RecordingCanvas.h"
#include <string>
#include <vector>

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
			return info;
		}
		vector3df getPosition() const override { return vector3df(); }
		std::string getObjectType() const override { return type; }
		bool report(Actor& player, const char* approach)
		{
			player.message = std::string(approach) + " on " + name;
			return true;
		}
		bool onNormal(Actor& player) override { return report(player, "Normal"); }
		bool onForce(Actor& player) override { return report(player, "Force"); }
		bool onSkill(Actor& player) override { return report(player, ("Skill(" + player.skill + ")").c_str()); }
		bool onGuile(Actor& player) override { return report(player, "Guile"); }
	};

	const character::Profession medic{ "Medic", { { character::MEDICINE, 30 } }, {} };

	SEvent key(EKEY_CODE code, bool down = true)
	{
		SEvent event;
		event.EventType = EET_KEY_INPUT_EVENT;
		event.KeyInput.Key = code;
		event.KeyInput.PressedDown = down;
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
		std::vector<std::string> log;
		Thing door{ "Test Door", "locked_door" };
		Thing crate{ "Test Crate", "supply_crate" };
		Thing open{ "Open Door", "unlocked_door" };

		character::CharacterSheet sheet = character::CharacterSheet::create("Ivy", character::Gender::Male, medic);

		Rig()
		{
			input.setPlayer(&sheet);
			input.setLog([this](const std::string& line) { log.push_back(line); });
		}

		bool press(EKEY_CODE code) { return input.onEvent(key(code)); }

		bool says(const std::wstring& text)
		{
			RecordingCanvas canvas;
			hud.render(canvas);
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::TEXT && call.text == text)
					return true;
			return false;
		}
	};
}

TEST_CASE("with nothing targeted the D-pad and X pass through untouched")
{
	Rig rig;
	for (EKEY_CODE code : { KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_LEFT, KEY_KEY_R })
		CHECK_FALSE(rig.press(code));
	CHECK(rig.log.empty());
}

TEST_CASE("a newly targeted door has nothing picked and no prompt, and X runs Normal; a direction picks a slot and X runs that instead")
{
	Rig rig;
	rig.targets.update(&rig.door, 1, rig.hud);
	CHECK(rig.targets.selected() == nullptr);
	CHECK_FALSE(rig.says(L"X: Try Handle"));
	CHECK(rig.press(KEY_KEY_R));
	CHECK(rig.log == std::vector<std::string>{ "OK: Normal on Test Door" });
	CHECK(rig.press(KEY_LEFT));
	REQUIRE(rig.targets.selected() != nullptr);
	CHECK(rig.targets.selected()->type == Approach::FORCE);
	CHECK(rig.targets.promptAction() == "Open");
	CHECK_FALSE(rig.says(L"X: Kick Down"));
	rig.press(KEY_KEY_R);
	rig.press(KEY_UP);
	rig.press(KEY_KEY_R);
	rig.press(KEY_DOWN);
	rig.press(KEY_KEY_R);
	rig.press(KEY_RIGHT);
	rig.press(KEY_KEY_R);
	CHECK(rig.log == std::vector<std::string>{ "OK: Normal on Test Door", "OK: Force on Test Door", "OK: Skill(lockpicking) on Test Door",
		"OK: Skill(electronics) on Test Door", "OK: Guile on Test Door" });
}

TEST_CASE("pressing the picked direction again unpicks it, so X goes back to Normal")
{
	Rig rig;
	rig.targets.update(&rig.door, 1, rig.hud);
	rig.press(KEY_UP);
	REQUIRE(rig.targets.selected() != nullptr);
	CHECK(rig.press(KEY_UP));
	CHECK(rig.targets.selected() == nullptr);
	CHECK_FALSE(rig.says(L"X: Try Handle"));
	rig.press(KEY_KEY_R);
	CHECK(rig.log == std::vector<std::string>{ "OK: Normal on Test Door" });
}

TEST_CASE("an empty slot is taken but changes nothing: the crate has only Force, on the left")
{
	Rig rig;
	rig.targets.update(&rig.crate, 1, rig.hud);
	for (EKEY_CODE code : { KEY_UP, KEY_RIGHT, KEY_DOWN })
		CHECK(rig.press(code));
	CHECK(rig.targets.selected() == nullptr);
	CHECK(rig.press(KEY_LEFT));
	REQUIRE(rig.targets.selected() != nullptr);
	CHECK(rig.targets.selected()->type == Approach::FORCE);
	rig.press(KEY_DOWN);
	CHECK(rig.targets.selected()->type == Approach::FORCE);
	rig.press(KEY_KEY_R);
	CHECK(rig.log == std::vector<std::string>{ "OK: Force on Test Crate" });
}

TEST_CASE("a target with only Normal shows no slots and no prompt, and X runs it")
{
	Rig rig;
	rig.targets.update(&rig.open, 1, rig.hud);
	CHECK(rig.targets.selected() == nullptr);
	for (DpadDirection slot : { DpadDirection::UP, DpadDirection::RIGHT, DpadDirection::DOWN, DpadDirection::LEFT })
		CHECK(rig.targets.optionAt(slot) == nullptr);
	CHECK_FALSE(rig.says(L"X: Open Door"));
	rig.press(KEY_KEY_R);
	CHECK(rig.log == std::vector<std::string>{ "OK: Normal on Open Door" });
}

TEST_CASE("the pick holds while the target does, and a different target starts fresh")
{
	Rig rig;
	rig.targets.update(&rig.door, 1, rig.hud);
	rig.press(KEY_LEFT);
	rig.targets.update(&rig.door, 2, rig.hud);
	rig.targets.update(&rig.door, 1, rig.hud);
	REQUIRE(rig.targets.selected() != nullptr);
	CHECK(rig.targets.selected()->type == Approach::FORCE);
	CHECK(rig.targets.promptAction() == "Open");
	CHECK_FALSE(rig.says(L"X: Kick Down"));
	rig.targets.update(&rig.crate, 2, rig.hud);
	CHECK(rig.targets.selected() == nullptr);
	CHECK_FALSE(rig.says(L"X: Kick Down"));
	CHECK_FALSE(rig.says(L"X: Open Crate"));
	rig.targets.update(&rig.door, 2, rig.hud);
	CHECK(rig.targets.selected() == nullptr);
}

TEST_CASE("losing the target clears the selection and lets the keys through again")
{
	Rig rig;
	rig.targets.update(&rig.door, 1, rig.hud);
	rig.press(KEY_RIGHT);
	rig.targets.update(nullptr, 0, rig.hud);
	CHECK(rig.targets.selected() == nullptr);
	CHECK_FALSE(rig.press(KEY_KEY_R));
	CHECK_FALSE(rig.press(KEY_RIGHT));
	CHECK(rig.log.empty());
}

TEST_CASE("key releases are never taken")
{
	Rig rig;
	rig.targets.update(&rig.door, 1, rig.hud);
	CHECK_FALSE(rig.input.onEvent(key(KEY_RIGHT, false)));
	CHECK_FALSE(rig.input.onEvent(key(KEY_KEY_R, false)));
}

namespace
{
	SEvent mouse(EMOUSE_INPUT_EVENT kind, position2d<s32> at = position2d<s32>(0, 0))
	{
		SEvent event;
		event.EventType = EET_MOUSE_INPUT_EVENT;
		event.MouseInput.Event = kind;
		event.MouseInput.X = at.X;
		event.MouseInput.Y = at.Y;
		event.MouseInput.Wheel = 0.0f;
		event.MouseInput.Shift = false;
		event.MouseInput.Control = false;
		event.MouseInput.ButtonStates = 0;
		return event;
	}

	std::vector<std::string> labels(const ContextPanel& panel)
	{
		std::vector<std::string> out;
		for (const ContextRow& row : panel.rows())
			out.push_back(std::to_string(row.number) + " " + row.label + " " + row.iconPath);
		return out;
	}
}

TEST_CASE("with nothing targeted the PC keys and both mouse buttons pass through untouched")
{
	Rig rig;
	for (EKEY_CODE code : { KEY_KEY_E, KEY_KEY_Q, KEY_TAB, KEY_KEY_1, KEY_KEY_2, KEY_KEY_3, KEY_KEY_4, KEY_ESCAPE })
		CHECK_FALSE(rig.press(code));
	CHECK_FALSE(rig.input.onEvent(mouse(EMIE_LMOUSE_PRESSED_DOWN)));
	CHECK_FALSE(rig.input.onEvent(mouse(EMIE_RMOUSE_PRESSED_DOWN)));
	CHECK_FALSE(rig.panel.isOpen());
	CHECK(rig.log.empty());
}

TEST_CASE("E and left-click run Normal at once, and 1-4 run Force, Skill, the second Skill and Guile directly")
{
	Rig rig;
	rig.targets.update(&rig.door, 1, rig.hud);
	CHECK(rig.press(KEY_KEY_E));
	CHECK(rig.input.onEvent(mouse(EMIE_LMOUSE_PRESSED_DOWN)));
	CHECK(rig.press(KEY_KEY_1));
	CHECK(rig.press(KEY_KEY_2));
	CHECK(rig.press(KEY_KEY_3));
	CHECK(rig.press(KEY_KEY_4));
	CHECK(rig.log == std::vector<std::string>{ "OK: Normal on Test Door", "OK: Normal on Test Door", "OK: Force on Test Door",
		"OK: Skill(lockpicking) on Test Door", "OK: Skill(electronics) on Test Door", "OK: Guile on Test Door" });
}

TEST_CASE("Q opens the panel with the door's four non-Normal approaches; a number key runs one and closes it")
{
	Rig rig;
	rig.targets.update(&rig.door, 1, rig.hud);
	CHECK(rig.press(KEY_KEY_Q));
	REQUIRE(rig.panel.isOpen());
	CHECK(labels(rig.panel) == std::vector<std::string>{ "1 Kick Down modes/force.png", "2 Pick Lock modes/skill.png", "3 Hack Keypad modes/skill.png",
		"4 Send Companion modes/guile.png" });
	rig.press(KEY_KEY_1);
	CHECK(rig.log == std::vector<std::string>{ "OK: Force on Test Door" });
	CHECK_FALSE(rig.panel.isOpen());
}

TEST_CASE("the crate's panel has only Force, and 2, 3 and 4 do nothing")
{
	Rig rig;
	rig.targets.update(&rig.crate, 1, rig.hud);
	rig.press(KEY_TAB);
	REQUIRE(rig.panel.isOpen());
	CHECK(labels(rig.panel) == std::vector<std::string>{ "1 Smash Open modes/force.png" });
	CHECK(rig.press(KEY_KEY_2));
	CHECK(rig.press(KEY_KEY_3));
	CHECK(rig.press(KEY_KEY_4));
	CHECK(rig.log.empty());
	CHECK(rig.panel.isOpen());
}

TEST_CASE("the panel closes on its open key again, on Escape, and when the target is lost or changes")
{
	Rig rig;
	rig.targets.update(&rig.door, 1, rig.hud);
	rig.press(KEY_KEY_Q);
	rig.press(KEY_KEY_Q);
	CHECK_FALSE(rig.panel.isOpen());
	rig.input.onEvent(mouse(EMIE_RMOUSE_PRESSED_DOWN));
	CHECK(rig.panel.isOpen());
	rig.input.onEvent(mouse(EMIE_RMOUSE_PRESSED_DOWN));
	CHECK_FALSE(rig.panel.isOpen());
	rig.press(KEY_TAB);
	CHECK(rig.press(KEY_ESCAPE));
	CHECK_FALSE(rig.panel.isOpen());
	CHECK_FALSE(rig.press(KEY_ESCAPE));

	rig.press(KEY_KEY_Q);
	rig.targets.update(&rig.door, 2, rig.hud);
	rig.input.update(0.0f);
	CHECK(rig.panel.isOpen());
	rig.targets.update(&rig.crate, 2, rig.hud);
	rig.input.update(0.0f);
	CHECK_FALSE(rig.panel.isOpen());
	rig.press(KEY_KEY_Q);
	rig.targets.update(nullptr, 0, rig.hud);
	rig.input.update(0.0f);
	CHECK_FALSE(rig.panel.isOpen());
	CHECK(rig.log.empty());
}

TEST_CASE("clicking a panel row runs that approach; a click off the rows runs nothing; moving the mouse highlights the row")
{
	Rig rig;
	rig.targets.update(&rig.door, 1, rig.hud);
	rig.press(KEY_KEY_Q);
	position2d<s32> hack = rig.panel.rowArea(2).getCenter();
	CHECK(rig.input.onEvent(mouse(EMIE_MOUSE_MOVED, hack)));
	CHECK(rig.panel.hovered() == 2);
	CHECK(rig.input.onEvent(mouse(EMIE_LMOUSE_PRESSED_DOWN, position2d<s32>(5, 5))));
	CHECK(rig.log.empty());
	CHECK(rig.panel.isOpen());
	rig.input.onEvent(mouse(EMIE_LMOUSE_PRESSED_DOWN, hack));
	CHECK(rig.log == std::vector<std::string>{ "OK: Skill(electronics) on Test Door" });
	CHECK_FALSE(rig.panel.isOpen());
}


TEST_CASE("a target with no Normal approach ignores E and left-click")
{
	Rig rig;
	Thing odd("Odd Thing", "toaster");
	rig.targets.update(&odd, 1, rig.hud);
	CHECK(rig.press(KEY_KEY_E));
	CHECK(rig.input.onEvent(mouse(EMIE_LMOUSE_PRESSED_DOWN)));
	CHECK(rig.log.empty());
}

TEST_CASE("while the console or a screen is open every key and click passes through, so typing a 3 in the console runs nothing")
{
	Rig rig;
	bool console = true;
	rig.input.setBlocked([&console] { return console; });
	rig.targets.update(&rig.door, 1, rig.hud);
	for (EKEY_CODE code : { KEY_KEY_3, KEY_KEY_2, KEY_KEY_E, KEY_KEY_Q, KEY_TAB, KEY_RIGHT, KEY_KEY_R })
		CHECK_FALSE(rig.press(code));
	CHECK_FALSE(rig.input.onEvent(mouse(EMIE_LMOUSE_PRESSED_DOWN)));
	CHECK(rig.log.empty());
	CHECK_FALSE(rig.panel.isOpen());
	console = false;
	CHECK(rig.press(KEY_KEY_3));
	CHECK(rig.log == std::vector<std::string>{ "OK: Skill(electronics) on Test Door" });
}

TEST_CASE("E and left-click do what the prompt says: the picked slot if there is one, else Normal")
{
	Rig rig;
	rig.targets.update(&rig.door, 1, rig.hud);
	rig.press(KEY_LEFT);
	CHECK(rig.press(KEY_KEY_E));
	CHECK(rig.input.onEvent(mouse(EMIE_LMOUSE_PRESSED_DOWN)));
	rig.press(KEY_LEFT);
	rig.press(KEY_KEY_E);
	CHECK(rig.log == std::vector<std::string>{ "OK: Force on Test Door", "OK: Force on Test Door", "OK: Normal on Test Door" });
}
