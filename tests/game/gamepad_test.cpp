#include "doctest.h"
#include "Gamepad.h"
#include <vector>

using namespace irr;

namespace
{
	struct Pad
	{
		Pad() : layer([this](EKEY_CODE key) { keys.push_back(key); }, [this](s16& x, s16& y) { x = hatX; y = hatY; }) {}

		bool send(u32 buttons, s16 x = 0, s16 y = 0, u16 pov = 65535)
		{
			SEvent event;
			event.EventType = EET_JOYSTICK_INPUT_EVENT;
			event.JoystickEvent.ButtonStates = buttons;
			for (s16& axis : event.JoystickEvent.Axis)
				axis = 0;
			event.JoystickEvent.Axis[0] = x;
			event.JoystickEvent.Axis[1] = y;
			event.JoystickEvent.POV = pov;
			event.JoystickEvent.Joystick = 0;
			return layer.onEvent(event);
		}

		s16 hatX = 0;
		s16 hatY = 0;
		std::vector<EKEY_CODE> keys;
		GamepadLayer layer;
	};
}

TEST_CASE("each face and menu button presses its key once when it goes down: A Enter, B Escape, X R, Start I and Back C")
{
	Pad pad;
	const struct { int button; EKEY_CODE key; } map[] = { { 0, KEY_RETURN }, { 1, KEY_ESCAPE }, { 2, KEY_KEY_R }, { 7, KEY_KEY_I }, { 6, KEY_KEY_C } };
	for (const auto& entry : map)
	{
		INFO(entry.button);
		pad.keys.clear();
		CHECK(pad.send(1u << entry.button));
		CHECK(pad.send(1u << entry.button));
		CHECK(pad.send(0));
		REQUIRE(pad.keys.size() == 1);
		CHECK(pad.keys[0] == entry.key);
	}
	pad.keys.clear();
	pad.send(1u << 3);
	pad.send(1u << 4);
	pad.send(0);
	CHECK(pad.keys.empty());
}

TEST_CASE("the left stick and the POV hat each press an arrow once per push, past a dead zone")
{
	Pad pad;
	pad.send(0, 30000, 0);
	pad.send(0, 30000, 0);
	pad.send(0, 0, 0);
	pad.send(0, 0, -30000);
	pad.send(0, 5000, 5000);
	pad.send(0, 0, 30000);
	pad.send(0, -30000, 0);
	pad.send(0, 0, 0, 0);
	pad.send(0, 0, 0, 9000);
	pad.send(0, 0, 0, 18000);
	pad.send(0, 0, 0, 27000);
	pad.send(0, 0, 0, 31500);
	pad.send(0, 0, 0, 65535);
	pad.send(0, 0, 0, 0);
	const std::vector<EKEY_CODE> expected = { KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_LEFT, KEY_UP, KEY_UP };
	CHECK(pad.keys == expected);
}

TEST_CASE("the gamepad layer leaves every other event alone")
{
	Pad pad;
	SEvent event;
	event.EventType = EET_KEY_INPUT_EVENT;
	event.KeyInput.Key = KEY_RETURN;
	event.KeyInput.PressedDown = true;
	CHECK_FALSE(pad.layer.onEvent(event));
	CHECK(pad.keys.empty());
}

TEST_CASE("the Linux D-pad, read as hat axes 6 and 7, presses an arrow once per push like the POV hat, and the stick still works beside it")
{
	Pad pad;
	pad.hatX = -32767;
	pad.send(0);
	pad.send(0);
	pad.hatX = 0;
	pad.send(0);
	pad.hatY = -32767;
	pad.send(0);
	pad.hatY = 32767;
	pad.send(0);
	pad.hatY = 0;
	pad.hatX = 32767;
	pad.send(0);
	pad.hatX = 0;
	pad.send(0);
	pad.send(0, 30000, 0);
	pad.send(0);
	pad.hatX = 5000;
	pad.send(0);
	const std::vector<EKEY_CODE> expected = { KEY_LEFT, KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_RIGHT };
	CHECK(pad.keys == expected);
}

TEST_CASE("a Linux joystick record sets the hat from axes 6 and 7, including the opening snapshot, and leaves the rest alone")
{
	s16 x = 0;
	s16 y = 0;
	applyHat(JoystickRecord{ 0, -32767, 0x02, 6 }, x, y);
	CHECK(x == -32767);
	applyHat(JoystickRecord{ 0, 32767, 0x82, 7 }, x, y);
	CHECK(y == 32767);
	applyHat(JoystickRecord{ 0, 1234, 0x02, 0 }, x, y);
	applyHat(JoystickRecord{ 0, 1, 0x01, 6 }, x, y);
	applyHat(JoystickRecord{ 0, 1, 0x01, 7 }, x, y);
	CHECK(x == -32767);
	CHECK(y == 32767);
	CHECK(sizeof(JoystickRecord) == 8);
}
