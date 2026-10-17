#include "Gamepad.h"

using namespace irr;

namespace
{
	const s16 deadZone = 16000;

	EKEY_CODE pointing(s16 x, s16 y)
	{
		if (x > deadZone)
			return KEY_RIGHT;
		if (x < -deadZone)
			return KEY_LEFT;
		if (y > deadZone)
			return KEY_DOWN;
		if (y < -deadZone)
			return KEY_UP;
		return KEY_KEY_CODES_COUNT;
	}
}

GamepadLayer::GamepadLayer(std::function<void(EKEY_CODE)> press) : press_(press)
{
}

bool GamepadLayer::onEvent(const SEvent& event)
{
	if (event.EventType != EET_JOYSTICK_INPUT_EVENT)
		return false;
	const SEvent::SJoystickEvent& pad = event.JoystickEvent;
	const struct { u32 button; EKEY_CODE key; } buttons[] = { { 0, KEY_RETURN }, { 1, KEY_ESCAPE }, { 2, KEY_KEY_R }, { 7, KEY_KEY_I }, { 6, KEY_KEY_C } };
	for (const auto& entry : buttons)
		if (pad.IsButtonPressed(entry.button) && !(buttons_ & (1u << entry.button)))
			press_(entry.key);
	buttons_ = pad.ButtonStates;

	EKEY_CODE direction = KEY_KEY_CODES_COUNT;
	if (pad.POV != 65535)
	{
		const EKEY_CODE quarters[4] = { KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_LEFT };
		direction = quarters[((pad.POV + 4500) / 9000) % 4];
	}
	if (direction == KEY_KEY_CODES_COUNT)
		direction = pointing(pad.Axis[0], pad.Axis[1]);
	if (direction != direction_ && direction != KEY_KEY_CODES_COUNT)
		press_(direction);
	direction_ = direction;
	return true;
}
