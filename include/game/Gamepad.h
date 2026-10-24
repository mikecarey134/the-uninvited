#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <functional>
#include "InputRouter.h"
#include "InputMode.h"
#include <irrlicht.h>

struct JoystickRecord
{
	irr::u32 time;
	irr::s16 value;
	irr::u8 type;
	irr::u8 number;
};

void applyHat(const JoystickRecord& record, irr::s16& x, irr::s16& y);
std::function<void(irr::s16&, irr::s16&)> linuxHat();

class GamepadLayer : public InputLayer
{
public:
	// press gets each made-up key and the part of the pad that made it: a face button, the D-pad (POV or hat) or the left stick.
	GamepadLayer(std::function<void(irr::EKEY_CODE, PadPart)> press, std::function<void(irr::s16&, irr::s16&)> hat = linuxHat());
	bool onEvent(const irr::SEvent& event) override;
	// The right stick, -1..1 each way (right and down positive), zero inside a small dead zone. On Linux pads (xpad layout) the
	// right stick is axes 3 and 4; axes 6 and 7 are the D-pad hat.
	irr::f32 rightX() const { return rightX_; }
	irr::f32 rightY() const { return rightY_; }

private:
	std::function<void(irr::EKEY_CODE, PadPart)> press_;
	std::function<void(irr::s16&, irr::s16&)> hat_;
	irr::u32 buttons_ = 0;
	irr::EKEY_CODE direction_ = irr::KEY_KEY_CODES_COUNT;
	irr::f32 rightX_ = 0.0f;
	irr::f32 rightY_ = 0.0f;
};

#endif
