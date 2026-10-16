#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H

#include <functional>
#include <map>
#include <string>
#include "CharacterSheet.h"
#include "client/NetClient.h"

class World;

NetClient::Events remoteEvents(std::function<World*()> world, std::function<void(const std::string&)> line,
	std::map<uint16_t, character::CharacterSheet>& sheets);

#endif
