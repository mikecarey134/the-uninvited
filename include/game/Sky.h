#ifndef SKY_H
#define SKY_H

#include <irrlicht.h>

struct SkyLook
{
	irr::video::SColor zenith;
	irr::video::SColor horizon;
	irr::video::SColorf ambient;
	irr::video::SColorf sun;
	irr::core::vector3df sunDirection;
};

SkyLook skyAt(irr::f32 hours);

#endif
