#ifndef THEME_H
#define THEME_H

#include <SColor.h>

namespace theme
{
	const irr::video::SColor backgroundTop(255, 0x0c, 0x0a, 0x06);
	const irr::video::SColor backgroundBottom(255, 0x14, 0x12, 0x10);
	const irr::video::SColor panel(242, 0x0e, 0x0c, 0x08);
	const irr::video::SColor border(26, 180, 140, 80);
	const irr::video::SColor borderHover(70, 180, 140, 80);
	const irr::video::SColor accent(255, 0xb8, 0x4c, 0x1c);
	const irr::video::SColor accentFill(38, 184, 76, 28);
	const irr::video::SColor accentFillHover(60, 184, 76, 28);
	const irr::video::SColor accentBorder(70, 184, 76, 28);
	const irr::video::SColor accentBorderHover(130, 184, 76, 28);
	const irr::video::SColor text(255, 0xe0, 0xd8, 0xc8);
	const irr::video::SColor muted(255, 0x5a, 0x50, 0x40);
	const irr::video::SColor dim(255, 0x4a, 0x44, 0x38);
	const irr::video::SColor input(230, 26, 24, 18);
	const irr::video::SColor scrollThumb(255, 40, 35, 26);
}

#endif
