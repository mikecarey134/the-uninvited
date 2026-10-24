#include "InputMode.h"
#include <cstdlib>

using namespace irr;

namespace
{
	const s32 stickActive = 8000; // the same dead zone as the right-stick camera
}

void InputSource::observe(const SEvent& event)
{
	if (pad_ != PadPart::NONE)
	{
		mode_ = InputMode::GAMEPAD;
		return;
	}
	switch (event.EventType)
	{
	case EET_KEY_INPUT_EVENT:
		if (event.KeyInput.PressedDown)
			mode_ = InputMode::KEYBOARD_MOUSE;
		break;
	case EET_MOUSE_INPUT_EVENT:
		if (event.MouseInput.Event != EMIE_MOUSE_MOVED)
			mode_ = InputMode::KEYBOARD_MOUSE;
		break;
	case EET_JOYSTICK_INPUT_EVENT:
	{
		// Irrlicht sends pad state every frame, so only a pressed button, a hat, or an axis that moves counts as using it. An axis
		// can rest far from zero (Linux reports an untouched trigger at -32768), so it is its change that counts, not its value.
		const SEvent::SJoystickEvent& pad = event.JoystickEvent;
		bool used = pad.ButtonStates != 0 || pad.POV != 65535;
		for (s32 axis = 0; axis < SEvent::SJoystickEvent::NUMBER_OF_AXES; ++axis)
		{
			if (seen_ && std::abs((s32)pad.Axis[axis] - (s32)axes_[axis]) > stickActive)
				used = true;
			axes_[axis] = pad.Axis[axis];
		}
		seen_ = true;
		if (used)
			mode_ = InputMode::GAMEPAD;
		break;
	}
	default:
		break;
	}
}
