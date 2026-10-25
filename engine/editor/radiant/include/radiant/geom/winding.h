#pragma once
//Convex polygon lying on a plane, replaces GtkRadiant's winding_t (malloc'd, variable sized).

#include "radiant/math/aabb.h"
#include "radiant/math/plane.h"
#include "radiant/math/vec3.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace radiant {

class Winding
{
public:
	Winding() = default;
	explicit Winding(std::vector<Vec3> points) : m_points(std::move(points)) {}

	//A quad covering the plane out to +/-range on its two in-plane axes (Winding_BaseForPlane).
	static Winding baseForPlane(const Plane &plane, double range);

	//Cuts away everything behind `split`, keeping the front side and points within epsilon of it
	//(Winding_Clip with keepon=false). The winding becomes empty if nothing is in front.
	void clipFront(const Plane &split, double epsilon = kOnEpsilon);

	const std::vector<Vec3> &points() const { return m_points; }
	std::size_t size() const { return m_points.size(); }
	bool empty() const { return m_points.empty(); }
	//A winding needs at least three points to enclose any area.
	bool isValid() const { return m_points.size() >= 3; }
	void clear() { m_points.clear(); }

	double area() const;
	Vec3 center() const;
	AABB bounds() const;

private:
	std::vector<Vec3> m_points;
};

} // namespace radiant
