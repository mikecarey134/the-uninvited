#include "ContainerInteractable.h"

InteractableInfo ContainerInteractable::getInfo() const
{
	InteractableInfo info;
	info.name = container_.kind;
	info.iconPath = "interactables/supply-crate.png";
	info.interactionRange = loot::REACH;
	return info;
}

bool ContainerInteractable::onNormal(Actor& player)
{
	if (!open_)
		return false;
	open_(container_);
	player.message = "You open the " + container_.kind + ".";
	return true;
}

bool ContainerInteractable::onForce(Actor& player)
{
	if (!open_)
		return false;
	open_(container_);
	player.message = "You smash the " + container_.kind + " open.";
	return true;
}
