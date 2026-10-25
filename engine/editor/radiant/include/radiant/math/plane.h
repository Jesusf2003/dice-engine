#pragma once
//Plane as normal + distance from the origin: dot(normal, p) == dist for points on it.

#include "radiant/math/vec3.h"

#include <cmath>
#include <optional>

namespace radiant {

//Tolerances inherited from GtkRadiant (qedefs.h ON_EPSILON, Brush_MakeFaceWinding).
inline constexpr double kOnEpsilon = 0.01;
inline constexpr double kPlaneNormalEpsilon = 0.001;	//1 - dot(n1, n2)
inline constexpr double kPlaneDistEpsilon = 0.01;

enum class PlaneSide
{
	Front,
	Back,
	On,
};

struct Plane
{
	Vec3 normal;
	double dist = 0.0;

	//Builds the plane through three .map points. Like Quake, the normal is (p0 - p1) x (p2 - p1),
	//so points listed clockwise when seen from outside give an outward normal.
	//Returns nullopt for collinear/coincident points.
	static std::optional<Plane> fromPoints(const Vec3 &p0, const Vec3 &p1, const Vec3 &p2)
	{
		const Vec3 n = cross(p0 - p1, p2 - p1);
		const double len = length(n);
		if (len <= 0.0)
			return std::nullopt;
		const Vec3 unit = n / len;
		return Plane{unit, dot(p1, unit)};
	}

	Plane flipped() const { return {-normal, -dist}; }

	double distanceTo(const Vec3 &p) const { return dot(normal, p) - dist; }

	PlaneSide classify(const Vec3 &p, double epsilon = kOnEpsilon) const
	{
		const double d = distanceTo(p);
		if (d > epsilon)
			return PlaneSide::Front;
		if (d < -epsilon)
			return PlaneSide::Back;
		return PlaneSide::On;
	}

	//Same orientation and position, within the editor's tolerances.
	bool nearlyEquals(const Plane &o) const
	{
		return dot(normal, o.normal) > 1.0 - kPlaneNormalEpsilon && std::fabs(dist - o.dist) < kPlaneDistEpsilon;
	}

	//Index of the axis the normal points along the most (0=x, 1=y, 2=z).
	int majorAxis() const
	{
		int axis = 0;
		for (int i = 1; i < 3; i++)
			if (std::fabs(normal[i]) > std::fabs(normal[axis]))
				axis = i;
		return axis;
	}
};

} // namespace radiant
