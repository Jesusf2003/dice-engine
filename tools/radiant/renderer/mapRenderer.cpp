//==============================
//	mapRenderer.cpp
//==============================

#include "renderer/mapRenderer.h"
#include "renderer/camera.h"
#include "renderer/glFuncs.h"
#include "renderer/texture.h"
#include "renderer/textures.h"
#include "map/map.h"
#include "map/selection.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <map>
#include <tuple>

MapRenderer g_mapRenderer;

#define TRANSLUCENT_ALPHA	0.5f	// liquids, clip, hint: BFL_TRANS
#define ATLAS_WIDTH			4096	// lightmap atlas, samples; the world is 65536 / 16 across
#define MAX_ATLAS_HEIGHT	16384

// trenchbroom's edge colors: light lines on their own, dark ones over faces
static const float s_wireTint[4] = { 0.85f, 0.85f, 0.85f, 1.0f };
static const float s_edgeTint[4] = { 0.0f, 0.0f, 0.0f, 0.4f };

/*
========================================================================

	SHADERS

	one program for faces, entity boxes and lines. Mode picks the color:
	0 the vertex color, 1 FlatColor (a texture's average), 2 the texture.
	Shade is trenchbroom's ShadeFaces, weighted by the normal's axis.
	Lighting multiplies by the baked lightmap, 128 being the texture as is
	and 255 twice that: FTE's overbright for quake BSPs (gl_overbright 1)
========================================================================
*/

static const char *s_mapVertex = R"(#version 330 core
uniform mat4 ModelViewProjection;
uniform vec2 TexSize;
uniform vec2 LightmapSize;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec4 color;
layout(location = 4) in vec2 lightmapCoord;

out vec3 vNormal;
out vec2 vTexCoord;
out vec2 vLightmapCoord;
out vec4 vColor;

void main()
{
	gl_Position = ModelViewProjection * vec4(position, 1.0);
	vNormal = normal;
	vTexCoord = texCoord / TexSize;
	vLightmapCoord = lightmapCoord / LightmapSize;
	vColor = color;
}
)";

static const char *s_mapFragment = R"(#version 330 core
uniform int Mode;
uniform vec4 FlatColor;
uniform vec4 Tint;
uniform float Alpha;
uniform bool Shade;
uniform bool Lighting;
uniform sampler2D Texture;
uniform sampler2D Lightmap;

in vec3 vNormal;
in vec2 vTexCoord;
in vec2 vLightmapCoord;
in vec4 vColor;
out vec4 fragColor;

void main()
{
	vec4 c;
	if (Mode == 2)
	{
		c = texture(Texture, vTexCoord);
		if (c.a < 0.5)		// half-life's {masked textures
			discard;
		c.a = 1.0;
	}
	else if (Mode == 1)
		c = FlatColor;
	else
		c = vColor;

	if (Lighting)
		c.rgb = min(c.rgb * texture(Lightmap, vLightmapCoord).rgb * (255.0 / 128.0), vec3(1.0));
	else if (Shade)
	{
		vec3 n = normalize(vNormal);
		c.rgb *= dot(n * n, vec3(0.8, 0.9, 1.0));
	}
	c *= Tint;
	fragColor = vec4(c.rgb, c.a * Alpha);
}
)";

static const char *s_drawModeNames[MapRenderer::DRAWMODE_COUNT] =
{
	"3D Wireframe",
	"3D Filled Polygons",
	"3D Textured Polygons",
	"3D Shaded Textured Polygons",
	"3D Lighting Preview",
};

const char *MapRenderer::DrawModeName(DrawMode mode)
{
	return (mode >= 0 && mode < DRAWMODE_COUNT) ? s_drawModeNames[mode] : "";
}


/*
========================================================================

	GEOMETRY

========================================================================
*/

struct mapVertex_t
{
	float	pos[3];
	float	normal[3];
	float	uv[2];		// texels
	float	color[4];
	float	lm[2];		// lightmap atlas texels
};

static mapVertex_t MakeVertex(const vec3 &p, const vec3 &n, float u, float v, const vec3 &color, float lmu = 0, float lmv = 0)
{
	return mapVertex_t{ { p.x, p.y, p.z }, { n.x, n.y, n.z }, { u, v }, { color.x, color.y, color.z, 1.0f }, { lmu, lmv } };
}

/*
==================
TextureVecs

the two vectors a point is dotted with for its texture coordinates, scale
folded in. the standard projection is qbsp's TextureAxisFromPlane plus the
rotation; valve 220 gives the axes outright. the lightmap grid is laid on
these same coordinates
==================
*/
static const dvec3 s_baseAxis[18] =
{
	dvec3(0, 0, 1), dvec3(1, 0, 0), dvec3(0, -1, 0),	// floor
	dvec3(0, 0, -1), dvec3(1, 0, 0), dvec3(0, -1, 0),	// ceiling
	dvec3(1, 0, 0), dvec3(0, 1, 0), dvec3(0, 0, -1),	// west wall
	dvec3(-1, 0, 0), dvec3(0, 1, 0), dvec3(0, 0, -1),	// east wall
	dvec3(0, 1, 0), dvec3(1, 0, 0), dvec3(0, 0, -1),	// south wall
	dvec3(0, -1, 0), dvec3(1, 0, 0), dvec3(0, 0, -1),	// north wall
};

