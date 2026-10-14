#include "PlayerStatsCommand.h"

PlayerStatsCommand::PlayerStatsCommand(std::function<const character::CharacterSheet*()> chosen)
	: IC_Command(L"player_stats"), chosen_(chosen)
{
	setUsage(L"player_stats");
	addDescLine(L"Prints the character in play: attributes, profession, bloodline and skills");
}

bool PlayerStatsCommand::invoke(const array<WideString>&, IC_Dispatcher*, IC_MessageSink* output)
{
	const character::CharacterSheet* sheet = chosen_();
	if (!sheet)
	{
		output->appendMessage(L"No character is in play");
		return true;
	}
	for (const std::string& line : character::describe(*sheet))
		output->appendMessage(WideString(line.c_str()));
	return true;
}
