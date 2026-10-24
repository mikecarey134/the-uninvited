#include "doctest.h"
#include "InputMode.h"

using namespace irr;

namespace
{
	SEvent keyEvent(EKEY_CODE code, bool down = true)
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

	SEvent padEvent(u32 buttons = 0, s16 axis0 = 0, u16 pov = 65535)
	{
		SEvent event;
		event.EventType = EET_JOYSTICK_INPUT_EVENT;
		event.JoystickEvent.ButtonStates = buttons;
		for (s16& axis : event.JoystickEvent.Axis)
			axis = 0;
		event.JoystickEvent.Axis[0] = axis0;
		event.JoystickEvent.POV = pov;
		event.JoystickEvent.Joystick = 0;
		return event;
	}

	SEvent mouseEvent(EMOUSE_INPUT_EVENT kind)
	{
		SEvent event;
		event.EventType = EET_MOUSE_INPUT_EVENT;
		event.MouseInput.Event = kind;
		event.MouseInput.X = 0;
		event.MouseInput.Y = 0;
		event.MouseInput.Wheel = 0.0f;
		event.MouseInput.Shift = false;
		event.MouseInput.Control = false;
		event.MouseInput.ButtonStates = 0;
		return event;
	}
}

TEST_CASE("input starts as keyboard and mouse; using the pad switches to gamepad and a real key or click switches back")
{
	InputSource source;
	CHECK(source.mode() == InputMode::KEYBOARD_MOUSE);
	source.observe(padEvent(1u << 2));
	CHECK(source.mode() == InputMode::GAMEPAD);
	source.observe(keyEvent(KEY_KEY_W));
	CHECK(source.mode() == InputMode::KEYBOARD_MOUSE);
	source.observe(padEvent(0, 0, 9000));
	CHECK(source.mode() == InputMode::GAMEPAD);
	source.observe(mouseEvent(EMIE_LMOUSE_PRESSED_DOWN));
	CHECK(source.mode() == InputMode::KEYBOARD_MOUSE);
	source.observe(padEvent(0, 20000));
	CHECK(source.mode() == InputMode::GAMEPAD);
}

TEST_CASE("an axis resting far from zero, like a Linux trigger at -32768, is not use; moving it is")
{
	InputSource source;
	for (int frame = 0; frame < 5; ++frame)
		source.observe(padEvent(0, -32768));
	CHECK(source.mode() == InputMode::KEYBOARD_MOUSE);
	source.observe(padEvent(0, 0));
	CHECK(source.mode() == InputMode::GAMEPAD);
	source.observe(keyEvent(KEY_KEY_E));
	source.observe(padEvent(0, 0));
	CHECK(source.mode() == InputMode::KEYBOARD_MOUSE);
}

TEST_CASE("an idle pad's every-frame report, a key release and mouse movement change nothing")
{
	InputSource source;
	source.observe(padEvent(0, 3000));
	source.observe(padEvent(0, 6000));
	CHECK(source.mode() == InputMode::KEYBOARD_MOUSE);
	source.observe(padEvent(1u));
	REQUIRE(source.mode() == InputMode::GAMEPAD);
	source.observe(keyEvent(KEY_KEY_W, false));
	source.observe(mouseEvent(EMIE_MOUSE_MOVED));
	CHECK(source.mode() == InputMode::GAMEPAD);
}

TEST_CASE("a key press the pad layer made up counts as the pad, says which part made it, and only while it is being posted")
{
	InputSource source;
	CHECK(source.pad() == PadPart::NONE);
	source.beginPad(PadPart::DPAD);
	source.observe(keyEvent(KEY_LEFT));
	CHECK(source.pad() == PadPart::DPAD);
	CHECK(source.mode() == InputMode::GAMEPAD);
	source.endPad();
	CHECK(source.pad() == PadPart::NONE);
	source.observe(keyEvent(KEY_LEFT));
	CHECK(source.mode() == InputMode::KEYBOARD_MOUSE);
}
