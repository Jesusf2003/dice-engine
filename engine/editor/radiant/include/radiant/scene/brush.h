#pragma once
//Convex brush: the intersection of the back half-spaces of its faces (replaces brush_t).
//Owns its faces directly; no intrusive lists, render state or UI handles.

#include "radiant/math/aabb.h"
#include "radiant/scene/face.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace radiant {

struct BrushBuildOptions
{
	//Half-extent of the initial winding for each face (GtkRadiant's g_MaxWorldCoord).
	double worldRange = 65536.0;
};

class Brush
{
public:
	Brush() = default;
	explicit Brush(std::vector<Face> faces) : m_faces(std::move(faces)) {}

	//An axis aligned box with the same texdef on every side, already built.
	static Brush makeBox(const AABB &box, const TexDef &texdef);

	//Editing faces invalidates the derived data; call build() afterwards.
	std::vector<Face> &faces() { return m_faces; }
	const std::vector<Face> &faces() const { return m_faces; }

	//Rebuilds every face's plane and winding, then the bounds (Brush_Build without the
	//rendering/undo side effects). Returns isValid().
	bool build(const BrushBuildOptions &options = {});

	//True when the last build() produced a closed solid: every face has a valid plane,
	//at least four faces have windings and the bounds aren't empty.
	bool isValid() const { return m_valid; }
	const AABB &bounds() const { return m_bounds; }

	//Faces that ended up with a polygon (the others are redundant or degenerate).
	std::size_t visibleFaceCount() const;

private:
	std::vector<Face> m_faces;
	AABB m_bounds;
	bool m_valid = false;
};

} // namespace radiant
