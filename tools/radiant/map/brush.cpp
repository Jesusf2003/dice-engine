//==============================
//	brush.cpp
//==============================

#include "map/brush.h"
#include "map/grid.h"

#include <algorithm>
#include <cfloat>

int Brush::UsableFaceCount() const
{
	int n = 0;
	for (const BrushFace &f : faces)
		if (!f.winding.empty())
			n++;
	return n;
}

/*
==================
Brush::BuildGeometry

qbsp's CreateBrushWindings: each plane's base winding clipped by every other
plane. a degenerate or repeated plane is left out first, so it can't cut the
rest down to nothing
==================
*/
bool Brush::BuildGeometry(std::vector<BrushProblem> &problems)
{
	size_t n = faces.size();
	int usablePlanes = 0;

	for (size_t i = 0; i < n; i++)
	{
		BrushFace &f = faces[i];
		f.winding.clear();
		f.planeValid = PlaneFromPoints(f.points[0], f.points[1], f.points[2], f.plane);
		if (!f.planeValid)
		{
			problems.push_back({ BrushProblem::DEGENERATE_FACE, (int)i, -1 });
			continue;
		}

		for (size_t j = 0; j < i; j++)
		{
			if (faces[j].planeValid && PlanesEqual(faces[j].plane, f.plane))
			{
				problems.push_back({ BrushProblem::DUPLICATE_PLANE, (int)i, (int)j });
				f.planeValid = false;
				break;
			}
		}
		if (f.planeValid)
			usablePlanes++;
	}

	valid = false;
	mins = dvec3(DBL_MAX, DBL_MAX, DBL_MAX);
	maxs = dvec3(-DBL_MAX, -DBL_MAX, -DBL_MAX);

	if (usablePlanes < 4)
	{
		problems.push_back({ BrushProblem::TOO_FEW_FACES, -1, -1 });
		return false;
	}

	for (size_t i = 0; i < n; i++)
	{
		BrushFace &f = faces[i];
		if (!f.planeValid)
			continue;

		Winding w = BaseWindingForPlane(f.plane);
		for (size_t j = 0; j < n && w.size() >= 3; j++)
		{
			if (j != i && faces[j].planeValid)
				ClipWinding(w, faces[j].plane);
		}

		// a sliver squeezed between two planes still has points, but no area
		if (w.size() < 3 || WindingArea(w) < PLANE_EPSILON)
		{
			problems.push_back({ BrushProblem::REDUNDANT_FACE, (int)i, -1 });
			continue;
		}
		f.winding.swap(w);

		for (const dvec3 &p : f.winding)
		{
			for (int k = 0; k < 3; k++)
			{
				mins[k] = std::min(mins[k], p[k]);
				maxs[k] = std::max(maxs[k], p[k]);
			}
		}
	}

	// a closed convex solid needs at least 4 polygons (a tetrahedron)
	if (UsableFaceCount() < 4)
	{
		problems.push_back({ BrushProblem::NO_VOLUME, -1, -1 });
		return false;
	}

	// far past the world only happens when nothing closes that side off
	double extent = 0;
	for (int k = 0; k < 3; k++)
		extent = std::max(extent, std::max(-mins[k], maxs[k]));

	if (extent > MAX_WORLD_COORD * 2)
	{
		problems.push_back({ BrushProblem::UNBOUNDED, -1, -1 });
		return false;
	}
	if (extent > MAX_WORLD_COORD)
		problems.push_back({ BrushProblem::OUT_OF_BOUNDS, -1, -1 });

	valid = true;
	return true;
}

/*
==================
Brush::RayCast

the ray against a convex solid: it's inside past every plane it crosses going in
and before the first one it crosses going out
==================
*/
bool Brush::RayCast(const dvec3 &start, const dvec3 &dir, double &dist) const
{
	if (!valid)
		return false;

	double enter = -DBL_MAX, leave = DBL_MAX;

	for (const BrushFace &f : faces)
	{
		if (!f.planeValid || f.winding.empty())
			continue;

		double d = f.plane.Distance(start);
		double denom = dot(f.plane.normal, dir);
		if (fabs(denom) < 1e-9)
		{
			if (d > 0)
				return false;		// parallel and outside
			continue;
		}

		double t = -d / denom;
		if (denom < 0)
			enter = std::max(enter, t);
		else
			leave = std::min(leave, t);
		if (enter > leave)
			return false;
	}

	if (enter < 0)		// behind the start, or the start is inside
		return false;
	dist = enter;
	return true;
}
