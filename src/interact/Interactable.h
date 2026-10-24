#ifndef INTERACTABLE_H
#define INTERACTABLE_H

#include <irrlicht.h>
#include <string>

struct InteractableInfo
{
	std::string name;     // "Locked Door", "Supply Crate", etc.
	std::string iconPath; // path relative to assets/ui/icons/
	float interactionRange = 2.5f; // how close the player needs to be, in world units
};

// Anything in the world the player can act on through the D-pad.
class Interactable
{
public:
	virtual ~Interactable() {}
	virtual InteractableInfo getInfo() const = 0;
	virtual irr::core::vector3df getPosition() const = 0;
};

#endif
