#include "Multiplayer.h"
#include "CharacterProfile.h"
#include "Outfits.h"
#include "World.h"

namespace
{
	void place(World& world, const NetClient::Remote& remote)
	{
		if (remote.hasState)
			world.placeRemote(remote.id, remote.state.seq, remote.state.pos, remote.state.rot[1], remote.state.state == World::WALKING_STATE);
	}

	void spawn(World& world, const NetClient::Remote& remote)
	{
		character::Gender gender;
		int outfit;
		menu::unpackAppearance(remote.model, gender, outfit);
		world.addRemote(remote.id, menu::characterModel(gender), menu::outfitSkin(gender, outfit).c_str());
		place(world, remote);
	}
}

irr::video::SColor lineColour(NetClient::LineKind kind)
{
	switch (kind)
	{
	case NetClient::JoinedLine:
	case NetClient::InfoLine:
		return irr::video::SColor(255, 0, 255, 0);
	case NetClient::LeftLine:
	case NetClient::DiedLine:
	case NetClient::ErrorLine:
		return irr::video::SColor(255, 255, 0, 0);
	default:
		return irr::video::SColor(255, 255, 255, 255);
	}
}

NetClient::Events remoteEvents(std::function<World*()> world, std::function<void(const std::string&, NetClient::LineKind)> line,
	std::map<uint16_t, character::CharacterSheet>& sheets)
{
	NetClient::Events events;
	events.line = line;
	events.joined = [world, &sheets](const NetClient::Remote& remote) {
		character::Gender gender;
		int outfit;
		menu::unpackAppearance(remote.model, gender, outfit);
		character::CharacterSheet sheet = character::CharacterSheet::create(remote.name, gender, character::Profession());
		if (character::unpackProfile(remote.profile, remote.name, gender, outfit, sheet))
		{
			sheets.erase(remote.id);
			sheets.emplace(remote.id, sheet);
		}
		if (World* at = world())
			spawn(*at, remote);
	};
	events.moved = [world](const NetClient::Remote& remote) {
		if (World* at = world())
			place(*at, remote);
	};
	events.clock = [world](float timeOfDay, float timeScale) {
		if (World* at = world())
		{
			at->setTimeCompression(timeScale);
			at->setTimeOfDay(timeOfDay);
		}
	};
	events.left = [world, &sheets](uint16_t id) {
		sheets.erase(id);
		if (World* at = world())
			at->removeRemote(id);
	};
	return events;
}
