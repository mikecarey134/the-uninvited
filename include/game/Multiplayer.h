#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H

#include <functional>
#include <map>
#include <string>
#include "CharacterSheet.h"
#include "client/NetClient.h"
#include <irrlicht.h>

class World;

irr::video::SColor lineColour(NetClient::LineKind kind);

NetClient::Events remoteEvents(std::function<World*()> world, std::function<void(const std::string&, NetClient::LineKind)> line,
	std::map<uint16_t, character::CharacterSheet>& sheets);

#endif
