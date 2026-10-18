#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <functional>
#include "InputRouter.h"
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
	GamepadLayer(std::function<void(irr::EKEY_CODE)> press, std::function<void(irr::s16&, irr::s16&)> hat = linuxHat());
	bool onEvent(const irr::SEvent& event) override;

private:
	std::function<void(irr::EKEY_CODE)> press_;
	std::function<void(irr::s16&, irr::s16&)> hat_;
	irr::u32 buttons_ = 0;
	irr::EKEY_CODE direction_ = irr::KEY_KEY_CODES_COUNT;
};

#endif
