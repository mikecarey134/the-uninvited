#include "doctest.h"
#include "IconArt.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>

using namespace irr;
using namespace core;

TEST_CASE("every icon PNG has an art box, and each box matches a fresh measurement of its file")
{
	IrrlichtDevice* device = createDevice(video::EDT_NULL);
	REQUIRE(device != nullptr);
	video::IVideoDriver* driver = device->getVideoDriver();
	const std::string root = std::string(ASSETS_DIR) + "/ui/icons/";
	int measured = 0;
	for (const auto& folder : std::filesystem::directory_iterator(root))
		for (const auto& file : std::filesystem::directory_iterator(folder.path()))
		{
			if (file.path().extension() != ".png")
				continue;
			std::string relative = folder.path().filename().string() + "/" + file.path().filename().string();
			CAPTURE(relative);
			video::IImage* image = driver->createImageFromFile(file.path().string().c_str());
			REQUIRE(image != nullptr);
			s32 width = (s32)image->getDimension().Width;
			s32 height = (s32)image->getDimension().Height;
			s32 left = width, top = height, right = -1, bottom = -1;
			const s32 step = 3;
			for (s32 y = 0; y < height; y += step)
				for (s32 x = 0; x < width; x += step)
					if (image->getPixel((u32)x, (u32)y).getAlpha() > 40)
					{
						left = std::min(left, x);
						right = std::max(right, x);
						top = std::min(top, y);
						bottom = std::max(bottom, y);
					}
			image->drop();
			REQUIRE(right >= 0);
			f32 fill = std::max(right - left + 1, bottom - top + 1) / (f32)width;
			ArtBox box = artBox(relative);
			// Sampling every third pixel can miss the edge by up to two pixels a side.
			f32 tolerance = 0.015f + 2.0f * step / width;
			CHECK(box.fill < 1.0f);
			CHECK(std::fabs(box.fill - fill) < tolerance);
			CHECK(std::fabs(box.centreX - (left + right + 1) / 2.0f / width) < tolerance);
			CHECK(std::fabs(box.centreY - (top + bottom + 1) / 2.0f / height) < tolerance);
			++measured;
		}
	CHECK(measured >= 42);
	device->drop();
}

TEST_CASE("artRect sizes an icon by its visible art, so a heavily padded icon is drawn larger than a full one")
{
	position2d<s32> at(200, 200);
	rect<s32> padded = artRect("approach/force.png", at, 30);
	rect<s32> full = artRect("interactables/barricaded-door.png", at, 30);
	CHECK(padded.getWidth() * artBox("approach/force.png").fill == doctest::Approx(30.0f).epsilon(0.05));
	CHECK(full.getWidth() * artBox("interactables/barricaded-door.png").fill == doctest::Approx(30.0f).epsilon(0.05));
	CHECK(padded.getWidth() > full.getWidth() * 2);
	CHECK(std::abs(padded.getCenter().X - at.X) <= 1);
	CHECK(std::abs(padded.getCenter().Y - at.Y) <= 1);
	rect<s32> unknown = artRect("nowhere/none.png", at, 30);
	CHECK(unknown.getWidth() == 30);
}
