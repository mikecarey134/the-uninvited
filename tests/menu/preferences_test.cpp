#include "doctest.h"
#include "Preferences.h"

using namespace menu;

TEST_CASE("with no settings file every volume is full, the game is windowed at 1280 by 720, and new worlds run in real time")
{
	std::vector<std::string> problems;
	Preferences preferences = Preferences::fromText("", problems);
	CHECK(preferences.masterVolume == 100);
	CHECK(preferences.musicVolume == 100);
	CHECK(preferences.sfxVolume == 100);
	CHECK_FALSE(preferences.fullscreen);
	CHECK(preferences.width == 1280);
	CHECK(preferences.height == 720);
	CHECK(preferences.time == TimeMode::RealTime);
	CHECK(problems.empty());
}

TEST_CASE("settings are written as one key = value line each and read back exactly")
{
	Preferences preferences;
	preferences.masterVolume = 35;
	preferences.musicVolume = 0;
	preferences.sfxVolume = 80;
	preferences.fullscreen = true;
	preferences.width = 1920;
	preferences.height = 1080;
	preferences.time = TimeMode::Fast;
	CHECK(preferences.toText() == "master_volume = 35\nmusic_volume = 0\nsfx_volume = 80\nfullscreen = true\nresolution = 1920x1080\ntime = fast\n");
	std::vector<std::string> problems;
	Preferences back = Preferences::fromText(preferences.toText(), problems);
	CHECK(problems.empty());
	CHECK(back.masterVolume == 35);
	CHECK(back.musicVolume == 0);
	CHECK(back.sfxVolume == 80);
	CHECK(back.fullscreen);
	CHECK(back.width == 1920);
	CHECK(back.height == 1080);
	CHECK(back.time == TimeMode::Fast);
	preferences.time = TimeMode::HalfSpeed;
	CHECK(Preferences::fromText(preferences.toText(), problems).time == TimeMode::HalfSpeed);
}

TEST_CASE("a bad value keeps its default and is reported, while the good lines around it still load")
{
	std::vector<std::string> problems;
	Preferences preferences = Preferences::fromText(
		"master_volume = 150\nmusic_volume = loud\nsfx_volume = 40\nfullscreen = maybe\nresolution = 0x720\ntime = slow\n", problems);
	CHECK(preferences.masterVolume == 100);
	CHECK(preferences.musicVolume == 100);
	CHECK(preferences.sfxVolume == 40);
	CHECK_FALSE(preferences.fullscreen);
	CHECK(preferences.width == 1280);
	CHECK(preferences.height == 720);
	CHECK(preferences.time == TimeMode::RealTime);
	CHECK(problems.size() == 5);
	problems.clear();
	preferences = Preferences::fromText("master_volume = -1\nresolution = 800 by 600\n", problems);
	CHECK(preferences.masterVolume == 100);
	CHECK(preferences.width == 1280);
	CHECK(problems.size() == 2);
	problems.clear();
	preferences = Preferences::fromText("resolution = 1024x768\n", problems);
	CHECK(preferences.width == 1280);
	CHECK(problems.size() == 1);
	problems.clear();
	preferences = Preferences::fromText("resolution = 1920x600\n", problems);
	CHECK(preferences.width == 1280);
	CHECK(preferences.height == 720);
	CHECK(problems.size() == 1);
	problems.clear();
	preferences = Preferences::fromText("resolution = 1920x1080p\n", problems);
	CHECK(preferences.width == 1280);
	CHECK(problems.size() == 1);
	problems.clear();
	preferences = Preferences::fromText("master_volume = 0\nresolution = 1280x1024\n", problems);
	CHECK(preferences.masterVolume == 0);
	CHECK(preferences.width == 1280);
	CHECK(preferences.height == 1024);
	CHECK(problems.empty());
}
