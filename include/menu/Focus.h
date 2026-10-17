#pragma once
#include <vector>
#include <irrlicht.h>

namespace focus
{
	int first(const std::vector<irr::core::rect<irr::s32> >& areas);
	int step(const std::vector<irr::core::rect<irr::s32> >& areas, int current, bool backward);
	int toward(const std::vector<irr::core::rect<irr::s32> >& areas, int current, irr::EKEY_CODE direction);
}
