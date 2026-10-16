#ifndef DEBUGCOMMANDS_H
#define DEBUGCOMMANDS_H

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "LuaVM.h"
#include "console.h"

class World;

typedef std::function<World*()> WorldSource;

class TpCommand : public IC_Command
{
public:
	explicit TpCommand(WorldSource world);
	bool invoke(const array<WideString>& args, IC_Dispatcher* dispatcher, IC_MessageSink* output) override;

private:
	WorldSource world_;
};

class SpeedCommand : public IC_Command
{
public:
	explicit SpeedCommand(WorldSource world);
	bool invoke(const array<WideString>& args, IC_Dispatcher* dispatcher, IC_MessageSink* output) override;

private:
	WorldSource world_;
};

class HealthCommand : public IC_Command
{
public:
	explicit HealthCommand(WorldSource world);
	bool invoke(const array<WideString>& args, IC_Dispatcher* dispatcher, IC_MessageSink* output) override;

private:
	WorldSource world_;
};

class HurtCommand : public IC_Command
{
public:
	explicit HurtCommand(WorldSource world);
	bool invoke(const array<WideString>& args, IC_Dispatcher* dispatcher, IC_MessageSink* output) override;

private:
	WorldSource world_;
};

class ChatCommand : public IC_Command
{
public:
	ChatCommand(const wchar_t* name, std::function<void(const std::string&)> send);
	bool invoke(const array<WideString>& args, IC_Dispatcher* dispatcher, IC_MessageSink* output) override;

private:
	std::function<void(const std::string&)> send_;
};

class LuaCommand : public IC_Command
{
public:
	explicit LuaCommand(LuaVM& lua);
	bool invoke(const array<WideString>& args, IC_Dispatcher* dispatcher, IC_MessageSink* output) override;

private:
	LuaVM& lua_;
};

std::vector<std::unique_ptr<IC_Command> > debugCommands(WorldSource world, std::function<void(const std::string&)> send, LuaVM& lua);

#endif
