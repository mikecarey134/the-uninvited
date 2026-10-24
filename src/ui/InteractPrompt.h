#ifndef INTERACT_PROMPT_H
#define INTERACT_PROMPT_H

#include "Canvas.h"
#include <string>

// The interact prompt under the target marker, in the keycap-then-verb form PC games use ("E Search"): a light rounded keycap
// with a darker lip and a dark key letter, then the action, all on a dark see-through pill. Draws nothing when action is empty.
// TODO: controller glyphs (Xbox X/A/B/Y, PlayStation square/cross/circle/triangle, Nintendo Y/B/A/X) and switching with the last
// input device (keyboard prompts after keyboard input, pad glyphs after pad input). Until then the key is the letter it is given.
void drawInteractPrompt(Canvas& canvas, irr::core::position2d<irr::s32> topCentre, const std::string& key, const std::string& action);

#endif
