//Port of the geometry half of GtkRadiant's radiant/brush.cpp
//(Face_MakePlane, Brush_MakeFaceWinding, Brush_BuildWindings, Brush_Create).

#include "radiant/scene/brush.h"

#include <algorithm>

namespace radiant {

Brush Brush::makeBox(const AABB &box, const TexDef &texdef)
{
	const Vec3 &a = box.mins;
	const Vec3 &b = box.maxs;

	//Each triple is clockwise seen from outside, so Plane::fromPoints gives outward normals.
	std::vector<Face> faces;
	faces.reserve(6);
	faces.emplace_back(std::array<Vec3, 3>{Vec3{a.x, a.y, a.z}, Vec3{a.x, b.y, a.z}, Vec3{a.x, a.y, b.z}}, texdef);	//-x
	faces.emplace_back(std::array<Vec3, 3>{Vec3{b.x, a.y, a.z}, Vec3{b.x, a.y, b.z}, Vec3{b.x, b.y, a.z}}, texdef);	//+x
	faces.emplace_back(std::array<Vec3, 3>{Vec3{a.x, a.y, a.z}, Vec3{a.x, a.y, b.z}, Vec3{b.x, a.y, a.z}}, texdef);	//-y
	faces.emplace_back(std::array<Vec3, 3>{Vec3{a.x, b.y, a.z}, Vec3{b.x, b.y, a.z}, Vec3{a.x, b.y, b.z}}, texdef);	//+y
	faces.emplace_back(std::array<Vec3, 3>{Vec3{a.x, a.y, a.z}, Vec3{b.x, a.y, a.z}, Vec3{a.x, b.y, a.z}}, texdef);	//-z
	faces.emplace_back(std::array<Vec3, 3>{Vec3{a.x, a.y, b.z}, Vec3{a.x, b.y, b.z}, Vec3{b.x, a.y, b.z}}, texdef);	//+z

	Brush brush(std::move(faces));
	brush.build();
	return brush;
}

bool Brush::build(const BrushBuildOptions &options)
{
	m_bounds = AABB{};
	m_valid = false;

	bool allPlanesValid = true;
	for (Face &face : m_faces)
	{
		const auto &pts = face.m_planePoints;
		const auto plane = Plane::fromPoints(pts[0], pts[1], pts[2]);
		face.m_planeValid = plane.has_value();
		face.m_plane = plane.value_or(Plane{});
		face.m_winding.clear();
		allPlanesValid &= face.m_planeValid;
	}

	//Each face's polygon: a huge quad on its plane, chopped by every other face.
	for (std::size_t i = 0; i < m_faces.size(); i++)
	{
		Face &face = m_faces[i];
		if (!face.m_planeValid)
			continue;

		Winding w = Winding::baseForPlane(face.m_plane, options.worldRange + 1.0);
		for (std::size_t j = 0; j < m_faces.size() && !w.empty(); j++)
		{
			const Face &clip = m_faces[j];
			if (j == i || !clip.m_planeValid)
				continue;
			if (face.m_plane.nearlyEquals(clip.m_plane))
			{	//identical plane: the later face keeps the polygon
				if (j > i)
					w.clear();
				continue;
			}
			//keep the back side of every other face, i.e. the inside of the brush
			w.clipFront(clip.m_plane.flipped());
		}

		if (w.isValid())
		{
			m_bounds.add(w.bounds());
			face.m_winding = std::move(w);
		}
	}

	m_valid = allPlanesValid && visibleFaceCount() >= 4 && !m_bounds.isEmpty();
	return m_valid;
}

std::size_t Brush::visibleFaceCount() const
{
	return static_cast<std::size_t>(std::count_if(m_faces.begin(), m_faces.end(),
		[](const Face &f) { return f.winding().isValid(); }));
}

} // namespace radiant
