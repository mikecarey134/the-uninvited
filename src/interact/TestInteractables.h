#ifndef TEST_INTERACTABLES_H
#define TEST_INTERACTABLES_H

#include "Interactable.h"

// TEMPORARY (SCRUM-1001/1005): stand-ins for real world objects, with placeholder handlers that prove stat checks work.
struct TestObject : public Interactable
{
	InteractableInfo info;
	irr::core::vector3df position;
	std::string type;

	InteractableInfo getInfo() const override { return info; }
	irr::core::vector3df getPosition() const override { return position; }
	std::string getObjectType() const override { return type; }
};

struct TestDoor : public TestObject
{
	static const int STRENGTH_TO_KICK = 25;
	static const int LOCKPICKING_TO_PICK = 20;
	static const int ELECTRONICS_TO_HACK = 20;

	bool onNormal(Actor& player) override;
	bool onForce(Actor& player) override;
	bool onSkill(Actor& player) override;
	bool onGuile(Actor& player) override;
};

struct TestTerminal : public TestObject
{
	static const int ELECTRONICS_TO_BYPASS = 20;

	bool onNormal(Actor& player) override;
	bool onSkill(Actor& player) override;
};

struct TestCrate : public TestObject
{
	bool onNormal(Actor& player) override;
	bool onForce(Actor& player) override;
};

#endif
