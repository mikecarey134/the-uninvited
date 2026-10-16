#include "doctest.h"
#include "DebugCommands.h"
#include "LuaVM.h"
#include "World.h"
#include <memory>
#include <string>
#include <vector>

using namespace irr;

namespace
{
	struct Bench
	{
		Bench() : device(createDevice(video::EDT_NULL, core::dimension2d<u32>(1280, 720)))
		{
			world.reset(new World(device, "assets/models/stick_mike.ms3d", "assets/models/playerskin_m2.jpg", [] {}));
		}
		~Bench()
		{
			world.reset();
			device->drop();
		}

		std::function<World*()> live() { return [this] { return world.get(); }; }

		static array<WideString> words(std::initializer_list<const wchar_t*> list)
		{
			array<WideString> args;
			for (const wchar_t* word : list)
				args.push_back(WideString(word));
			return args;
		}

		IrrlichtDevice* device;
		std::unique_ptr<World> world;
	};

	bool refused(IC_Command& command, const array<WideString>& args)
	{
		try
		{
			command.invoke(args, nullptr, nullptr);
		}
		catch (IC_Error&)
		{
			return true;
		}
		return false;
	}
}

TEST_CASE("the console commands keep the names Mike's game gave them")
{
	Bench bench;
	LuaVM lua;
	std::vector<std::unique_ptr<IC_Command> > commands = debugCommands(bench.live(), [](const std::string&) {}, lua);
	std::vector<std::wstring> names;
	for (const std::unique_ptr<IC_Command>& command : commands)
		names.push_back(command->getName().c_str());
	CHECK(names == std::vector<std::wstring>{ L"tp", L"pspeed", L"ph", L"h", L"say", L"c", L"lua" });
}

TEST_CASE("tp warps the player to X Y Z and refuses anything but three numbers")
{
	Bench bench;
	TpCommand tp(bench.live());
	CHECK(tp.invoke(Bench::words({ L"4", L"0", L"-9.5" }), nullptr, nullptr));
	CHECK(bench.world->player()->getPosition().X == doctest::Approx(4.0f));
	CHECK(bench.world->player()->getPosition().Z == doctest::Approx(-9.5f));
	CHECK(refused(tp, Bench::words({ L"4", L"0" })));
	CHECK(refused(tp, Bench::words({ L"4", L"zero", L"1" })));
	CHECK(refused(tp, Bench::words({ L"1", L"2", L"3", L"4" })));
	CHECK(refused(tp, Bench::words({ L"4x", L"0", L"1" })));
	CHECK(bench.world->player()->getPosition().X == doctest::Approx(4.0f));
	TpCommand nowhere([] { return (World*)nullptr; });
	CHECK(refused(nowhere, Bench::words({ L"1", L"2", L"3" })));
}

TEST_CASE("pspeed sets the walking speed, ph sets health and h takes five")
{
	Bench bench;
	SpeedCommand speed(bench.live());
	CHECK(speed.invoke(Bench::words({ L"7.25" }), nullptr, nullptr));
	CHECK(bench.world->speed() == doctest::Approx(7.25f));
	CHECK(refused(speed, Bench::words({})));
	CHECK(refused(speed, Bench::words({ L"fast" })));

	HealthCommand health(bench.live());
	CHECK(health.invoke(Bench::words({ L"60" }), nullptr, nullptr));
	CHECK(bench.world->sheet().health() == 60);
	CHECK(refused(health, Bench::words({ L"60", L"1" })));

	HurtCommand hurt(bench.live());
	CHECK(hurt.invoke(Bench::words({}), nullptr, nullptr));
	CHECK(bench.world->sheet().health() == 55);
}

TEST_CASE("say and c send the whole line as one chat message, and want something to say")
{
	std::vector<std::string> sent;
	ChatCommand say(L"say", [&sent](const std::string& text) { sent.push_back(text); });
	ChatCommand c(L"c", [&sent](const std::string& text) { sent.push_back(text); });
	CHECK(say.invoke(Bench::words({ L"hello", L"there" }), nullptr, nullptr));
	CHECK(c.invoke(Bench::words({ L"over" }), nullptr, nullptr));
	CHECK(sent == std::vector<std::string>{ "hello there", "over" });
	CHECK(refused(say, Bench::words({})));
	CHECK(sent.size() == 2);
}

TEST_CASE("lua runs the rest of the line in the game's script state")
{
	LuaVM lua;
	std::vector<double> seen;
	lua.registerFunction("note", [&seen](const std::vector<LuaValue>& args) {
		seen.push_back(args.empty() ? -1.0 : args[0].asNumber());
		return std::vector<LuaValue>();
	});
	LuaCommand command(lua);
	CHECK(command.invoke(Bench::words({ L"note(6", L"*", L"7)" }), nullptr, nullptr));
	CHECK(seen == std::vector<double>{ 42.0 });
	CHECK(refused(command, Bench::words({})));
}
