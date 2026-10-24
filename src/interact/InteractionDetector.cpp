#include "InteractionDetector.h"
#include <algorithm>
#include <cmath>

using namespace irr;
using namespace core;

const float InteractionDetector::CONE_DEGREES = 7.5f;
const float InteractionDetector::STICKY_DEGREES = 2.5f;

namespace
{
	vector3df flat(const vector3df& v)
	{
		return vector3df(v.X, 0.0f, v.Z);
	}

	f32 degreesOff(const vector3df& ahead, const vector3df& toward)
	{
		vector3df direction = flat(toward);
		direction.normalize();
		return std::acos(std::min(std::max(ahead.dotProduct(direction), -1.0f), 1.0f)) * RADTODEG;
	}
}

void InteractionDetector::update(const vector3df& playerPos, const vector3df& cameraForward, const vector3df& cameraPos)
{
	vector3df ahead = flat(cameraForward);
	ahead.normalize();
	inRange_.clear();
	for (Interactable* obj : all_)
	{
		vector3df at = obj->getPosition();
		if (flat(at - playerPos).getLength() > obj->getInfo().interactionRange)
			continue;
		if (flat(at - cameraPos).dotProduct(ahead) <= 0.0f)
			continue;
		inRange_.push_back(obj);
	}
	if (inRange_.size() <= 1)
	{
		target_ = inRange_.empty() ? nullptr : inRange_.front();
		return;
	}

	Interactable* best = nullptr;
	f32 bestAngle = CONE_DEGREES;
	for (Interactable* obj : inRange_)
	{
		f32 angle = degreesOff(ahead, obj->getPosition() - cameraPos);
		if (obj == target_)
			angle -= STICKY_DEGREES;
		if (angle <= bestAngle)
		{
			best = obj;
			bestAngle = angle;
		}
	}
	if (!best)
	{
		f32 nearest = 0.0f;
		for (Interactable* obj : inRange_)
		{
			f32 distance = flat(obj->getPosition() - playerPos).getLength();
			if (!best || distance < nearest)
			{
				best = obj;
				nearest = distance;
			}
		}
	}
	target_ = best;
}

void InteractionDetector::addInteractable(Interactable* obj)
{
	if (obj && std::find(all_.begin(), all_.end(), obj) == all_.end())
		all_.push_back(obj);
}

void InteractionDetector::removeInteractable(Interactable* obj)
{
	all_.erase(std::remove(all_.begin(), all_.end(), obj), all_.end());
	inRange_.erase(std::remove(inRange_.begin(), inRange_.end(), obj), inRange_.end());
	if (target_ == obj)
		target_ = nullptr;
}
