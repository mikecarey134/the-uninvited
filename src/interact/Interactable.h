#ifndef INTERACTABLE_H
#define INTERACTABLE_H

#include <irrlicht.h>
#include <string>

namespace character
{
	class CharacterSheet;
}

struct InteractableInfo
{
	std::string name;     // "Locked Door", "Supply Crate", etc.
	std::string iconPath; // path relative to assets/ui/icons/
	float interactionRange = 2.5f; // how close the player needs to be, in world units
	// approach availability comes from the ApproachRegistry, keyed by the object type
};

// Who is acting on an interactable: the player's stats, and the line the handler wants them to see.
struct Actor
{
	const character::CharacterSheet& sheet;
	std::string message;
	std::string skill;         // for onSkill: which skill the player chose ("lockpicking", "electronics"...)
	bool openedScreen = false; // the handler opened a screen (loot, dialogue...), which is feedback enough
};

// Anything in the world the player can act on through the D-pad.
class Interactable
{
public:
	virtual ~Interactable() {}
	virtual InteractableInfo getInfo() const = 0;
	virtual irr::core::vector3df getPosition() const = 0;
	virtual std::string getObjectType() const = 0; // the ApproachRegistry key: "locked_door", "supply_crate", ...

	// Approach handlers: true when the action succeeds, false when it fails. A handler may set player.message.
	virtual bool onNormal(Actor& player) { (void)player; return false; }
	virtual bool onForce(Actor& player) { (void)player; return false; }
	virtual bool onSkill(Actor& player) { (void)player; return false; }
	virtual bool onGuile(Actor& player) { (void)player; return false; }
};

#endif
