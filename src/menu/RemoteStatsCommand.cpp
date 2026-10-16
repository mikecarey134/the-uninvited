#include "RemoteStatsCommand.h"

RemoteStatsCommand::RemoteStatsCommand(const std::map<uint16_t, character::CharacterSheet>& remotes)
	: IC_Command(L"remote_stats"), remotes_(remotes)
{
	setUsage(L"remote_stats");
	addDescLine(L"Prints the characters of the other connected players, as they arrived over the network");
}

bool RemoteStatsCommand::invoke(const array<WideString>&, IC_Dispatcher*, IC_MessageSink* output)
{
	if (remotes_.empty())
		output->appendMessage(L"No other players are connected");
	for (const auto& remote : remotes_)
	{
		std::string header = "Player " + std::to_string(remote.first) + ", age " + std::to_string((int)remote.second.age());
		output->appendMessage(WideString(header.c_str()));
		for (const std::string& line : character::describe(remote.second))
			output->appendMessage(WideString(line.c_str()));
	}
	return true;
}
