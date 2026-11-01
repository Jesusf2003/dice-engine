//==============================
//	winding.cpp
//==============================

#include "map/winding.h"
#include "map/grid.h"

#define BOGUS_RANGE		(MAX_WORLD_COORD * 4.0)	// past anything a face can reach

bool PlaneFromPoints(const dvec3 &p0, const dvec3 &p1, const dvec3 &p2, Plane &out)
{
	dvec3 normal = cross(p0 - p1, p2 - p1);
	double len = length(normal);

	if (len < 1e-9)
		return false;
	out.normal = normal / len;
	out.dist = dot(p0, out.normal);
	return true;
}

bool PlanesEqual(const Plane &a, const Plane &b)
{
	return dot(a.normal, b.normal) > 1.0 - 1e-9 && fabs(a.dist - b.dist) < PLANE_EPSILON;
}

/*
==================
BaseWindingForPlane

qbsp: a square bigger than the world on the plane, built around the axis the
normal points along the least
==================
*/
Winding BaseWindingForPlane(const Plane &p)
{
	dvec3 up = (abs_max_axis(p.normal) == 2) ? dvec3(1, 0, 0) : dvec3(0, 0, 1);

	up = up - p.normal * dot(up, p.normal);
	up = up / length(up);

	dvec3 right = cross(up, p.normal);
	dvec3 org = p.normal * p.dist;

	up = up * BOGUS_RANGE;
	right = right * BOGUS_RANGE;

	return Winding{
		org - right + up,
		org + right + up,
		org + right - up,
		org - right - up,
	};
}

/*
==================
ClipWinding

qbsp's ClipWinding, keeping the back side. points within PLANE_EPSILON count
as on the plane and stay; an edge crossing from front to back gets split
==================
*/
void ClipWinding(Winding &w, const Plane &split)
{
	enum { SIDE_FRONT, SIDE_BACK, SIDE_ON };

	size_t n = w.size();
	if (n == 0)
		return;

	std::vector<double> dists(n);
	std::vector<int> sides(n);
	bool anyFront = false;

	for (size_t i = 0; i < n; i++)
	{
		dists[i] = split.Distance(w[i]);
		if (dists[i] > PLANE_EPSILON)
		{
			sides[i] = SIDE_FRONT;
			anyFront = true;
		}
		else if (dists[i] < -PLANE_EPSILON)
			sides[i] = SIDE_BACK;
		else
			sides[i] = SIDE_ON;
	}

	if (!anyFront)
		return;		// all behind or on, nothing to cut

	Winding out;
	out.reserve(n + 4);

	for (size_t i = 0; i < n; i++)
	{
		size_t j = (i + 1) % n;

		if (sides[i] != SIDE_FRONT)
			out.push_back(w[i]);

		if (sides[i] == SIDE_ON || sides[j] == SIDE_ON || sides[i] == sides[j])
			continue;

		// the edge crosses the plane: the point where it does
		double t = dists[i] / (dists[i] - dists[j]);
		dvec3 mid = w[i] + (w[j] - w[i]) * t;

		// exact on axial planes, like qbsp, so 64 stays 64
		for (int k = 0; k < 3; k++)
		{
			if (split.normal[k] == 1)
				mid[k] = split.dist;
			else if (split.normal[k] == -1)
				mid[k] = -split.dist;
		}
		out.push_back(mid);
	}

	w.swap(out);
}

double WindingArea(const Winding &w)
{
	dvec3 sum;

	// fan from the first point, the cross products add up to twice the area along the normal
	for (size_t i = 2; i < w.size(); i++)
		sum = sum + cross(w[i - 1] - w[0], w[i] - w[0]);
	return length(sum) * 0.5;
}
