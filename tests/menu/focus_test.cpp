#include "doctest.h"
#include "Focus.h"

using namespace irr;
using namespace core;

namespace
{
	const std::vector<rect<s32> > form = {
		rect<s32>(64, 632, 184, 672),
		rect<s32>(64, 100, 400, 140),
		rect<s32>(64, 180, 228, 212),
		rect<s32>(236, 180, 400, 212),
		rect<s32>(192, 632, 400, 672) };
}

TEST_CASE("Tab walks the elements as they are laid out, top to bottom then left to right, and wraps both ways")
{
	CHECK(focus::first(form) == 1);
	CHECK(focus::step(form, -1, false) == 1);
	CHECK(focus::step(form, 1, false) == 2);
	CHECK(focus::step(form, 2, false) == 3);
	CHECK(focus::step(form, 3, false) == 0);
	CHECK(focus::step(form, 0, false) == 4);
	CHECK(focus::step(form, 4, false) == 1);
	CHECK(focus::step(form, 1, true) == 4);
	CHECK(focus::step(form, 0, true) == 3);
	CHECK(focus::step(form, -1, true) == 4);
	CHECK(focus::first(std::vector<rect<s32> >()) == -1);
	CHECK(focus::step(std::vector<rect<s32> >(), -1, false) == -1);
}

TEST_CASE("an arrow moves to the nearest element that way, preferring one in line, and stays put at an edge")
{
	CHECK(focus::toward(form, 1, KEY_DOWN) == 2);
	CHECK(focus::toward(form, 2, KEY_RIGHT) == 3);
	CHECK(focus::toward(form, 3, KEY_LEFT) == 2);
	CHECK(focus::toward(form, 3, KEY_DOWN) == 4);
	CHECK(focus::toward(form, 2, KEY_DOWN) == 0);
	CHECK(focus::toward(form, 0, KEY_RIGHT) == 4);
	CHECK(focus::toward(form, 4, KEY_UP) == 3);
	CHECK(focus::toward(form, 1, KEY_UP) == 1);
	CHECK(focus::toward(form, 3, KEY_RIGHT) == 3);
	CHECK(focus::toward(form, -1, KEY_DOWN) == 1);
}
