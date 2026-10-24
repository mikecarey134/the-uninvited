#include "TestInteractables.h"
#include "CharacterSheet.h"

const int TestDoor::STRENGTH_TO_KICK;
const int TestDoor::LOCKPICKING_TO_PICK;
const int TestDoor::ELECTRONICS_TO_HACK;
const int TestTerminal::ELECTRONICS_TO_BYPASS;

namespace
{
	bool say(Actor& player, bool done, const char* success, const char* failure)
	{
		player.message = done ? success : failure;
		return done;
	}
}

bool TestDoor::onNormal(Actor& player)
{
	return say(player, false, "", "The door is locked.");
}

bool TestDoor::onForce(Actor& player)
{
	return say(player, player.sheet.getStrength() >= STRENGTH_TO_KICK, "You kick the door open!", "Not strong enough.");
}

bool TestDoor::onSkill(Actor& player)
{
	if (player.skill == "electronics")
		return say(player, player.sheet.getElectronics() >= ELECTRONICS_TO_HACK, "You hack the keypad.", "Keypad is too advanced.");
	return say(player, player.sheet.getLockpicking() >= LOCKPICKING_TO_PICK, "You pick the lock.", "Lock is too complex.");
}

bool TestDoor::onGuile(Actor& player)
{
	return say(player, true, "Your companion moves to the door.", "");
}

bool TestTerminal::onNormal(Actor& player)
{
	return say(player, true, "You access the terminal.", "");
}

bool TestTerminal::onSkill(Actor& player)
{
	return say(player, player.sheet.getElectronics() >= ELECTRONICS_TO_BYPASS, "You bypass the security.", "Too advanced.");
}

bool TestCrate::onNormal(Actor& player)
{
	return say(player, true, "The test crate is empty.", "");
}

bool TestCrate::onForce(Actor& player)
{
	return say(player, true, "You smash the test crate. Nothing inside.", "");
}