static void TextureVecs(const BrushFace &f, dvec3 vecs[2])
{
	const FaceTexture &t = f.tex;

	if (t.valve)
	{
		vecs[0] = t.axis[0];
		vecs[1] = t.axis[1];
	}
	else
	{
		int best = 0;
		double bestDot = 0;
		for (int i = 0; i < 6; i++)
		{
			double d = dot(f.plane.normal, s_baseAxis[i * 3]);
			if (d > bestDot)
			{
				bestDot = d;
				best = i;
			}
		}
		vecs[0] = s_baseAxis[best * 3 + 1];
		vecs[1] = s_baseAxis[best * 3 + 2];

		// exact for the right angles, they're by far the common ones
		double sinv, cosv;
		if (t.rotate == 0)			{ sinv = 0; cosv = 1; }
		else if (t.rotate == 90)	{ sinv = 1; cosv = 0; }
		else if (t.rotate == 180)	{ sinv = 0; cosv = -1; }
		else if (t.rotate == 270)	{ sinv = -1; cosv = 0; }
		else
		{
			double a = t.rotate / 180.0 * 3.14159265358979323846;
			sinv = sin(a);
			cosv = cos(a);
		}

		int sv = vecs[0][0] != 0 ? 0 : vecs[0][1] != 0 ? 1 : 2;
		int tv = vecs[1][0] != 0 ? 0 : vecs[1][1] != 0 ? 1 : 2;
		for (int i = 0; i < 2; i++)
		{
			double ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
			double nt = sinv * vecs[i][sv] + cosv * vecs[i][tv];
			vecs[i][sv] = ns;
			vecs[i][tv] = nt;
		}
	}

	for (int i = 0; i < 2; i++)
		vecs[i] = vecs[i] / (t.scale[i] != 0 ? t.scale[i] : 1.0);
}

/*
==================
AddBox

a point entity: 6 quads facing out, and its 12 edges
==================
*/
static void AddBox(std::vector<mapVertex_t> &tris, std::vector<mapVertex_t> &lines, const dvec3 &mins, const dvec3 &maxs, const vec3 &color)
{
	vec3 lo = to_vec3(mins), hi = to_vec3(maxs);
	vec3 c[8];
	for (int i = 0; i < 8; i++)
		c[i] = vec3((i & 1) ? hi.x : lo.x, (i & 2) ? hi.y : lo.y, (i & 4) ? hi.z : lo.z);

	// corner indices of each side, counterclockwise from outside
	static const int quads[6][4] = {
		{ 0, 4, 6, 2 }, { 1, 3, 7, 5 },		// -x, +x
		{ 0, 1, 5, 4 }, { 2, 6, 7, 3 },		// -y, +y
		{ 0, 2, 3, 1 }, { 4, 5, 7, 6 },		// -z, +z
	};
	static const vec3 normals[6] = {
		vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, -1, 0), vec3(0, 1, 0), vec3(0, 0, -1), vec3(0, 0, 1),
	};
	static const int tri[6] = { 0, 1, 2, 0, 2, 3 };

	for (int q = 0; q < 6; q++)
		for (int k : tri)
			tris.push_back(MakeVertex(c[quads[q][k]], normals[q], 0, 0, color));

	static const int edges[12][2] = {
		{ 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 },		// along x
		{ 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 },		// along y
		{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },		// along z
	};
	for (const int *e : edges)
	{
		lines.push_back(MakeVertex(c[e[0]], vec3(), 0, 0, color));
		lines.push_back(MakeVertex(c[e[1]], vec3(), 0, 0, color));
	}
}

static void SetupVertexArray(unsigned int &vao, unsigned int &vbo)
{
	qglGenVertexArrays(1, &vao);
	qglGenBuffers(1, &vbo);
	qglBindVertexArray(vao);
	qglBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLsizei stride = sizeof(mapVertex_t);
	qglEnableVertexAttribArray(0);
	qglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(mapVertex_t, pos));
	qglEnableVertexAttribArray(1);
	qglVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(mapVertex_t, normal));
	qglEnableVertexAttribArray(2);
	qglVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(mapVertex_t, uv));
	qglEnableVertexAttribArray(3);
	qglVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(mapVertex_t, color));
	qglEnableVertexAttribArray(4);
	qglVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(mapVertex_t, lm));

	qglBindVertexArray(0);
	qglBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void Upload(unsigned int vbo, const std::vector<mapVertex_t> &verts)
{
	qglBindBuffer(GL_ARRAY_BUFFER, vbo);
	qglBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(mapVertex_t), verts.empty() ? nullptr : verts.data(), GL_STATIC_DRAW);
	qglBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*
==================
AtlasPacker

shelves, left to right then down. every face gets a texel of padding round it
==================
*/
struct AtlasPacker
{
	int		x = 0, y = 0, shelf = 0;

