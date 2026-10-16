#ifndef HUD_H
#define HUD_H

#include "Canvas.h"
#include "CharacterSheet.h"

const int HUD_FIST = 0;
const int HUD_BAT = 1;
const int HUD_PIPE = 2;
const int HUD_PISTOL = 3;
const int HUD_SHOTGUN = 4;
const int HUD_KNIFE = 5;

void drawVitals(Canvas& canvas, const character::CharacterSheet& sheet, int weapon = HUD_FIST);

#endif
