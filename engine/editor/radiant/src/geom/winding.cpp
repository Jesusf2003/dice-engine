//Port of GtkRadiant's radiant/winding.cpp (Winding_BaseForPlane, Winding_Clip).

#include "radiant/geom/winding.h"

namespace radiant {

Winding Winding::baseForPlane(const Plane &plane, double range)
{
	//"up" is whichever world axis is least aligned with the normal, then made perpendicular to it.
	Vec3 up = plane.majorAxis() == 2 ? Vec3{1.0, 0.0, 0.0} : Vec3{0.0, 0.0, 1.0};
	up = normalized(up - plane.normal * dot(up, plane.normal));
	const Vec3 right = cross(up, plane.normal);
	const Vec3 origin = plane.normal * plane.dist;

	up *= range;
	const Vec3 r = right * range;
	return Winding({
		origin - r + up,
		origin + r + up,
		origin + r - up,
		origin - r - up,
	});
}

void Winding::clipFront(const Plane &split, double epsilon)
{
	const std::size_t count = m_points.size();
	if (count == 0)
		return;

	std::vector<double> dists(count);
	std::vector<PlaneSide> sides(count);
	bool anyFront = false;
	bool anyBack = false;
	for (std::size_t i = 0; i < count; i++)
	{
		dists[i] = split.distanceTo(m_points[i]);
		sides[i] = split.classify(m_points[i], epsilon);
		anyFront |= sides[i] == PlaneSide::Front;
		anyBack |= sides[i] == PlaneSide::Back;
	}

	if (!anyBack)
		return;	//entirely in front (or on the plane): untouched
	if (!anyFront)
	{
		m_points.clear();	//entirely behind
		return;
	}

	std::vector<Vec3> out;
	out.reserve(count + 4);
	for (std::size_t i = 0; i < count; i++)
	{
		const std::size_t next = (i + 1) % count;
		const Vec3 &p1 = m_points[i];

		if (sides[i] != PlaneSide::Back)
			out.push_back(p1);
		if (sides[i] == PlaneSide::On || sides[next] == PlaneSide::On || sides[i] == sides[next])
			continue;

		//The edge crosses the plane: emit the intersection point.
		const Vec3 &p2 = m_points[next];
		const double t = dists[i] / (dists[i] - dists[next]);
		Vec3 mid = p1 + (p2 - p1) * t;
		for (int axis = 0; axis < 3; axis++)
		{	//avoid round off error on axial planes
			if (split.normal[axis] == 1.0)
				mid[axis] = split.dist;
			else if (split.normal[axis] == -1.0)
				mid[axis] = -split.dist;
		}
		out.push_back(mid);
	}
	m_points = std::move(out);
}

double Winding::area() const
{
	double total = 0.0;
	for (std::size_t i = 2; i < m_points.size(); i++)
		total += length(cross(m_points[i - 1] - m_points[0], m_points[i] - m_points[0]));
	return total * 0.5;
}

Vec3 Winding::center() const
{
	if (m_points.empty())
		return {};
	Vec3 sum;
	for (const Vec3 &p : m_points)
		sum += p;
	return sum / static_cast<double>(m_points.size());
}

AABB Winding::bounds() const
{
	AABB box;
	for (const Vec3 &p : m_points)
		box.add(p);
	return box;
}

} // namespace radiant
