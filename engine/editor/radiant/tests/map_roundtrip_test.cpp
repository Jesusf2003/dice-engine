//Brush construction tests. The .map read/write round trip joins this file once radiant/io exists;
//for now it checks the in-memory side: box -> planes -> windings -> bounds.

#include "check.h"

#include "radiant/host.h"
#include "radiant/scene/brush.h"

#include <string>
#include <variant>

using namespace radiant;

namespace {

constexpr double kEps = 1e-9;

const AABB kBox = AABB::fromMinsMaxs({-64.0, -64.0, -64.0}, {64.0, 64.0, 64.0});

TexDef quakeTex(const char *shader)
{
	return TexDef{shader, QuakeProjection{}};
}

} // namespace

RADIANT_TEST(box_builds_six_quads)
{
	const Brush brush = Brush::makeBox(kBox, quakeTex("base_wall"));

	CHECK(brush.isValid());
	CHECK(brush.faces().size() == 6);
	CHECK(brush.visibleFaceCount() == 6);

	for (const Face &face : brush.faces())
	{
		CHECK(face.hasValidPlane());
		CHECK(face.winding().size() == 4);
		CHECK_NEAR(face.winding().area(), 128.0 * 128.0, 1e-6);
		CHECK(face.texdef.shader == "base_wall");
	}
}

RADIANT_TEST(box_bounds_match_input)
{
	const Brush brush = Brush::makeBox(kBox, quakeTex("base_wall"));

	CHECK(nearlyEqual(brush.bounds(), kBox, kEps));
	CHECK(nearlyEqual(brush.bounds().size(), Vec3{128.0, 128.0, 128.0}, kEps));
	CHECK(nearlyEqual(brush.bounds().center(), Vec3{}, kEps));
}

RADIANT_TEST(box_planes_are_outward_axial_and_contain_their_windings)
{
	const Brush brush = Brush::makeBox(kBox, quakeTex("base_wall"));
	const Vec3 centre = kBox.center();

	int axisHits[3][2] = {};	//[axis][negative, positive]
	for (const Face &face : brush.faces())
	{
		const Plane &plane = face.plane();
		CHECK_NEAR(length(plane.normal), 1.0, kEps);
		CHECK_NEAR(plane.dist, 64.0, kEps);
		CHECK(dot(plane.normal, face.winding().center() - centre) > 0.0);	//points outward

		const int axis = plane.majorAxis();
		CHECK_NEAR(std::fabs(plane.normal[axis]), 1.0, kEps);
		++axisHits[axis][plane.normal[axis] > 0.0 ? 1 : 0];

		for (const Vec3 &p : face.winding().points())
		{
			CHECK_NEAR(plane.distanceTo(p), 0.0, kEps);
			CHECK(kBox.contains(p, kEps));
		}
	}
	for (auto &axis : axisHits)
		CHECK(axis[0] == 1 && axis[1] == 1);	//exactly one face per side
}

RADIANT_TEST(duplicate_face_keeps_only_the_later_one)
{
	Brush brush = Brush::makeBox(kBox, quakeTex("first"));
	Face duplicate = brush.faces().front();
	duplicate.texdef.shader = "second";
	brush.faces().push_back(duplicate);

	CHECK(brush.build());
	CHECK(brush.faces().size() == 7);
	CHECK(brush.visibleFaceCount() == 6);
	CHECK(!brush.faces().front().winding().isValid());
	CHECK(brush.faces().back().winding().isValid());
	CHECK(nearlyEqual(brush.bounds(), kBox, kEps));
}

RADIANT_TEST(degenerate_plane_invalidates_brush)
{
	Brush brush = Brush::makeBox(kBox, quakeTex("base_wall"));
	brush.faces()[2].setPlanePoints({Vec3{0, 0, 0}, Vec3{1, 1, 1}, Vec3{2, 2, 2}});	//collinear

	CHECK(!brush.build());
	CHECK(!brush.faces()[2].hasValidPlane());
}

RADIANT_TEST(open_brush_is_invalid)
{
	Brush brush = Brush::makeBox(kBox, quakeTex("base_wall"));
	brush.faces().resize(3);

	CHECK(!brush.build());
}

RADIANT_TEST(texdef_keeps_projection_kind)
{
	ValveProjection valve;
	valve.uAxis = {0.0, 1.0, 0.0};
	valve.uShift = 16.0;
	valve.scale = {0.5, 0.5};
	const Brush brush = Brush::makeBox(kBox, TexDef{"trim", valve});

	const TexDef &tex = brush.faces().front().texdef;
	CHECK(std::holds_alternative<ValveProjection>(tex.projection));
	CHECK_NEAR(std::get<ValveProjection>(tex.projection).uShift, 16.0, kEps);
	CHECK(std::holds_alternative<QuakeProjection>(quakeTex("x").projection));
}

RADIANT_TEST(host_callbacks_are_optional)
{
	DocumentHost headless;
	headless.warning("nobody listens");	//must not crash
	CHECK(headless.confirm("Overwrite?", false) == false);
	CHECK(headless.confirm("Continue?", true) == true);

	std::string lastMessage;
	LogLevel lastLevel = LogLevel::Info;
	DocumentHost host(
		[&](LogLevel level, std::string_view msg) { lastLevel = level; lastMessage = msg; },
		[](std::string_view, bool) { return true; });
	host.error("brush plane with no normal");
	CHECK(lastLevel == LogLevel::Error);
	CHECK(lastMessage == "brush plane with no normal");
	CHECK(host.confirm("Overwrite?", false));
}

int main()
{
	return radiant::test::runAll();
}