	bool Place(int w, int h, int &outX, int &outY)
	{
		if (w > ATLAS_WIDTH)
			return false;
		if (x + w > ATLAS_WIDTH)
		{
			y += shelf;
			x = shelf = 0;
		}
		if (y + h > MAX_ATLAS_HEIGHT)
			return false;
		outX = x;
		outY = y;
		x += w;
		shelf = std::max(shelf, h);
		return true;
	}
	int Height() const	{ return std::max(y + shelf, 1); }
};


/*
========================================================================

	LIGHTS

	the entity keys the way ericw-tools' light reads them (light/entities.cc,
	light/light.cc), turned into what LightBaker works with
========================================================================
*/

// "r g b" as 0-255 or 0-1, both are around (normalize_color_format); 0..1 out
static vec3 KeyColor(const Entity &e, const char *key, const vec3 &def)
{
	const std::string &s = e.ValueForKey(key);
	if (s.empty())
		return def;

	double c[3] = { 0, 0, 0 };
	const char *p = s.c_str();
	for (int i = 0; i < 3; i++)
	{
		char *end;
		c[i] = strtod(p, &end);
		if (end == p)
			return def;
		p = end;
	}
	if (c[0] > 1 || c[1] > 1 || c[2] > 1)
		return vec3((float)c[0], (float)c[1], (float)c[2]) / 255.0f;
	return vec3((float)c[0], (float)c[1], (float)c[2]);
}

static float KeyFloat(const Entity &e, const char *key, float def)
{
	const std::string &s = e.ValueForKey(key);
	return s.empty() ? def : (float)atof(s.c_str());
}

// vec_from_mangle: yaw pitch roll, pitch -90 points straight down
static vec3 VecFromMangle(const std::string &s)
{
	const char *p = s.c_str();
	char *end;
	float yaw = strtof(p, &end);
	float pitch = (end != p) ? strtof(end, nullptr) : 0.0f;
	yaw = DEG2RAD(yaw);
	pitch = DEG2RAD(pitch);
	return vec3(cosf(yaw) * cosf(pitch), sinf(yaw) * cosf(pitch), sinf(pitch));
}

static const Entity *FindTarget(const Entity &e)
{
	const std::string &target = e.ValueForKey("target");
	if (target.empty())
		return nullptr;
	for (const Entity &t : g_map.entities)
		if (t.ValueForKey("targetname") == target)
			return &t;
	return nullptr;
}

/*
==================
LightReach

the distance past which a light can't add a quarter of a lightmap step
(after _range), so the baker skips it there
==================
*/
static float LightReach(const bakeLight_t &l, float rangeScale)
{
	float gate = 0.25f / std::max(rangeScale, 0.001f);
	float light = fabsf(l.light);

	if (l.formula == 0 && l.falloff > 0)
		return l.falloff;

	switch (l.formula)
	{
	case 0:		return light / l.atten + 1;
	case 1:		return light * 128.0f / gate / l.atten + 1;
	case 2:		return 128.0f * sqrtf(light / gate) / l.atten + 1;
	case 5:		return std::max(128.0f * sqrtf(light / gate) - 128.0f, 0.0f) / l.atten + 1;
	default:	return 1e9f;	// no falloff
	}
}

static void AddSun(bakeInput_t &bake, const vec3 &travel, float light, const vec3 &color, float anglescale)
{
	if (light == 0 || length(travel) < 1e-6f)
		return;
	bake.suns.push_back(bakeSun_t{ -normalize(travel), light, color, anglescale });
}

/*
==================
AddSkyDome

entities.cc's SetupSkyDome (from q3map2): _sunlight2 as a dome of suns
over the sky, _sunlight3 the same from below. sunsamples is ericw's 64
==================
*/
static void AddSkyDome(bakeInput_t &bake, float upper, const vec3 &upperColor, float lower, const vec3 &lowerColor, float anglescale)
{
	if (upper <= 0 && lower <= 0)
		return;

	int iterations = std::max((int)rint(sqrt((64 - 1) / 4.0)) + 1, 2);
	int elevationSteps = iterations - 1;
	int angleSteps = elevationSteps * 4;
	float elevationStep = DEG2RAD(90.0f / (elevationSteps + 1));
	float angleStep = DEG2RAD(360.0f / angleSteps);
	int numSuns = angleSteps * elevationSteps + 1;
	float upperValue = upper / numSuns, lowerValue = lower / numSuns;

	float elevation = elevationStep * 0.5f, angle = 0;
	for (int i = 0; i < elevationSteps; i++)
	{
		for (int j = 0; j < angleSteps; j++)
		{
			vec3 dir(cosf(angle) * cosf(elevation), sinf(angle) * cosf(elevation), -sinf(elevation));
			if (upperValue > 0)
				AddSun(bake, dir, upperValue, upperColor, anglescale);
			dir.z = -dir.z;
			if (lowerValue > 0)
				AddSun(bake, dir, lowerValue, lowerColor, anglescale);
			angle += angleStep;
		}
		elevation += elevationStep;
		angle += angleStep / elevationSteps;
	}

	// straight up and down
	if (upperValue > 0)
		AddSun(bake, vec3(0, 0, -1), upperValue, upperColor, anglescale);
	if (lowerValue > 0)
		AddSun(bake, vec3(0, 0, 1), lowerValue, lowerColor, anglescale);
}

