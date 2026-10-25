#pragma once
//One side of a brush: three plane points plus texturing, as stored in a .map file.
//The plane and winding are derived data, rebuilt by Brush::build().

#include "radiant/geom/winding.h"
#include "radiant/math/plane.h"
#include "radiant/math/vec3.h"

#include <array>
#include <string>
#include <utility>
#include <variant>

namespace radiant {

//Standard Quake: "tex xoff yoff rot xscale yscale", axes picked from the face normal.
struct QuakeProjection
{
	std::array<double, 2> shift{0.0, 0.0};
	double rotate = 0.0;
	std::array<double, 2> scale{1.0, 1.0};
};

//Valve 220: "tex [ ux uy uz uoff ] [ vx vy vz voff ] rot xscale yscale", explicit texture axes.
struct ValveProjection
{
	Vec3 uAxis{1.0, 0.0, 0.0};
	double uShift = 0.0;
	Vec3 vAxis{0.0, -1.0, 0.0};
	double vShift = 0.0;
	double rotate = 0.0;	//kept for round-tripping, the axes already include it
	std::array<double, 2> scale{1.0, 1.0};
};

using TextureProjection = std::variant<QuakeProjection, ValveProjection>;

struct TexDef
{
	std::string shader;	//texture name as written in the .map, without extension
	TextureProjection projection;
};

class Face
{
public:
	Face() = default;
	Face(const std::array<Vec3, 3> &planePoints, TexDef tex) : texdef(std::move(tex)), m_planePoints(planePoints) {}

	//The defining points. Changing them invalidates plane()/winding() until the owning brush is rebuilt.
	const std::array<Vec3, 3> &planePoints() const { return m_planePoints; }
	void setPlanePoints(const std::array<Vec3, 3> &points) { m_planePoints = points; }

	TexDef texdef;

	//Derived by Brush::build().
	const Plane &plane() const { return m_plane; }
	bool hasValidPlane() const { return m_planeValid; }
	//The visible polygon; empty when the face is clipped away or duplicates another face.
	const Winding &winding() const { return m_winding; }

private:
	friend class Brush;

	std::array<Vec3, 3> m_planePoints{};
	Plane m_plane;
	bool m_planeValid = false;
	Winding m_winding;
};

} // namespace radiant
