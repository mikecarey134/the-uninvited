#ifndef BUTTON_GLYPHS_H
#define BUTTON_GLYPHS_H

#include "InputMode.h"

// What a prompt asks the player to press, independent of the device.
enum class GlyphAction
{
	INTERACT,      // E / pad X (Square): runs the target's Normal, or the picked D-pad slot
	CANCEL,        // Escape / B (Circle)
	CONTEXT_MENU,  // Q (Tab and a right-click also work); the pad has none, it uses the D-pad
	DPAD_UP,       // keyboard: the number key that runs the same slot (2 Skill)
	DPAD_RIGHT,    // 4 Guile
	DPAD_DOWN,     // 3 second Skill
	DPAD_LEFT,     // 1 Force
	SHOULDER_LEFT,
	SHOULDER_RIGHT,
	TRIGGER_LEFT,
	TRIGGER_RIGHT,
	POINTER_PRIMARY,   // left mouse button
	POINTER_SECONDARY, // right mouse button
};

enum class GlyphFamily { KEYBOARD_MOUSE, XBOX, PLAYSTATION };

// Which glyphs to show: the keyboard's while the keyboard and mouse were used last, else the connected pad's family.
GlyphFamily glyphFamily(InputMode mode, PadFamily pad);
// The glyph image for an action in a family (under assets/textures/glyphs/), or "" where that device has no such button.
// Drawn through Canvas::loadTexture, which caches by path, so switching device just swaps which path is drawn.
const char* glyphPath(GlyphAction action, GlyphFamily family);

#endif
