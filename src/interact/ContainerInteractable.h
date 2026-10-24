#ifndef CONTAINER_INTERACTABLE_H
#define CONTAINER_INTERACTABLE_H

#include "Interactable.h"
#include "Loot.h"
#include <functional>

// A loot container seen as a supply_crate: Normal opens the loot screen like "Press E to search", Force smashes it open.
// It points into the world's container list, which is filled once and never resized.
class ContainerInteractable : public Interactable
{
public:
	ContainerInteractable(loot::Container& container, std::function<void(loot::Container&)> open) : container_(container), open_(open) {}

	InteractableInfo getInfo() const override;
	irr::core::vector3df getPosition() const override { return irr::core::vector3df(container_.x, 0.4f, container_.z); }
	std::string getObjectType() const override { return "supply_crate"; }
	bool onNormal(Actor& player) override;
	bool onForce(Actor& player) override;

	loot::Container& container() const { return container_; }

private:
	loot::Container& container_;
	std::function<void(loot::Container&)> open_;
};

#endif
