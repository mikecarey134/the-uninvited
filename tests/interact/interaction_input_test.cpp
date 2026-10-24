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
		InteractionInput input{ targets, hud };
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