void MapRenderer::GatherLights(bakeInput_t &bake)
{
	const Entity *world = g_map.World();
	static const Entity empty{};
	const Entity &w = world ? *world : empty;

	bake.rangeScale = KeyFloat(w, "_range", 0.5f);
	bake.gamma = KeyFloat(w, "_gamma", 1.0f);
	if (bake.gamma <= 0)
		bake.gamma = 1.0f;
	float scaleDist = KeyFloat(w, "_dist", 1.0f);
	float globalAnglescale = KeyFloat(w, "_anglescale", KeyFloat(w, "_anglesense", 0.5f));

	float minlight = KeyFloat(w, "light", KeyFloat(w, "_minlight", 0));
	bake.minlight = KeyColor(w, "_minlight_color", vec3(1, 1, 1)) * minlight;

	vec3 sunTravel = w.HasKey("_sun_mangle") ? VecFromMangle(w.ValueForKey("_sun_mangle")) :
		w.HasKey("_sunlight_mangle") ? VecFromMangle(w.ValueForKey("_sunlight_mangle")) : vec3(0, 0, -1);
	AddSun(bake, sunTravel, KeyFloat(w, "_sunlight", 0), KeyColor(w, "_sunlight_color", vec3(1, 1, 1)), globalAnglescale);
	AddSkyDome(bake, KeyFloat(w, "_sunlight2", 0), KeyColor(w, "_sunlight2_color", vec3(1, 1, 1)),
		KeyFloat(w, "_sunlight3", 0), KeyColor(w, "_sunlight3_color", vec3(1, 1, 1)), globalAnglescale);

	for (const Entity &e : g_map.entities)
	{
		const std::string &cls = e.Classname();
		if (cls.compare(0, 5, "light") != 0 || !e.brushes.empty())
			continue;

		// a switchable light that starts off isn't in the lightmap the game shows first
		if (((int)KeyFloat(e, "spawnflags", 0) & 1) && e.HasKey("targetname"))
			continue;

		bakeLight_t l = {};
		l.light = KeyFloat(e, "light", 300);
		if (l.light == 0)
			l.light = 300;	// ericw: 0 means the default
		l.color = KeyColor(e, "_color", vec3(1, 1, 1));
		l.anglescale = std::clamp(KeyFloat(e, "_anglescale", KeyFloat(e, "_anglesense", globalAnglescale)), 0.0f, 1.0f);
		l.origin = to_vec3(e.Origin());
		const Entity *target = FindTarget(e);

		// "_sun" turns the entity into a sun, pointing along its mangle or at its target
		if (KeyFloat(e, "_sun", 0) != 0)
		{
			vec3 travel = e.HasKey("mangle") ? VecFromMangle(e.ValueForKey("mangle")) :
				target ? to_vec3(target->Origin()) - l.origin : vec3(0, 0, -1);
			AddSun(bake, travel, l.light, l.color, l.anglescale);
			continue;
		}

		l.formula = (int)KeyFloat(e, "delay", 0);
		if (l.formula < 0 || l.formula > 5)
			l.formula = 0;
		float wait = KeyFloat(e, "wait", 1);
		l.atten = (wait > 0 ? wait : 1.0f) * scaleDist;
		l.falloff = KeyFloat(e, "_falloff", 0);

		// quake's spotlights: a mangle, or a target to point at
		if (e.HasKey("mangle") || target)
		{
			l.spot = true;
			l.spotDir = e.HasKey("mangle") ? VecFromMangle(e.ValueForKey("mangle")) : normalize(to_vec3(target->Origin()) - l.origin);
			float angle = KeyFloat(e, "angle", 40);
			if (angle <= 0)
				angle = 40;
			float soft = KeyFloat(e, "_softangle", 0);
			l.spotOuter = cosf(DEG2RAD(angle) / 2);
			l.spotInner = (soft > 0 && soft < angle) ? cosf(DEG2RAD(soft) / 2) : l.spotOuter;
			if (l.spotInner <= l.spotOuter)
				l.spotInner = l.spotOuter + 1e-4f;
		}

		l.reach = LightReach(l, bake.rangeScale);
		bake.lights.push_back(l);
	}

	lightCount = (int)bake.lights.size();
	sunCount = (int)bake.suns.size();
}


/*
========================================================================

	RENDERER

========================================================================
*/

MapRenderer::~MapRenderer()
{
	Shutdown();
}

