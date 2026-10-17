#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <functional>
#include "InputRouter.h"
#include <irrlicht.h>

class GamepadLayer : public InputLayer
{
public:
	explicit GamepadLayer(std::function<void(irr::EKEY_CODE)> press);
	bool onEvent(const irr::SEvent& event) override;

private:
	std::function<void(irr::EKEY_CODE)> press_;
	irr::u32 buttons_ = 0;
	irr::EKEY_CODE direction_ = irr::KEY_KEY_CODES_COUNT;
};

#endif
