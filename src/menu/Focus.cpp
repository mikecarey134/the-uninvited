#include "Focus.h"
#include <algorithm>
#include <cstdlib>

using namespace irr;
using namespace core;

namespace focus
{
	namespace
	{
		std::vector<int> ordered(const std::vector<rect<s32> >& areas)
		{
			std::vector<int> order;
			for (int i = 0; i < (int)areas.size(); ++i)
				order.push_back(i);
			std::stable_sort(order.begin(), order.end(), [&areas](int a, int b) {
				if (areas[a].UpperLeftCorner.Y != areas[b].UpperLeftCorner.Y)
					return areas[a].UpperLeftCorner.Y < areas[b].UpperLeftCorner.Y;
				return areas[a].UpperLeftCorner.X < areas[b].UpperLeftCorner.X;
			});
			return order;
		}
	}

	int first(const std::vector<rect<s32> >& areas)
	{
		return areas.empty() ? -1 : ordered(areas).front();
	}

	int step(const std::vector<rect<s32> >& areas, int current, bool backward)
	{
		std::vector<int> order = ordered(areas);
		if (order.empty())
			return -1;
		std::vector<int>::iterator at = std::find(order.begin(), order.end(), current);
		if (at == order.end())
			return backward ? order.back() : order.front();
		int index = (int)(at - order.begin()) + (backward ? -1 : 1);
		int count = (int)order.size();
		return order[(index + count) % count];
	}

	int toward(const std::vector<rect<s32> >& areas, int current, EKEY_CODE direction)
	{
		if (current < 0 || current >= (int)areas.size())
			return first(areas);
		const rect<s32>& from = areas[current];
		bool across = direction == KEY_LEFT || direction == KEY_RIGHT;
		int best = current;
		long long bestScore = 0;
		for (int i = 0; i < (int)areas.size(); ++i)
		{
			const rect<s32>& to = areas[i];
			s32 shift = across ? to.getCenter().X - from.getCenter().X : to.getCenter().Y - from.getCenter().Y;
			if (i == current || (direction == KEY_RIGHT || direction == KEY_DOWN ? shift <= 0 : shift >= 0))
				continue;
			s32 gap = direction == KEY_RIGHT ? to.UpperLeftCorner.X - from.LowerRightCorner.X
				: direction == KEY_LEFT ? from.UpperLeftCorner.X - to.LowerRightCorner.X
				: direction == KEY_DOWN ? to.UpperLeftCorner.Y - from.LowerRightCorner.Y : from.UpperLeftCorner.Y - to.LowerRightCorner.Y;
			s32 apart = across ? std::max(to.UpperLeftCorner.Y - from.LowerRightCorner.Y, from.UpperLeftCorner.Y - to.LowerRightCorner.Y)
				: std::max(to.UpperLeftCorner.X - from.LowerRightCorner.X, from.UpperLeftCorner.X - to.LowerRightCorner.X);
			s32 offset = across ? std::abs(to.getCenter().Y - from.getCenter().Y) : std::abs(to.getCenter().X - from.getCenter().X);
			long long score = (long long)(std::max(gap, 0) + 3 * std::max(apart, 0)) * 65536 + offset;
			if (best == current || score < bestScore)
			{
				best = i;
				bestScore = score;
			}
		}
		return best;
	}
}
