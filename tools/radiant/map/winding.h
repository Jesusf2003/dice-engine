//==============================
//	winding.h
//
//	planes and windings (convex polygons on a plane), the way qbsp builds a
//	brush: every face's plane starts as a huge square and gets clipped by all
//	the other planes, what's left is that face's polygon. from quake's
//	qbsp/winding.c (BaseWindingForPlane, ClipWinding) and PlaneFromPoints
//==============================

#ifndef __WINDING_H__
#define __WINDING_H__

#include "mathlib.h"

#include <vector>

#define PLANE_EPSILON	0.01	// distance a point can be off a plane and still be on it

struct Plane
{
	dvec3	normal;
	double	dist = 0;

	inline double Distance(const dvec3 &p) const	{ return dot(p, normal) - dist; }
};

// quake's convention: the normal is (p0 - p1) x (p2 - p1) and points out of the brush.
// false when the points are collinear (or two of them the same)
bool	PlaneFromPoints(const dvec3 &p0, const dvec3 &p1, const dvec3 &p2, Plane &out);
bool	PlanesEqual(const Plane &a, const Plane &b);

typedef std::vector<dvec3> Winding;

Winding	BaseWindingForPlane(const Plane &p);
void	ClipWinding(Winding &w, const Plane &split);	// keeps what's behind split (inside the brush)
double	WindingArea(const Winding &w);

#endif
