#pragma once
#include "CharacterSheet.h"
#include "console.h"
#include <functional>

class PlayerStatsCommand : public IC_Command
{
public:
	explicit PlayerStatsCommand(std::function<const character::CharacterSheet*()> chosen);
	bool invoke(const array<WideString>& args, IC_Dispatcher* dispatcher, IC_MessageSink* output) override;

private:
	std::function<const character::CharacterSheet*()> chosen_;
};
