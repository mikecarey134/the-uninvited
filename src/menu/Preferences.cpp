#include "Preferences.h"
#include "KeyValueFile.h"
#include <cstdio>

namespace menu
{
	namespace
	{
		const char* const timeNames[3] = { "real_time", "half_speed", "fast" };

		void volume(KeyValueReader& fields, const std::string& key, int& value, std::vector<std::string>& problems)
		{
			std::string raw;
			int read = 0;
			if (!fields.text(key, raw))
				return;
			if (fields.whole(key, read) && read >= 0 && read <= 100)
				value = read;
			else
				problems.push_back("bad " + key + ": " + raw);
		}
	}

	std::string Preferences::toText() const
	{
		KeyValueWriter out;
		out.add("master_volume", masterVolume);
		out.add("music_volume", musicVolume);
		out.add("sfx_volume", sfxVolume);
		out.add("fullscreen", std::string(fullscreen ? "true" : "false"));
		out.add("resolution", std::to_string(width) + "x" + std::to_string(height));
		out.add("time", std::string(timeNames[(int)time]));
		return out.text();
	}

	Preferences Preferences::fromText(const std::string& text, std::vector<std::string>& problems)
	{
		Preferences preferences;
		KeyValueReader fields(text);
		volume(fields, "master_volume", preferences.masterVolume, problems);
		volume(fields, "music_volume", preferences.musicVolume, problems);
		volume(fields, "sfx_volume", preferences.sfxVolume, problems);
		std::string raw;
		if (fields.text("fullscreen", raw))
		{
			if (raw == "true" || raw == "false")
				preferences.fullscreen = raw == "true";
			else
				problems.push_back("bad fullscreen: " + raw);
		}
		if (fields.text("resolution", raw))
		{
			int width = 0;
			int height = 0;
			char end = 0;
			if (std::sscanf(raw.c_str(), "%dx%d%c", &width, &height, &end) == 2 && width >= SMALLEST_WIDTH && height >= SMALLEST_HEIGHT)
			{
				preferences.width = width;
				preferences.height = height;
			}
			else
				problems.push_back("bad resolution: " + raw);
		}
		if (fields.text("time", raw))
		{
			bool known = false;
			for (int i = 0; i < 3; ++i)
				if (raw == timeNames[i])
				{
					preferences.time = (TimeMode)i;
					known = true;
				}
			if (!known)
				problems.push_back("bad time: " + raw);
		}
		return preferences;
	}
}
