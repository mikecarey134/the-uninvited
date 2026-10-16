#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H

#include <functional>
#include <string>
#include "client/NetClient.h"

class World;

NetClient::Events remoteEvents(std::function<World*()> world, std::function<void(const std::string&)> line);

#endif
