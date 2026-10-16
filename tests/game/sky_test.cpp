#include "doctest.h"
#include "Sky.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace irr;
using namespace video;

namespace
{
	bool bluish(const SColor& colour) { return colour.getBlue() > colour.getRed() && colour.getBlue() > colour.getGreen(); }
	bool warm(const SColor& colour) { return colour.getRed() > colour.getBlue() && colour.getRed() > colour.getGreen(); }
	f32 brightness(const SColor& colour) { return (colour.getRed() + colour.getGreen() + colour.getBlue()) / 3.0f; }
	f32 level(const SColorf& colour) { return (colour.r + colour.g + colour.b) / 3.0f; }
	int step(const SColor& a, const SColor& b)
	{
		return std::max(std::abs((int)a.getRed() - (int)b.getRed()), std::max(std::abs((int)a.getGreen() - (int)b.getGreen()),
			std::abs((int)a.getBlue() - (int)b.getBlue())));
	}
}

TEST_CASE("night is a dark blue sky under moonlight with the sun off")
{
	for (f32 hour : { 19.5f, 23.0f, 0.0f, 3.0f, 4.9f })
	{
		INFO(hour);
		SkyLook night = skyAt(hour);
		CHECK(bluish(night.zenith));
		CHECK(brightness(night.zenith) < 30.0f);
		CHECK(brightness(night.horizon) < 40.0f);
		CHECK(level(night.ambient) > 0.05f);
		CHECK(level(night.ambient) < 0.2f);
		CHECK(level(night.sun) == 0.0f);
	}
}

TEST_CASE("the day is a light blue sky with full ambient light and a white, slightly warm sun")
{
	for (f32 hour : { 7.5f, 10.0f, 12.0f, 15.0f, 16.9f })
	{
		INFO(hour);
		SkyLook day = skyAt(hour);
		CHECK(bluish(day.zenith));
		CHECK(bluish(day.horizon));
		CHECK(brightness(day.horizon) > 150.0f);
		CHECK(level(day.ambient) >= 0.45f);
		CHECK(day.sun.r >= day.sun.b);
		CHECK(day.sun.b > 0.7f);
	}
}

TEST_CASE("dawn and dusk turn the horizon warm with a warm, low sun, dusk leaning purple overhead")
{
	SkyLook dawn = skyAt(6.0f);
	CHECK(warm(dawn.horizon));
	CHECK(brightness(dawn.horizon) > 120.0f);
	CHECK(dawn.sun.r > dawn.sun.b);
	CHECK(level(dawn.ambient) > level(skyAt(3.0f).ambient));
	CHECK(level(dawn.ambient) < level(skyAt(12.0f).ambient));

	SkyLook dusk = skyAt(18.0f);
	CHECK(warm(dusk.horizon));
	CHECK(dusk.zenith.getRed() > dusk.zenith.getGreen());
	CHECK(dusk.zenith.getBlue() > dusk.zenith.getGreen());
	CHECK(dusk.sun.r > dusk.sun.b);
	CHECK(level(dusk.ambient) < level(skyAt(12.0f).ambient));
}

TEST_CASE("ambient light rises through dawn and falls through dusk, never jumping")
{
	for (f32 hour = 5.0f; hour < 7.0f; hour += 0.25f)
		CHECK(level(skyAt(hour + 0.25f).ambient) >= level(skyAt(hour).ambient));
	for (f32 hour = 17.0f; hour < 19.0f; hour += 0.25f)
		CHECK(level(skyAt(hour + 0.25f).ambient) <= level(skyAt(hour).ambient));
}

TEST_CASE("the sky changes smoothly, a few shades a game-minute at most, all the way round the clock and across midnight")
{
	int biggest = 0;
	for (int minute = 0; minute < 24 * 60; ++minute)
	{
		SkyLook now = skyAt(minute / 60.0f), next = skyAt((minute + 1) / 60.0f);
		biggest = std::max(biggest, std::max(step(now.zenith, next.zenith), step(now.horizon, next.horizon)));
		CHECK(std::fabs(level(now.ambient) - level(next.ambient)) < 0.01f);
		CHECK(std::fabs(level(now.sun) - level(next.sun)) < 0.02f);
	}
	CHECK(biggest <= 4);
	CHECK(biggest >= 1);
	CHECK(step(skyAt(24.0f).zenith, skyAt(0.0f).zenith) == 0);
	CHECK(step(skyAt(-1.0f).horizon, skyAt(23.0f).horizon) == 0);
	CHECK(step(skyAt(30.0f).horizon, skyAt(6.0f).horizon) == 0);
	CHECK(step(skyAt(-18.0f).horizon, skyAt(6.0f).horizon) == 0);
	CHECK(step(skyAt(-17.5f).zenith, skyAt(6.5f).zenith) == 0);
}

TEST_CASE("the sun shines from the east at dawn, straight down at noon, from the west at dusk, and from below the ground at night")
{
	CHECK(skyAt(6.0f).sunDirection.X == doctest::Approx(-1.0f));
	CHECK(skyAt(6.0f).sunDirection.Y == doctest::Approx(0.0f));
	CHECK(skyAt(9.0f).sunDirection.X < 0.0f);
	CHECK(skyAt(9.0f).sunDirection.Y < 0.0f);
	CHECK(skyAt(12.0f).sunDirection.Y == doctest::Approx(-1.0f));
	CHECK(skyAt(15.0f).sunDirection.X > 0.0f);
	CHECK(skyAt(18.0f).sunDirection.X == doctest::Approx(1.0f));
	CHECK(skyAt(0.0f).sunDirection.Y > 0.9f);
	CHECK(skyAt(21.0f).sunDirection.Y > 0.0f);
	for (f32 hour = 0.0f; hour < 24.0f; hour += 0.5f)
		CHECK(skyAt(hour).sunDirection.getLength() == doctest::Approx(1.0f));
}
