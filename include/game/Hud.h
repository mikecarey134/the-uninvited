#ifndef HUD_H
#define HUD_H

#include "Canvas.h"
#include "CharacterSheet.h"

const irr::video::SColor hudHealth(255, 168, 40, 32);
const irr::video::SColor hudFood(255, 200, 142, 40);
const irr::video::SColor hudWater(255, 52, 112, 176);

void drawVitals(Canvas& canvas, const character::CharacterSheet& sheet);

#endif
