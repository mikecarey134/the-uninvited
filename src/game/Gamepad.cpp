#include "Gamepad.h"
#include <memory>
#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#endif

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

void applyHat(const JoystickRecord& record, s16& x, s16& y)
{
	if ((record.type & 0x7F) != 0x02)
		return;
	if (record.number == 6)
		x = record.value;
	else if (record.number == 7)
		y = record.value;
}

std::function<void(s16&, s16&)> linuxHat()
{
#ifdef __linux__
	struct Device
	{
		int file = open("/dev/input/js0", O_RDONLY | O_NONBLOCK);
		s16 x = 0;
		s16 y = 0;
		~Device()
		{
			if (file >= 0)
				close(file);
		}
	};
	std::shared_ptr<Device> device = std::make_shared<Device>();
	return [device](s16& x, s16& y) {
		JoystickRecord record;
		while (device->file >= 0 && read(device->file, &record, sizeof(record)) == (ssize_t)sizeof(record))
			applyHat(record, device->x, device->y);
		x = device->x;
		y = device->y;
	};
#else
	return nullptr;
#endif
}

GamepadLayer::GamepadLayer(std::function<void(EKEY_CODE)> press, std::function<void(s16&, s16&)> hat) : press_(press), hat_(hat)
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
	if (direction == KEY_KEY_CODES_COUNT && hat_)
	{
		s16 x = 0;
		s16 y = 0;
		hat_(x, y);
		direction = pointing(x, y);
	}
	if (direction == KEY_KEY_CODES_COUNT)
		direction = pointing(pad.Axis[0], pad.Axis[1]);
	if (direction != direction_ && direction != KEY_KEY_CODES_COUNT)
		press_(direction);
	direction_ = direction;
	return true;
}
