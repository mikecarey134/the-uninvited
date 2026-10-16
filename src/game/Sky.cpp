#include "Sky.h"
#include <cmath>

using namespace irr;
using namespace core;
using namespace video;

namespace
{
	struct Key
	{
		f32 hour;
		SColor zenith;
		SColor horizon;
		SColorf ambient;
		SColorf sun;
	};

	const Key night{ 0.0f, SColor(255, 6, 8, 20), SColor(255, 16, 20, 38), SColorf(0.10f, 0.11f, 0.16f), SColorf(0.0f, 0.0f, 0.0f) };
	const Key dawn{ 6.0f, SColor(255, 70, 80, 130), SColor(255, 235, 140, 90), SColorf(0.30f, 0.25f, 0.25f), SColorf(0.90f, 0.55f, 0.35f) };
	const Key morning{ 7.0f, SColor(255, 95, 150, 215), SColor(255, 175, 205, 235), SColorf(0.45f, 0.45f, 0.45f), SColorf(1.0f, 0.90f, 0.75f) };
	const Key noon{ 12.0f, SColor(255, 80, 140, 215), SColor(255, 170, 200, 235), SColorf(0.50f, 0.50f, 0.50f), SColorf(1.0f, 0.97f, 0.90f) };
	const Key dusk{ 18.0f, SColor(255, 80, 60, 120), SColor(255, 230, 110, 60), SColorf(0.30f, 0.22f, 0.22f), SColorf(0.90f, 0.45f, 0.25f) };

	Key at(const Key& look, f32 hour)
	{
		Key moved = look;
		moved.hour = hour;
		return moved;
	}

	const Key keys[] = { at(night, 0.0f), at(night, 5.0f), dawn, morning, noon, at(morning, 17.0f), dusk, at(night, 19.0f), at(night, 24.0f) };
}

SkyLook skyAt(f32 hours)
{
	f32 hour = std::fmod(hours, 24.0f);
	if (hour < 0.0f)
		hour += 24.0f;
	size_t next = 1;
	while (keys[next].hour < hour)
		++next;
	const Key& from = keys[next - 1];
	const Key& to = keys[next];
	f32 t = (hour - from.hour) / (to.hour - from.hour);
	SkyLook look;
	look.zenith = to.zenith.getInterpolated(from.zenith, t);
	look.horizon = to.horizon.getInterpolated(from.horizon, t);
	look.ambient = to.ambient.getInterpolated(from.ambient, t);
	look.sun = to.sun.getInterpolated(from.sun, t);
	f32 elevation = (hour - 6.0f) / 12.0f * PI;
	look.sunDirection = vector3df(-std::cos(elevation), -std::sin(elevation), 0.0f);
	return look;
}
