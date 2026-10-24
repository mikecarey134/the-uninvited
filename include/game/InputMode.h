#ifndef INPUT_MODE_H
#define INPUT_MODE_H

#include <irrlicht.h>

// The pad never reaches the game as keyboard scancodes: Irrlicht hands it over as joystick events. It is our own GamepadLayer
// that turns pad buttons into key presses (A Enter, B Escape, X R, Start I, Back C; D-pad and left stick the arrows) so the
// menus work with a pad. InputSource tells those made-up presses from real keys, and remembers which device was used last.
enum class InputMode { KEYBOARD_MOUSE, GAMEPAD };
enum class PadPart { NONE, BUTTON, DPAD, STICK };
// Which button layout the pad has. Nintendo pads share the Xbox positions, so they count as Xbox for now.
enum class PadFamily { XBOX, PLAYSTATION };
// From the name the pad reports: Sony, DualShock, DualSense or PS mean PlayStation; anything else, Xbox.
PadFamily padFamilyFromName(const char* name);

class InputSource
{
public:
	// Every event, before it is routed: a real key or mouse press means keyboard and mouse; pad activity, or a press the pad
	// layer made up, means gamepad. Mouse movement is ignored, as the captured cursor is re-centred every frame.
	void observe(const irr::SEvent& event);
	InputMode mode() const { return mode_; }

	// Brackets the key presses the pad layer posts, naming the part of the pad that made them.
	void beginPad(PadPart part) { pad_ = part; }
	void endPad() { pad_ = PadPart::NONE; }
	// Which part of the pad made the key press being handled, or NONE for a real key.
	PadPart pad() const { return pad_; }
	PadFamily padFamily() const { return padFamily_; }
	void setPadFamily(PadFamily family) { padFamily_ = family; }

private:
	InputMode mode_ = InputMode::KEYBOARD_MOUSE;
	PadPart pad_ = PadPart::NONE;
	PadFamily padFamily_ = PadFamily::XBOX;
	irr::s16 axes_[irr::SEvent::SJoystickEvent::NUMBER_OF_AXES] = {};
	bool seen_ = false;
};

#endif
