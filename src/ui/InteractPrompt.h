#ifndef INTERACT_PROMPT_H
#define INTERACT_PROMPT_H

#include "Canvas.h"
#include <string>

// The interact prompt under the target marker, in the glyph-then-verb form PC games use ("E Search"): the button's glyph image
// (from ButtonGlyphs, for the device used last), then the action, on a dark see-through pill. Draws nothing when action is empty.
void drawInteractPrompt(Canvas& canvas, irr::core::position2d<irr::s32> topCentre, const std::string& glyph, const std::string& action);

#endif
