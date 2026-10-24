#ifndef ICON_ART_H
#define ICON_ART_H

#include <irrlicht.h>
#include <string>

// Where the visible art sits inside an icon's square PNG canvas. The masters carry very different padding (approach icons fill
// about 40% of their canvas, interactables nearly 90%), so drawing every PNG at one size gives specks next to giants.
struct ArtBox
{
	float fill;    // the art's larger side as a share of the canvas side
	float centreX; // the art's centre as a share of the canvas side
	float centreY;
};

// The art box of an icon under assets/ui/icons/ ("skills/lockpicking.png"); an unknown icon counts as filling its canvas.
ArtBox artBox(const std::string& iconPath);

// Where to draw an icon so its visible art is artSize across, centred on centre.
irr::core::rect<irr::s32> artRect(const std::string& iconPath, irr::core::position2d<irr::s32> centre, irr::s32 artSize);

#endif