bool MapRenderer::Init()
{
	if (!shader.Build("map3d", s_mapVertex, s_mapFragment))
		return false;

	SetupVertexArray(faceVao, faceVbo);
	SetupVertexArray(lineVao, lineVbo);
	SetupVertexArray(selFaceVao, selFaceVbo);
	SetupVertexArray(selLineVao, selLineVbo);
	selRevision = 0;

	glGenTextures(1, &lightmapTex);
	glBindTexture(GL_TEXTURE_2D, lightmapTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	built = false;
	return true;
}

void MapRenderer::Shutdown()
{
	baker.Cancel();
	if (faceVbo) qglDeleteBuffers(1, &faceVbo);
	if (faceVao) qglDeleteVertexArrays(1, &faceVao);
	if (lineVbo) qglDeleteBuffers(1, &lineVbo);
	if (lineVao) qglDeleteVertexArrays(1, &lineVao);
	if (selFaceVbo) qglDeleteBuffers(1, &selFaceVbo);
	if (selFaceVao) qglDeleteVertexArrays(1, &selFaceVao);
	if (selLineVbo) qglDeleteBuffers(1, &selLineVbo);
	if (selLineVao) qglDeleteVertexArrays(1, &selLineVao);
	if (lightmapTex) glDeleteTextures(1, &lightmapTex);
	faceVao = faceVbo = lineVao = lineVbo = lightmapTex = 0;
	selFaceVao = selFaceVbo = selLineVao = selLineVbo = 0;
	shader.Destroy();
	batches.clear();
	lightmapReady = bakePending = false;
	built = false;
}

void MapRenderer::Validate()
{
	if (!built || builtRevision != g_map.revision)
		Build();
}

float MapRenderer::BakeProgress() const
{
	return baker.Running() ? baker.Progress() : -1.0f;
}

/*
==================
SmoothPhongNormals

phong.cc: a face smooths with the faces of its entity that share a corner
with it, if they're within the angle. each corner's normal adds up theirs
weighted by area and the corner's angle
==================
*/
struct phongFace_t
{
	Winding	winding;
	dvec3	normal;
	double	area;
	int		bakeIndex;		// into bakeInput_t::faces, -1 without a lightmap
};

static void SmoothPhongNormals(const std::vector<phongFace_t> &faces, float angle, std::vector<bakeFace_t> &bakeFaces)
{
	auto cornerKey = [](const dvec3 &p)
	{
		return std::make_tuple(llround(p.x * 10), llround(p.y * 10), llround(p.z * 10));
	};
	std::map<std::tuple<long long, long long, long long>, std::vector<int>> facesAt;
	for (size_t i = 0; i < faces.size(); i++)
		for (const dvec3 &p : faces[i].winding)
			facesAt[cornerKey(p)].push_back((int)i);

	auto cornerAngle = [](const Winding &w, size_t j)
	{
		dvec3 a = w[(j + w.size() - 1) % w.size()] - w[j], b = w[(j + 1) % w.size()] - w[j];
		double la = length(a), lb = length(b);
		return (la > 0 && lb > 0) ? acos(std::clamp(dot(a, b) / (la * lb), -1.0, 1.0)) : 0.0;
	};

	double cosMax = cos(DEG2RAD(angle));
	for (const phongFace_t &f : faces)
	{
		if (f.bakeIndex < 0)
			continue;

		std::vector<dvec3> normals;
		for (const dvec3 &p : f.winding)
		{
			dvec3 sum;
			for (int g : facesAt[cornerKey(p)])
			{
				const phongFace_t &other = faces[g];
				if (dot(f.normal, other.normal) < cosMax)
					continue;
				for (size_t j = 0; j < other.winding.size(); j++)
					if (cornerKey(other.winding[j]) == cornerKey(p))
						sum = sum + other.normal * (other.area * cornerAngle(other.winding, j));
			}
			double len = length(sum);
			normals.push_back(len > 1e-9 ? sum / len : f.normal);
		}
		bakeFaces[f.bakeIndex].vertexNormals.swap(normals);
	}
}

/*
==================
FaceHidden

qbsp drops the faces between brushes of the same entity: just in front of a
hidden face is inside one of its other brushes
==================
*/
static bool FaceHidden(const Entity &e, const Brush &own, const BrushFace &f)
{
	dvec3 center;
	for (const dvec3 &p : f.winding)
		center = center + p;
	center = center / (double)f.winding.size() + f.plane.normal * 0.5;

	for (const Brush &b : e.brushes)
	{
		if (&b == &own || !b.valid)
			continue;
		bool inside = true;
		for (const BrushFace &g : b.faces)
		{
			if (g.planeValid && !g.winding.empty() && g.plane.Distance(center) > -0.01)
			{
				inside = false;
				break;
			}
		}
		if (inside)
			return true;
	}
	return false;
}

// what lights the game, the brushes func_detail and func_group fold into the world
static bool CastsShadows(const Entity &e)
{
	const std::string &cls = e.Classname();
	if (e.IsWorldspawn() || cls == "func_group")
		return true;
	if (cls.compare(0, 11, "func_detail") == 0 && cls != "func_detail_illusionary")
		return true;
	return KeyFloat(e, "_shadow", 0) != 0;
}

/*
==================
MapRenderer::Build

with "check brush geometry" off the loader never built the windings, so a
brush without any is built here first. faces are wound clockwise from the
front (qbsp's order), the triangles go out counterclockwise for GL.

the lightmap layout is worked out here too: each lit face's samples on the
16 unit grid of its texture coordinates, a rectangle of the atlas each. the
bake itself waits until the lighting preview is first drawn
==================
*/
void MapRenderer::Build()
{
	std::map<std::string, std::vector<mapVertex_t>> byTexture;
	std::map<std::string, unsigned> textureFlags;
	std::vector<mapVertex_t> boxes, lines;
	std::vector<BrushProblem> problems;
	const vec3 white(1, 1, 1);
	bakeInput_t bake;
	AtlasPacker packer;

	auto flagsFor = [&](const std::string &name)
	{
		auto it = textureFlags.find(name);
		if (it != textureFlags.end())
			return it->second;
		unsigned flags = Textures::ForName(name)->showflags;
		textureFlags[name] = flags;
		return flags;
	};

	for (Entity &e : g_map.entities)
	{
		vec3 edgeColor = e.IsWorldspawn() ? white : e.Color();	// netradiant: brush entities' wires in their color
		vec3 minlight = KeyColor(e, "_mincolor", white) * KeyFloat(e, "_minlight", 0);
		bool shadows = CastsShadows(e);
		float phongAngle = KeyFloat(e, "_phong_angle", 0);
		if (phongAngle == 0 && KeyFloat(e, "_phong", 0) > 0)
			phongAngle = 89;	// ericw's DEFAULT_PHONG_ANGLE
		std::vector<phongFace_t> phongFaces;

		for (Brush &b : e.brushes)
		{
			if (!b.valid && b.UsableFaceCount() == 0)
			{
				problems.clear();
				b.BuildGeometry(problems);
			}
			if (!b.valid)
				continue;

			// a shadow caster unless it's water or nothing but tool faces (clip, hint, skip)
			bool sky = false, liquid = false, toolsOnly = true;
			for (const BrushFace &f : b.faces)
			{
				unsigned flags = flagsFor(f.tex.name);
				sky |= (flags & BFL_SKY) != 0;
				liquid |= (flags & BFL_LIQUID) != 0;
				if (!(flags & (BFL_CLIP | BFL_HINT | BFL_SKIP | BFL_TRANS)))
					toolsOnly = false;
			}
			if (shadows && !liquid && !toolsOnly)
			{
				bakeBrush_t blocker;
				for (const BrushFace &f : b.faces)
					if (f.planeValid && !f.winding.empty())
						blocker.planes.push_back(f.plane);
				blocker.mins = b.mins;
				blocker.maxs = b.maxs;
				blocker.sky = sky;
				bake.brushes.push_back(std::move(blocker));
			}

			for (const BrushFace &f : b.faces)
			{
				size_t n = f.winding.size();
				if (n < 3)
					continue;

				dvec3 vecs[2];
				TextureVecs(f, vecs);
				vec3 normal = to_vec3(f.plane.normal);

				// the lightmap: no sky, liquids or tool faces, the BSP has none for them
				bool lit = !(flagsFor(f.tex.name) & (BFL_SKY | BFL_TRANS | BFL_SKIP));
				bakeFace_t bf;
				if (lit)
				{
					double smin = 1e30, smax = -1e30, tmin = 1e30, tmax = -1e30;
					for (const dvec3 &p : f.winding)
					{
						double s = dot(p, vecs[0]) + f.tex.shift[0];
						double t = dot(p, vecs[1]) + f.tex.shift[1];
						smin = std::min(smin, s);
						smax = std::max(smax, s);
						tmin = std::min(tmin, t);
						tmax = std::max(tmax, t);
					}
					bf.smin = (int)floor(smin / LIGHTMAP_SCALE);
					bf.tmin = (int)floor(tmin / LIGHTMAP_SCALE);
					bf.w = (int)ceil(smax / LIGHTMAP_SCALE) - bf.smin + 1;
					bf.h = (int)ceil(tmax / LIGHTMAP_SCALE) - bf.tmin + 1;

					int x, y;
					lit = packer.Place(bf.w + 2, bf.h + 2, x, y);
					if (lit)
					{
						bf.x = x + 1;
						bf.y = y + 1;
						bf.winding = f.winding;
						bf.plane = f.plane;
						bf.vecs[0] = vecs[0];
						bf.vecs[1] = vecs[1];
						bf.shift[0] = f.tex.shift[0];
						bf.shift[1] = f.tex.shift[1];
						bf.minlight = minlight;
					}
				}

				std::vector<mapVertex_t> &verts = byTexture[f.tex.name];
				auto vertex = [&](const dvec3 &p)
				{
					float s = (float)(dot(p, vecs[0]) + f.tex.shift[0]);
					float t = (float)(dot(p, vecs[1]) + f.tex.shift[1]);
					float lmu = 0, lmv = 0;
					if (lit)	// sample i sits on texel i's center
					{
						lmu = s / LIGHTMAP_SCALE - bf.smin + 0.5f + bf.x;
						lmv = t / LIGHTMAP_SCALE - bf.tmin + 0.5f + bf.y;
					}
					return MakeVertex(to_vec3(p), normal, s, t, minlight, lmu, lmv);
				};

				for (size_t i = 1; i + 1 < n; i++)
				{
					verts.push_back(vertex(f.winding[0]));
					verts.push_back(vertex(f.winding[i + 1]));
					verts.push_back(vertex(f.winding[i]));
				}
				for (size_t i = 0; i < n; i++)
				{
					lines.push_back(MakeVertex(to_vec3(f.winding[i]), vec3(), 0, 0, edgeColor));
					lines.push_back(MakeVertex(to_vec3(f.winding[(i + 1) % n]), vec3(), 0, 0, edgeColor));
				}

				if (phongAngle > 0 && !FaceHidden(e, b, f))
					phongFaces.push_back(phongFace_t{ f.winding, f.plane.normal, WindingArea(f.winding), lit ? (int)bake.faces.size() : -1 });
				if (lit)
					bake.faces.push_back(std::move(bf));
			}
		}
		if (!phongFaces.empty())
			SmoothPhongNormals(phongFaces, phongAngle, bake.faces);

		dvec3 mins, maxs;
		if (e.PointBounds(mins, maxs))
			AddBox(boxes, lines, mins, maxs, e.Color());
	}

	std::vector<mapVertex_t> faces;
	batches.clear();
	for (auto &kv : byTexture)
	{
		bool lightmapped = !(flagsFor(kv.first) & (BFL_SKY | BFL_TRANS | BFL_SKIP));
		batches.push_back(Batch{ kv.first, (int)faces.size(), (int)kv.second.size(), lightmapped });
		faces.insert(faces.end(), kv.second.begin(), kv.second.end());
	}
	entityFirst = (int)faces.size();
	entityCount = (int)boxes.size();
	faces.insert(faces.end(), boxes.begin(), boxes.end());
	lineCount = (int)lines.size();

	Upload(faceVbo, faces);
	Upload(lineVbo, lines);

	bake.width = ATLAS_WIDTH;
	bake.height = packer.Height();
	GatherLights(bake);

	baker.Cancel();
	pendingBake = std::move(bake);
	bakePending = true;
	lightmapReady = false;

	builtRevision = g_map.revision;
	built = true;
}

/*
==================
MapRenderer::UpdateLightmap

starts the bake the first time the preview wants it, uploads it once it's done
==================
*/
void MapRenderer::UpdateLightmap(bool wanted)
{
	if (wanted && bakePending)
	{
		baker.Start(std::move(pendingBake));
		bakePending = false;
	}

	std::vector<uint8_t> pixels;
	int w, h;
	if (!baker.TakeResult(pixels, w, h))
		return;

	glBindTexture(GL_TEXTURE_2D, lightmapTex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	glBindTexture(GL_TEXTURE_2D, 0);
	lightmapWidth = w;
	lightmapHeight = h;
	lightmapReady = true;
}

/*
==================
MapRenderer::DrawFaces

the textures are looked up by name every frame: a reload swaps them out from
under any pointer kept between frames
==================
*/
void MapRenderer::DrawFaces(DrawMode mode, bool translucent, bool lit)
{
	bool textured = mode == TEXTURED || mode == TEXTURED_SHADED || mode == LIGHTING;

	shader.Set("Shade", (mode == FLATSHADE || mode == TEXTURED_SHADED) ? 1 : 0);
	shader.Set("Tint", 1.0f, 1.0f, 1.0f, 1.0f);

	for (const Batch &b : batches)
	{
		Texture *tx = Textures::ForName(b.texture);
		if (((tx->showflags & BFL_TRANS) != 0) != translucent)
			continue;

		shader.Set("Alpha", translucent ? TRANSLUCENT_ALPHA : 1.0f);
		shader.Set("Lighting", (lit && b.lightmapped) ? 1 : 0);
		if (textured)
		{
			shader.Set("Mode", 2);
			shader.Set("TexSize", (float)std::max(tx->width, 1), (float)std::max(tx->height, 1));
			tx->glTex.Bind();
		}
		else
		{
			shader.Set("Mode", 1);
			shader.Set("FlatColor", tx->color[0] / 255.0f, tx->color[1] / 255.0f, tx->color[2] / 255.0f, 1.0f);
		}
		glDrawArrays(GL_TRIANGLES, b.first, b.count);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	shader.Set("Lighting", 0);
}

/*
==================
MapRenderer::Render

opaque faces and entity boxes pushed back a little so their edges draw clean
on top, then the edges, then the translucent faces without depth writes.
the lighting preview leaves the edges out, like hammer's; while it's still
baking the view is drawn shaded instead
==================
*/
void MapRenderer::Render(const Camera &camera, DrawMode mode)
{
	if (!shader.Valid())
		return;
	Validate();

	UpdateLightmap(mode == LIGHTING);
	bool lit = mode == LIGHTING && lightmapReady;
	if (mode == LIGHTING && !lit)
		mode = TEXTURED_SHADED;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glDisable(GL_BLEND);

	shader.Use();
	shader.Set("ModelViewProjection", camera.Matrix());
	shader.Set("Texture", 0);
	shader.Set("Lightmap", 1);
	shader.Set("TexSize", 1.0f, 1.0f);
	shader.Set("LightmapSize", (float)lightmapWidth, (float)lightmapHeight);
	shader.Set("Lighting", 0);

	if (lit)
	{
		qglActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, lightmapTex);
		qglActiveTexture(GL_TEXTURE0);
	}

	if (mode != WIREFRAME)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0f, 1.0f);

		qglBindVertexArray(faceVao);
		DrawFaces(mode, false, lit);

		shader.Set("Mode", 0);
		shader.Set("Shade", 1);
		shader.Set("Alpha", 1.0f);
		glDrawArrays(GL_TRIANGLES, entityFirst, entityCount);

		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (!lit)
	{
		const float *tint = (mode == WIREFRAME) ? s_wireTint : s_edgeTint;
		shader.Set("Mode", 0);
		shader.Set("Shade", 0);
		shader.Set("Alpha", 1.0f);
		shader.Set("Tint", tint[0], tint[1], tint[2], tint[3]);
		qglBindVertexArray(lineVao);
		glDrawArrays(GL_LINES, 0, lineCount);
	}

	if (mode != WIREFRAME)
	{
		glDepthMask(GL_FALSE);
		qglBindVertexArray(faceVao);
		DrawFaces(mode, true, lit);
		glDepthMask(GL_TRUE);
	}

	if (lit)
	{
		qglActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		qglActiveTexture(GL_TEXTURE0);
	}

	qglBindVertexArray(0);
	qglUseProgram(0);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

/*
========================================================================

	SELECTION

========================================================================
*/

static const vec3 s_selectedColor(1.0f, 0.15f, 0.1f);		// netradiant's and hammer's red

void MapRenderer::BuildSelection()
{
	std::vector<mapVertex_t> tris, lines;

	for (const selItem_t &item : g_selection.Items())
	{
		const Entity &e = g_map.entities[item.entity];
		if (item.brush < 0)
		{
			dvec3 mins, maxs;
			if (e.PointBounds(mins, maxs))
				AddBox(tris, lines, mins, maxs, s_selectedColor);
			continue;
		}
		const Brush &b = e.brushes[item.brush];
		for (const BrushFace &f : b.faces)
		{
			size_t n = f.winding.size();
			if (n < 3)
				continue;
			vec3 normal = to_vec3(f.plane.normal);
			for (size_t i = 1; i + 1 < n; i++)
			{
				tris.push_back(MakeVertex(to_vec3(f.winding[0]), normal, 0, 0, s_selectedColor));
				tris.push_back(MakeVertex(to_vec3(f.winding[i + 1]), normal, 0, 0, s_selectedColor));
				tris.push_back(MakeVertex(to_vec3(f.winding[i]), normal, 0, 0, s_selectedColor));
			}
			for (size_t i = 0; i < n; i++)
			{
				lines.push_back(MakeVertex(to_vec3(f.winding[i]), vec3(), 0, 0, s_selectedColor));
				lines.push_back(MakeVertex(to_vec3(f.winding[(i + 1) % n]), vec3(), 0, 0, s_selectedColor));
			}
		}
	}

	Upload(selFaceVbo, tris);
	Upload(selLineVbo, lines);
	selFaceCount = (int)tris.size();
	selLineCount = (int)lines.size();
	selRevision = g_selection.Revision();
	selMapRevision = g_map.revision;
}

void MapRenderer::RenderSelection(const Camera &camera, bool view3D)
{
	if (!shader.Valid())
		return;
	Validate();
	if (selRevision != g_selection.Revision() || selMapRevision != g_map.revision)
		BuildSelection();
	if (!selLineCount)
		return;

	shader.Use();
	shader.Set("ModelViewProjection", camera.Matrix());
	shader.Set("Mode", 0);
	shader.Set("Shade", 0);
	shader.Set("Lighting", 0);
	shader.Set("Alpha", 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (view3D)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);

		// the faces tinted, pulled a little toward the camera so they win over the map's
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.0f, -1.0f);
		glDepthFunc(GL_LEQUAL);
		shader.Set("Tint", 1.0f, 1.0f, 1.0f, 0.3f);
		qglBindVertexArray(selFaceVao);
		glDrawArrays(GL_TRIANGLES, 0, selFaceCount);
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_CULL_FACE);

		// trenchbroom's: the edges, then the hidden ones faint
		qglBindVertexArray(selLineVao);
		shader.Set("Tint", 1.0f, 1.0f, 1.0f, 1.0f);
		glDrawArrays(GL_LINES, 0, selLineCount);
		glDepthFunc(GL_GREATER);
		shader.Set("Tint", 1.0f, 1.0f, 1.0f, 0.3f);
		glDrawArrays(GL_LINES, 0, selLineCount);

		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
		shader.Set("Tint", 1.0f, 1.0f, 1.0f, 1.0f);
		qglBindVertexArray(selLineVao);
		glDrawArrays(GL_LINES, 0, selLineCount);
	}

	qglBindVertexArray(0);
	qglUseProgram(0);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
}
