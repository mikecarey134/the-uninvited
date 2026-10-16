#pragma once
#include "CharacterSheet.h"
#include "console.h"
#include <cstdint>
#include <map>

class RemoteStatsCommand : public IC_Command
{
public:
	explicit RemoteStatsCommand(const std::map<uint16_t, character::CharacterSheet>& remotes);
	bool invoke(const array<WideString>& args, IC_Dispatcher* dispatcher, IC_MessageSink* output) override;

private:
	const std::map<uint16_t, character::CharacterSheet>& remotes_;
};
