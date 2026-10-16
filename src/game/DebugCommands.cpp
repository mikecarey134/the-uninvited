#include "DebugCommands.h"
#include "World.h"
#include <cwchar>

namespace
{
	World& liveWorld(const WorldSource& world)
	{
		World* found = world();
		if (!found)
			throw IC_Error(L"No world is loaded");
		return *found;
	}

	float number(const WideString& text, const wchar_t* complaint)
	{
		const wchar_t* start = text.c_str();
		wchar_t* end = nullptr;
		float value = std::wcstof(start, &end);
		if (end == start || *end != L'\0')
			throw IC_Error(complaint);
		return value;
	}

	std::string joined(const array<WideString>& args)
	{
		std::string text;
		for (u32 i = 0; i < args.size(); ++i)
		{
			if (i > 0)
				text += ' ';
			for (u32 j = 0; j < args[i].size(); ++j)
				text += (char)args[i][j];
		}
		return text;
	}
}

TpCommand::TpCommand(WorldSource world) : IC_Command(L"tp"), world_(world)
{
	setUsage(L"tp <X>,<Y>,<Z>");
	addDescLine("warps player <X><Y><Z>");
}

bool TpCommand::invoke(const array<WideString>& args, IC_Dispatcher*, IC_MessageSink*)
{
	if (args.size() != 3)
		throw IC_Error(L"No Valid Arguments! <X><Y><Z>");
	World& world = liveWorld(world_);
	const wchar_t* complaint = L"No Valid Arguments! <X><Y><Z>";
	float x = number(args[0], complaint), y = number(args[1], complaint), z = number(args[2], complaint);
	world.teleport(x, y, z);
	return true;
}

SpeedCommand::SpeedCommand(WorldSource world) : IC_Command(L"pspeed"), world_(world)
{
	setUsage(L"\\pspeed <New Speed>");
	addDescLine("Increases or decreases player speed");
}

bool SpeedCommand::invoke(const array<WideString>& args, IC_Dispatcher*, IC_MessageSink*)
{
	if (args.size() != 1)
		throw IC_Error(L"No Valid Argument! <New Speed>");
	liveWorld(world_).setSpeed(number(args[0], L"No Valid Argument! <New Speed>"));
	return true;
}

HealthCommand::HealthCommand(WorldSource world) : IC_Command(L"ph"), world_(world)
{
	setUsage(L"\\ph <New Health>");
	addDescLine("Increases or decreases player health");
}

bool HealthCommand::invoke(const array<WideString>& args, IC_Dispatcher*, IC_MessageSink*)
{
	if (args.size() != 1)
		throw IC_Error(L"No Valid Argument! <New Health>");
	liveWorld(world_).setHealth((int)number(args[0], L"No Valid Argument! <New Health>"));
	return true;
}

HurtCommand::HurtCommand(WorldSource world) : IC_Command(L"h"), world_(world)
{
	setUsage(L"\\h ");
	addDescLine("Hurt Current Player");
}

bool HurtCommand::invoke(const array<WideString>&, IC_Dispatcher*, IC_MessageSink*)
{
	liveWorld(world_).hurt();
	return true;
}

ChatCommand::ChatCommand(const wchar_t* name, std::function<void(const std::string&)> send) : IC_Command(name), send_(send)
{
	setUsage(WideString((std::wstring(L"/") + name + L" <Message>").c_str()));
	addDescLine("Add Chat Message");
}

bool ChatCommand::invoke(const array<WideString>& args, IC_Dispatcher*, IC_MessageSink*)
{
	if (args.size() == 0)
		throw IC_Error(L"Missing Argument! <Message>");
	send_(joined(args));
	return true;
}

LuaCommand::LuaCommand(LuaVM& lua) : IC_Command(L"lua"), lua_(lua)
{
	setUsage(L"\\lua <code>");
	addDescLine("Runs a line of Lua in the game's script state");
}

bool LuaCommand::invoke(const array<WideString>& args, IC_Dispatcher*, IC_MessageSink*)
{
	if (args.size() == 0)
		throw IC_Error(L"Missing Argument! <code>");
	lua_.doString(joined(args));
	return true;
}

std::vector<std::unique_ptr<IC_Command> > debugCommands(WorldSource world, std::function<void(const std::string&)> send, LuaVM& lua)
{
	std::vector<std::unique_ptr<IC_Command> > commands;
	commands.emplace_back(new TpCommand(world));
	commands.emplace_back(new SpeedCommand(world));
	commands.emplace_back(new HealthCommand(world));
	commands.emplace_back(new HurtCommand(world));
	commands.emplace_back(new ChatCommand(L"say", send));
	commands.emplace_back(new ChatCommand(L"c", send));
	commands.emplace_back(new LuaCommand(lua));
	return commands;
}
