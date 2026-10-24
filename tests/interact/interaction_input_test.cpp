#include "doctest.h"
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
	};

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

		Rig()
		{
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

TEST_CASE("a newly targeted door has nothing selected; X does nothing until a direction picks an approach")
{
	Rig rig;
	rig.targets.update(&rig.door, 1, rig.hud);
	CHECK(rig.targets.selected() == nullptr);
	CHECK(rig.press(KEY_KEY_R));
	CHECK(rig.log.empty());
	CHECK(rig.press(KEY_RIGHT));
	REQUIRE(rig.targets.selected() != nullptr);
	CHECK(rig.targets.selected()->type == Approach::FORCE);
	CHECK(rig.says(L"X: Kick Down"));
	CHECK(rig.press(KEY_KEY_R));
	REQUIRE(rig.log.size() == 1);
	CHECK(rig.log[0] == "INTERACT: Force on Test Door");
	rig.press(KEY_UP);
	rig.press(KEY_KEY_R);
	rig.press(KEY_DOWN);
	rig.press(KEY_KEY_R);
	rig.press(KEY_LEFT);
	rig.press(KEY_KEY_R);
	CHECK(rig.log == std::vector<std::string>{ "INTERACT: Force on Test Door", "INTERACT: Normal on Test Door", "INTERACT: Skill on Test Door",
		"INTERACT: Guile on Test Door" });
}

TEST_CASE("a direction whose approach the target lacks is taken but changes nothing")
{
	Rig rig;
	rig.targets.update(&rig.crate, 1, rig.hud);
	CHECK(rig.press(KEY_DOWN));
	CHECK(rig.targets.selected() == nullptr);
	rig.press(KEY_RIGHT);
	CHECK(rig.press(KEY_LEFT));
	REQUIRE(rig.targets.selected() != nullptr);
	CHECK(rig.targets.selected()->type == Approach::FORCE);
	rig.press(KEY_KEY_R);
	CHECK(rig.log == std::vector<std::string>{ "INTERACT: Force on Test Crate" });
}

TEST_CASE("a target with a single approach starts with it selected, so X runs it at once")
{
	Rig rig;
	rig.targets.update(&rig.open, 1, rig.hud);
	REQUIRE(rig.targets.selected() != nullptr);
	CHECK(rig.targets.selected()->type == Approach::NORMAL);
	CHECK(rig.says(L"X: Open Door"));
	rig.press(KEY_KEY_R);
	CHECK(rig.log == std::vector<std::string>{ "INTERACT: Normal on Open Door" });
}

TEST_CASE("the selection holds while the target does, and a different target starts fresh")
{
	Rig rig;
	rig.targets.update(&rig.door, 1, rig.hud);
	rig.press(KEY_RIGHT);
	rig.targets.update(&rig.door, 2, rig.hud);
	rig.targets.update(&rig.door, 1, rig.hud);
	REQUIRE(rig.targets.selected() != nullptr);
	CHECK(rig.targets.selected()->type == Approach::FORCE);
	CHECK(rig.says(L"X: Kick Down"));
	rig.targets.update(&rig.crate, 2, rig.hud);
	CHECK(rig.targets.selected() == nullptr);
	CHECK_FALSE(rig.says(L"X: Kick Down"));
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

TEST_CASE("E and left-click run Normal at once, and number keys run their approach directly")
{
	Rig rig;
	rig.targets.update(&rig.door, 1, rig.hud);
	CHECK(rig.press(KEY_KEY_E));
	CHECK(rig.input.onEvent(mouse(EMIE_LMOUSE_PRESSED_DOWN)));
	CHECK(rig.press(KEY_KEY_2));
	CHECK(rig.press(KEY_KEY_3));
	CHECK(rig.press(KEY_KEY_4));
	CHECK(rig.press(KEY_KEY_1));
	CHECK(rig.log == std::vector<std::string>{ "INTERACT: Normal on Test Door", "INTERACT: Normal on Test Door", "INTERACT: Force on Test Door",
		"INTERACT: Skill on Test Door", "INTERACT: Guile on Test Door", "INTERACT: Normal on Test Door" });
}

TEST_CASE("Q opens the panel with all four door approaches; a number key runs one and closes it")
{
	Rig rig;
	rig.targets.update(&rig.door, 1, rig.hud);
	CHECK(rig.press(KEY_KEY_Q));
	REQUIRE(rig.panel.isOpen());
	CHECK(labels(rig.panel) == std::vector<std::string>{ "1 Try Handle modes/normal.png", "2 Kick Down modes/force.png", "3 Pick Lock modes/skill.png",
		"4 Send Companion modes/guile.png" });
	rig.press(KEY_KEY_2);
	CHECK(rig.log == std::vector<std::string>{ "INTERACT: Force on Test Door" });
	CHECK_FALSE(rig.panel.isOpen());
}

TEST_CASE("the crate's panel has only Normal and Force, and 3 and 4 do nothing")
{
	Rig rig;
	rig.targets.update(&rig.crate, 1, rig.hud);
	rig.press(KEY_TAB);
	REQUIRE(rig.panel.isOpen());
	CHECK(labels(rig.panel) == std::vector<std::string>{ "1 Open Crate modes/normal.png", "2 Smash Open modes/force.png" });
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
	rig.input.update();
	CHECK(rig.panel.isOpen());
	rig.targets.update(&rig.crate, 2, rig.hud);
	rig.input.update();
	CHECK_FALSE(rig.panel.isOpen());
	rig.press(KEY_KEY_Q);
	rig.targets.update(nullptr, 0, rig.hud);
	rig.input.update();
	CHECK_FALSE(rig.panel.isOpen());
	CHECK(rig.log.empty());
}

TEST_CASE("clicking a panel row runs that approach; a click off the rows runs nothing; moving the mouse highlights the row")
{
	Rig rig;
	rig.targets.update(&rig.crate, 1, rig.hud);
	rig.press(KEY_KEY_Q);
	position2d<s32> force = rig.panel.rowArea(1).getCenter();
	CHECK(rig.input.onEvent(mouse(EMIE_MOUSE_MOVED, force)));
	CHECK(rig.panel.hovered() == 1);
	CHECK(rig.input.onEvent(mouse(EMIE_LMOUSE_PRESSED_DOWN, position2d<s32>(5, 5))));
	CHECK(rig.log.empty());
	CHECK(rig.panel.isOpen());
	rig.input.onEvent(mouse(EMIE_LMOUSE_PRESSED_DOWN, force));
	CHECK(rig.log == std::vector<std::string>{ "INTERACT: Force on Test Crate" });
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
