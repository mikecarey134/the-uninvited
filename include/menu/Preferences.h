#pragma once
#include <string>
#include <vector>

namespace menu
{
	enum class TimeMode { RealTime, HalfSpeed, Fast };

	struct Preferences
	{
		int masterVolume = 100;
		int musicVolume = 100;
		int sfxVolume = 100;
		bool fullscreen = false;
		static const int SMALLEST_WIDTH = 1280;
		static const int SMALLEST_HEIGHT = 720;

		int width = 1280;
		int height = 720;
		TimeMode time = TimeMode::RealTime;

		std::string toText() const;
		static Preferences fromText(const std::string& text, std::vector<std::string>& problems);
	};
}
