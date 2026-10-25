#pragma once
//Axis aligned bounding box. A default constructed AABB is empty (mins > maxs) until a point is added.

#include "radiant/math/vec3.h"

#include <limits>

namespace radiant {

struct AABB
{
	Vec3 mins{std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
	Vec3 maxs{-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};

	static AABB fromMinsMaxs(const Vec3 &mins, const Vec3 &maxs)
	{
		AABB box;
		box.mins = componentMin(mins, maxs);
		box.maxs = componentMax(mins, maxs);
		return box;
	}

	bool isEmpty() const { return mins.x > maxs.x || mins.y > maxs.y || mins.z > maxs.z; }

	void add(const Vec3 &p)
	{
		mins = componentMin(mins, p);
		maxs = componentMax(maxs, p);
	}
	void add(const AABB &o)
	{
		if (o.isEmpty())
			return;
		add(o.mins);
		add(o.maxs);
	}

	Vec3 size() const { return isEmpty() ? Vec3{} : maxs - mins; }
	Vec3 center() const { return isEmpty() ? Vec3{} : (mins + maxs) * 0.5; }

	bool contains(const Vec3 &p, double epsilon = 0.0) const
	{
		for (int i = 0; i < 3; i++)
			if (p[i] < mins[i] - epsilon || p[i] > maxs[i] + epsilon)
				return false;
		return true;
	}

	//Touching boxes don't count as intersecting unless epsilon is positive.
	bool intersects(const AABB &o, double epsilon = 0.0) const
	{
		if (isEmpty() || o.isEmpty())
			return false;
		for (int i = 0; i < 3; i++)
			if (mins[i] >= o.maxs[i] + epsilon || maxs[i] <= o.mins[i] - epsilon)
				return false;
		return true;
	}
};

inline bool nearlyEqual(const AABB &a, const AABB &b, double epsilon)
{
	return a.isEmpty() == b.isEmpty() && (a.isEmpty() || (nearlyEqual(a.mins, b.mins, epsilon) && nearlyEqual(a.maxs, b.maxs, epsilon)));
}

} // namespace radiant
