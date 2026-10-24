#include "doctest.h"
#include "InteractionDetector.h"
#include <cmath>

using namespace irr;
using namespace core;

namespace
{
	struct Thing : public Interactable
	{
		std::string name;
		vector3df at;
		f32 range;

		Thing(const std::string& name, f32 x, f32 z, f32 range = 2.5f) : name(name), at(x, 0.5f, z), range(range) {}
		InteractableInfo getInfo() const override
		{
			InteractableInfo info;
			info.name = name;
			info.interactionRange = range;
			return info;
		}
		vector3df getPosition() const override { return at; }
		std::string getObjectType() const override { return "locked_door"; }
	};

	// The camera sits 4.5 behind the player along the facing, as World::follow places it.
	void look(InteractionDetector& detector, f32 degrees, const vector3df& player = vector3df())
	{
		vector3df forward(std::sin(degrees * DEGTORAD), -0.2f, std::cos(degrees * DEGTORAD));
		detector.update(player, forward, player - vector3df(forward.X, 0.0f, forward.Z) * 4.5f + vector3df(0.0f, 2.6f, 0.0f));
	}
}

TEST_CASE("one interactable in range is targeted outright, wherever the crosshair points in the forward half")
{
	InteractionDetector detector;
	Thing door("Door", 1.5f, 1.0f);
	detector.addInteractable(&door);
	look(detector, -40.0f);
	CHECK(detector.getTarget() == &door);
	CHECK(detector.hasTarget());
	CHECK(detector.getInRange().size() == 1);
}

TEST_CASE("nothing out of range or behind the camera is targeted")
{
	InteractionDetector detector;
	Thing far("Far", 0.0f, 3.0f);
	Thing behind("Behind", 0.0f, -6.0f, 10.0f);
	detector.addInteractable(&far);
	detector.addInteractable(&behind);
	look(detector, 0.0f);
	CHECK_FALSE(detector.hasTarget());
	CHECK(detector.getInRange().empty());
	look(detector, 180.0f);
	CHECK(detector.getTarget() == &behind);
}

TEST_CASE("with several in range the one nearest the cone's centre wins, not the nearest one")
{
	InteractionDetector detector;
	Thing near("Near", -1.0f, 0.5f);
	Thing aimed("Aimed", 0.6f, 2.0f);
	detector.addInteractable(&near);
	detector.addInteractable(&aimed);
	f32 toward = std::atan2(0.6f, 2.0f + 4.5f) * RADTODEG;
	look(detector, toward + 4.0f);
	CHECK(detector.getInRange().size() == 2);
	CHECK(detector.getTarget() == &aimed);
}

TEST_CASE("with several in range and none in the cone, the nearest one is targeted")
{
	InteractionDetector detector;
	Thing near("Near", -1.0f, 0.5f);
	Thing other("Other", 1.5f, 1.5f);
	detector.addInteractable(&near);
	detector.addInteractable(&other);
	look(detector, 0.0f);
	CHECK(detector.getTarget() == &near);
}

TEST_CASE("the current target holds until the crosshair is clearly past the midpoint between two")
{
	InteractionDetector detector;
	Thing left("Left", -0.5f, 2.0f);
	Thing right("Right", 0.5f, 2.0f);
	detector.addInteractable(&left);
	detector.addInteractable(&right);
	f32 half = std::atan2(0.5f, 6.5f) * RADTODEG;
	look(detector, -half);
	REQUIRE(detector.getTarget() == &left);
	look(detector, 0.5f);
	CHECK(detector.getTarget() == &left);
	look(detector, 1.0f);
	CHECK(detector.getTarget() == &left);
	look(detector, half);
	CHECK(detector.getTarget() == &right);
	look(detector, -0.5f);
	CHECK(detector.getTarget() == &right);
}

TEST_CASE("removing the target clears it")
{
	InteractionDetector detector;
	Thing door("Door", 0.0f, 1.0f);
	detector.addInteractable(&door);
	detector.addInteractable(&door);
	look(detector, 0.0f);
	REQUIRE(detector.getTarget() == &door);
	detector.removeInteractable(&door);
	CHECK_FALSE(detector.hasTarget());
	CHECK(detector.getInRange().empty());
	look(detector, 0.0f);
	CHECK_FALSE(detector.hasTarget());
}
