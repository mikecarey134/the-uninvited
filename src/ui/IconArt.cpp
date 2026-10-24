#include "IconArt.h"
#include <map>

using namespace irr;
using namespace core;

namespace
{
	// Measured from the PNGs (alpha above 40); tests/ui/icon_art_test.cpp re-measures them and fails if this drifts.
	const std::map<std::string, ArtBox> boxes = {
		{ "aegis/agility.png", { 0.77f, 0.50f, 0.53f } },
		{ "aegis/endurance.png", { 0.71f, 0.50f, 0.53f } },
		{ "aegis/guile.png", { 0.72f, 0.50f, 0.52f } },
		{ "aegis/intelligence.png", { 0.68f, 0.50f, 0.54f } },
		{ "aegis/strength.png", { 0.65f, 0.51f, 0.50f } },
		{ "approach/companion-delegate.png", { 0.45f, 0.50f, 0.50f } },
		{ "approach/force.png", { 0.40f, 0.50f, 0.50f } },
		{ "approach/normal.png", { 0.42f, 0.50f, 0.50f } },
		{ "approach/trade.png", { 0.37f, 0.50f, 0.50f } },
		{ "interactables/barricaded-door.png", { 0.88f, 0.50f, 0.50f } },
		{ "interactables/bear-trap.png", { 0.84f, 0.50f, 0.50f } },
		{ "interactables/cabinet.png", { 0.91f, 0.50f, 0.50f } },
		{ "interactables/generator.png", { 0.85f, 0.50f, 0.49f } },
		{ "interactables/ladder.png", { 0.91f, 0.50f, 0.50f } },
		{ "interactables/locked-door.png", { 0.88f, 0.50f, 0.50f } },
		{ "interactables/medical-cabinet.png", { 0.67f, 0.50f, 0.53f } },
		{ "interactables/safe.png", { 0.83f, 0.50f, 0.50f } },
		{ "interactables/supply-crate.png", { 0.88f, 0.50f, 0.56f } },
		{ "interactables/terminal.png", { 0.83f, 0.50f, 0.50f } },
		{ "interactables/unlocked-door.png", { 0.83f, 0.48f, 0.50f } },
		{ "interactables/vehicle.png", { 0.90f, 0.55f, 0.55f } },
		{ "interactables/weapon-rack.png", { 0.80f, 0.50f, 0.50f } },
		{ "interactables/window.png", { 0.83f, 0.41f, 0.50f } },
		{ "interactables/workbench.png", { 0.80f, 0.50f, 0.54f } },
		{ "modes/force.png", { 0.64f, 0.50f, 0.51f } },
		{ "modes/guile.png", { 0.56f, 0.50f, 0.53f } },
		{ "modes/normal.png", { 0.50f, 0.50f, 0.51f } },
		{ "modes/skill.png", { 0.69f, 0.51f, 0.52f } },
		{ "rings/cyan.png", { 0.62f, 0.50f, 0.50f } },
		{ "rings/gold.png", { 0.78f, 0.50f, 0.50f } },
		{ "rings/green.png", { 0.79f, 0.50f, 0.49f } },
		{ "rings/red.png", { 0.79f, 0.50f, 0.49f } },
		{ "skills/chemistry.png", { 0.65f, 0.50f, 0.52f } },
		{ "skills/construction.png", { 0.68f, 0.51f, 0.52f } },
		{ "skills/cooking.png", { 0.75f, 0.50f, 0.51f } },
		{ "skills/electronics.png", { 0.63f, 0.52f, 0.51f } },
		{ "skills/engineering.png", { 0.68f, 0.50f, 0.51f } },
		{ "skills/lockpicking.png", { 0.58f, 0.50f, 0.50f } },
		{ "skills/medicine.png", { 0.62f, 0.50f, 0.52f } },
		{ "skills/smithing.png", { 0.77f, 0.52f, 0.56f } },
		{ "skills/speech.png", { 0.70f, 0.52f, 0.53f } },
		{ "skills/survivalism.png", { 0.78f, 0.50f, 0.53f } },
	};
}

ArtBox artBox(const std::string& iconPath)
{
	std::map<std::string, ArtBox>::const_iterator found = boxes.find(iconPath);
	return found == boxes.end() ? ArtBox{ 1.0f, 0.5f, 0.5f } : found->second;
}

rect<s32> artRect(const std::string& iconPath, position2d<s32> centre, s32 artSize)
{
	ArtBox box = artBox(iconPath);
	f32 side = artSize / box.fill;
	f32 left = centre.X - box.centreX * side;
	f32 top = centre.Y - box.centreY * side;
	return rect<s32>((s32)(left + 0.5f), (s32)(top + 0.5f), (s32)(left + side + 0.5f), (s32)(top + side + 0.5f));
}
