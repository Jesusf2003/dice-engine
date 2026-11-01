//==============================
//	brush.h
//
//	a quake brush: the convex space behind a set of planes, each given by three
//	points and carrying a texture. the face polygons (windings) are derived,
//	built by BuildGeometry the way qbsp does, which is also how a bad brush is
//	found. renderer/mapRenderer draws them
//==============================

#ifndef __BRUSH_H__
#define __BRUSH_H__

#include "map/winding.h"

#include <string>
#include <vector>

/*
==================
FaceTexture

both texture projections the .map formats use:
	standard:	TEX xoff yoff rot xscale yscale (axes picked from the face normal)
	valve 220:	TEX [ ux uy uz uoff ] [ vx vy vz voff ] rot xscale yscale
==================
*/
struct FaceTexture
{
	std::string	name;
	bool	valve = false;
	dvec3	axis[2];		// valve 220 only: u and v
	double	shift[2] = { 0, 0 };
	double	rotate = 0;		// the standard format's rotation; valve keeps it for reference only
	double	scale[2] = { 1, 1 };
};

struct BrushFace
{
	dvec3		points[3];
	FaceTexture	tex;
	int			line = 0;		// in the .map file

	// geometry
	Plane		plane;
	bool		planeValid = false;
	Winding		winding;		// empty when the face doesn't touch the brush (redundant)
};

// what BuildGeometry found wrong, the validator turns these into messages
struct BrushProblem
{
	enum Kind
	{
		DEGENERATE_FACE,	// the three points don't make a plane; the face is ignored
		DUPLICATE_PLANE,	// same plane as an earlier face; ignored
		REDUNDANT_FACE,		// the plane doesn't touch the brush, qbsp drops it
		TOO_FEW_FACES,		// fewer than 4 usable planes, can't enclose anything
		NO_VOLUME,			// the planes don't enclose a closed space
		UNBOUNDED,			// the planes leave it open, it runs off to the base windings' edge
		OUT_OF_BOUNDS,		// reaches past MAX_WORLD_COORD
	};

	Kind	kind;
	int		face;			// index into faces, -1 for the whole brush
	int		other;			// DUPLICATE_PLANE: the face it repeats
};

class Brush
{
public:
	std::vector<BrushFace>	faces;
	int		line = 0;			// the brush's opening brace in the .map file

	// geometry
	dvec3	mins, maxs;
	bool	valid = false;

	// rebuilds every face's plane and winding and the bounds. false when the
	// brush can't exist (problems has TOO_FEW_FACES, NO_VOLUME or UNBOUNDED)
	bool	BuildGeometry(std::vector<BrushProblem> &problems);
	int		UsableFaceCount() const;	// faces with a winding

	// distance along dir (unit) to where the ray enters the brush, false when
	// it misses or starts inside. needs the geometry
	bool	RayCast(const dvec3 &start, const dvec3 &dir, double &dist) const;
};

#endif
