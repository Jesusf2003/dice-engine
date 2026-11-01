//==============================
//	lightBaker.cpp
//==============================

#include "renderer/lightBaker.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

#define SURFACE_OFFSET	1.0		// samples sit this far off their face, clear of it
#define SUN_DISTANCE	65536.0	// ericw's MAX_SKY_DIST is about this

// ericw-tools' light_formula_t
enum { LF_LINEAR, LF_INVERSE, LF_INVERSE2, LF_INFINITE, LF_LOCALMIN, LF_INVERSE2A };
#define LF_SCALE	128.0f

LightBaker::~LightBaker()
{
	Cancel();
}

void LightBaker::Start(bakeInput_t &&input)
{
	Cancel();

	in = std::move(input);
	pixels.assign((size_t)std::max(in.width, 1) * std::max(in.height, 1) * 4, 0);
	nextFace = 0;
	facesDone = 0;
	cancel = false;
	finished = false;
	taken = false;
	thread = std::thread(&LightBaker::Run, this);
}

void LightBaker::Cancel()
{
	cancel = true;
	if (thread.joinable())
		thread.join();
	taken = true;
}

bool LightBaker::Running() const
{
	return !taken && !finished;
}

float LightBaker::Progress() const
{
	return in.faces.empty() ? 1.0f : (float)facesDone / (float)in.faces.size();
}

bool LightBaker::TakeResult(std::vector<uint8_t> &out, int &width, int &height)
{
	if (taken || !finished)
		return false;
	if (thread.joinable())
		thread.join();
	out.swap(pixels);
	width = in.width;
	height = in.height;
	taken = true;
	return true;
}

/*
==================
LightBaker::Run

the BVH first, then every core but one on the faces
==================
*/
void LightBaker::Run()
{
	nodes.clear();
	order.resize(in.brushes.size());
	for (size_t i = 0; i < order.size(); i++)
		order[i] = (int)i;
	if (!order.empty())
		BuildNode(0, (int)order.size());

	unsigned n = std::max(1u, std::thread::hardware_concurrency() - 1);
	std::vector<std::thread> workers;
	for (unsigned i = 0; i < n; i++)
		workers.emplace_back(&LightBaker::Worker, this);
	for (std::thread &w : workers)
		w.join();

	finished = !cancel;
}

void LightBaker::Worker()
{
	for (;;)
	{
		if (cancel)
			return;
		int i = nextFace++;
		if (i >= (int)in.faces.size())
			return;
		BakeFace(in.faces[i]);
		facesDone++;
	}
}

/*
==================
LightBaker::BuildNode

median split along the widest axis of the brush centers, 4 brushes a leaf
==================
*/
int LightBaker::BuildNode(int first, int count)
{
	int index = (int)nodes.size();
	nodes.emplace_back();

	dvec3 mins(DBL_MAX, DBL_MAX, DBL_MAX), maxs(-DBL_MAX, -DBL_MAX, -DBL_MAX);
	dvec3 cmins = mins, cmaxs = maxs;
	for (int i = first; i < first + count; i++)
	{
		const bakeBrush_t &b = in.brushes[order[i]];
		for (int k = 0; k < 3; k++)
		{
			mins[k] = std::min(mins[k], b.mins[k]);
			maxs[k] = std::max(maxs[k], b.maxs[k]);
			double c = (b.mins[k] + b.maxs[k]) / 2;
			cmins[k] = std::min(cmins[k], c);
			cmaxs[k] = std::max(cmaxs[k], c);
		}
	}
	nodes[index].mins = mins;
	nodes[index].maxs = maxs;

	if (count <= 4)
	{
		nodes[index].first = first;
		nodes[index].count = count;
		return index;
	}

	int axis = abs_max_axis(cmaxs - cmins);
	int half = count / 2;
	std::nth_element(order.begin() + first, order.begin() + first + half, order.begin() + first + count, [&](int a, int b)
	{
		return in.brushes[a].mins[axis] + in.brushes[a].maxs[axis] < in.brushes[b].mins[axis] + in.brushes[b].maxs[axis];
	});

	int left = BuildNode(first, half);
	int right = BuildNode(first + half, count - half);
	nodes[index].left = left;
	nodes[index].right = right;
	return index;
}

static bool RayBox(const dvec3 &start, const dvec3 &invDir, double maxDist, const dvec3 &mins, const dvec3 &maxs)
{
	double enter = 0, leave = maxDist;
	for (int k = 0; k < 3; k++)
	{
		double t0 = (mins[k] - start[k]) * invDir[k];
		double t1 = (maxs[k] - start[k]) * invDir[k];
		if (t0 > t1)
			std::swap(t0, t1);
		enter = std::max(enter, t0);
		leave = std::min(leave, t1);
		if (enter > leave)
			return false;
	}
	return true;
}

// where along the segment it enters the convex brush; 0 when it starts inside
static bool RayBrush(const bakeBrush_t &b, const dvec3 &start, const dvec3 &dir, double maxDist, double &dist)
{
	double enter = 0, leave = maxDist;

	for (const Plane &p : b.planes)
	{
		double d = p.Distance(start);
		double denom = dot(p.normal, dir);
		if (fabs(denom) < 1e-12)
		{
			if (d > 0)
				return false;
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

	// only grazing it: a sample on the edge of a face sits on the surface of
	// the brush next to it (the floor under a wall), its rays leave that brush
	// straight away. ericw nudges such samples out of the solid instead
	if (leave - enter < 0.1)
		return false;

	dist = enter;
	return true;
}

bool LightBaker::InSolid(const dvec3 &point) const
{
	if (nodes.empty())
		return false;

	int stack[64];
	int sp = 0;
	stack[sp++] = 0;
	while (sp)
	{
		const node_t &n = nodes[stack[--sp]];
		if (point.x < n.mins.x || point.y < n.mins.y || point.z < n.mins.z ||
			point.x > n.maxs.x || point.y > n.maxs.y || point.z > n.maxs.z)
			continue;

		if (n.left < 0)
		{
			for (int i = n.first; i < n.first + n.count; i++)
			{
				bool inside = true;
				for (const Plane &p : in.brushes[order[i]].planes)
				{
					if (p.Distance(point) > -0.01)
					{
						inside = false;
						break;
					}
				}
				if (inside)
					return true;
			}
		}
		else if (sp < 62)
		{
			stack[sp++] = n.left;
			stack[sp++] = n.right;
		}
	}
	return false;
}

/*
==================
LightBaker::Trace

anyHit stops at the first brush found, for shadow rays; otherwise the nearest
==================
*/
int LightBaker::Trace(const dvec3 &start, const dvec3 &dir, double maxDist, bool anyHit) const
{
	if (nodes.empty())
		return -1;

	dvec3 invDir;
	for (int k = 0; k < 3; k++)
		invDir[k] = fabs(dir[k]) > 1e-12 ? 1.0 / dir[k] : (dir[k] < 0 ? -1e30 : 1e30);

	int best = -1;
	int stack[64];
	int sp = 0;
	stack[sp++] = 0;

	while (sp)
	{
		const node_t &n = nodes[stack[--sp]];
		if (!RayBox(start, invDir, maxDist, n.mins, n.maxs))
			continue;

		if (n.left < 0)
		{
			for (int i = n.first; i < n.first + n.count; i++)
			{
				double d;
				if (RayBrush(in.brushes[order[i]], start, dir, maxDist, d))
				{
					best = order[i];
					if (anyHit)
						return best;
					maxDist = d;
				}
			}
		}
		else if (sp < 62)
		{
			stack[sp++] = n.left;
			stack[sp++] = n.right;
		}
	}
	return best;
}

/*
==================
GetLightValue

ltface.cc's, the distance part
==================
*/
static float GetLightValue(const bakeLight_t &l, float dist)
{
	if (l.formula == LF_INFINITE || l.formula == LF_LOCALMIN)
		return l.light;

	if (l.falloff > 0 && l.formula == LF_LINEAR)
		return l.falloff > dist ? l.light * (1.0f - dist / l.falloff) : 0.0f;

	float value = l.atten * dist;
	switch (l.formula)
	{
	case LF_INVERSE:	return l.light / (value / LF_SCALE);
	case LF_INVERSE2A:	value += LF_SCALE;	// fall through
	case LF_INVERSE2:	return l.light / ((value * value) / (LF_SCALE * LF_SCALE));
	default:
		if (l.light > 0)
			return std::max(l.light - value, 0.0f);
		return std::min(l.light + value, 0.0f);
	}
}

/*
==================
LightBaker::LightSample

ltface.cc's LightFace_Entity and LightFace_Sky for one sample, then
LightFace_Min and LightPoint_ScaleAndClamp. light units, 0..255
==================
*/
vec3 LightBaker::LightSample(const dvec3 &point, const dvec3 &normal, const vec3 &faceMin) const
{
	vec3 color;
	vec3 n = to_vec3(normal);
	vec3 p = to_vec3(point);

	for (const bakeLight_t &l : in.lights)
	{
		vec3 d = l.origin - p;
		float dist = length(d);
		if (dist > l.reach)
			continue;

		vec3 toLight = dist > 0.001f ? d / dist : n;
		float angle = dot(toLight, n);
		if (angle < 0)
			continue;		// behind the face: nothing, ericw issue 181

		float value = GetLightValue(l, dist) * ((1.0f - l.anglescale) + l.anglescale * angle);
		if (l.spot)
		{
			float c = dot(l.spotDir, -toLight);
			if (c < l.spotOuter)
				continue;
			if (c < l.spotInner)
				value *= (c - l.spotOuter) / (l.spotInner - l.spotOuter);
		}
		if (fabsf(value) < 0.001f)
			continue;

		if (Trace(point, to_dvec3(toLight), std::max(dist - 0.5, 0.0), true) >= 0)
			continue;	// in shadow

		color += l.color * value;
	}

	for (const bakeSun_t &s : in.suns)
	{
		float angle = dot(s.toSun, n);
		if (angle < -0.01f)
			continue;
		angle = std::max(angle, 0.0f);

		int hit = Trace(point, to_dvec3(s.toSun), SUN_DISTANCE, false);
		if (hit < 0 || !in.brushes[hit].sky)
			continue;	// suns only come in through the sky

		color += s.color * (s.light * ((1.0f - s.anglescale) + s.anglescale * angle));
	}

	for (int k = 0; k < 3; k++)
	{
		color[k] = std::max(color[k], std::max(in.minlight[k], faceMin[k]));
		color[k] = std::max(color[k], 0.0f) * in.rangeScale;
		if (in.gamma != 1.0f)
			color[k] = powf(color[k] / 255.0f, 1.0f / in.gamma) * 255.0f;
		color[k] = std::min(color[k], 255.0f);
	}
	return color;
}

/*
==================
SamplePoint

the point on the face's plane at texture coordinates s, t: ltface.cc's
CalcPoints, the texture matrix with the plane as its third row solved
==================
*/
static dvec3 SamplePoint(const bakeFace_t &f, double s, double t)
{
	const dvec3 &a = f.vecs[0], &b = f.vecs[1], &c = f.plane.normal;
	dvec3 bc = cross(b, c), ca = cross(c, a), ab = cross(a, b);
	double det = dot(a, bc);
	double r0 = s - f.shift[0], r1 = t - f.shift[1], r2 = f.plane.dist;
	return (bc * r0 + ca * r1 + ab * r2) / det;
}

/*
==================
ClampToWinding

samples on the edge of the grid can fall outside the face. ericw leaves them
where they are unless that's inside a solid (a neighbouring wall), then moves
them back onto the face; here to the nearest point of the polygon, a little inside
==================
*/
static dvec3 ClampToWinding(const Winding &w, const dvec3 &normal, const dvec3 &p)
{
	size_t n = w.size();
	dvec3 center;
	for (const dvec3 &v : w)
		center = center + v;
	center = center / (double)n;

	bool inside = true;
	for (size_t i = 0; i < n && inside; i++)
	{
		dvec3 edgeNormal = cross(w[(i + 1) % n] - w[i], normal);
		if (dot(edgeNormal, center - w[i]) > 0)
			edgeNormal = -edgeNormal;		// pointing out
		if (dot(edgeNormal, p - w[i]) > 0)
			inside = false;
	}
	if (inside)
		return p;

	dvec3 best = p;
	double bestDist = DBL_MAX;
	for (size_t i = 0; i < n; i++)
	{
		dvec3 a = w[i], e = w[(i + 1) % n] - a;
		double len2 = dot(e, e);
		double t = len2 > 0 ? std::clamp(dot(p - a, e) / len2, 0.0, 1.0) : 0.0;
		dvec3 q = a + e * t;
		double d = length(p - q);
		if (d < bestDist)
		{
			bestDist = d;
			best = q;
		}
	}
	dvec3 in = center - best;
	double len = length(in);
	return len > 0.25 ? best + in * (0.25 / len) : center;
}

/*
==================
LightBaker::PhongNormal

the smoothed vertex normals across the fan triangle the point falls in
==================
*/
dvec3 LightBaker::PhongNormal(const bakeFace_t &f, const dvec3 &point)
{
	const Winding &w = f.winding;
	const dvec3 &n = f.plane.normal;
	size_t best = 1;
	double bestMin = -DBL_MAX, bary[3] = { 1, 0, 0 };

	for (size_t i = 1; i + 1 < w.size(); i++)
	{
		const dvec3 &a = w[0], &b = w[i], &c = w[i + 1];
		double area = dot(cross(b - a, c - a), n);
		if (fabs(area) < 1e-9)
			continue;
		double u = dot(cross(c - b, point - b), n) / area;	// weight of a
		double v = dot(cross(a - c, point - c), n) / area;	// weight of b
		double t = 1 - u - v;								// weight of c
		double m = std::min(u, std::min(v, t));
		if (m > bestMin)
		{
			bestMin = m;
			best = i;
			bary[0] = u;
			bary[1] = v;
			bary[2] = t;
		}
		if (m >= 0)
			break;
	}

	dvec3 r = f.vertexNormals[0] * bary[0] + f.vertexNormals[best] * bary[1] + f.vertexNormals[best + 1] * bary[2];
	double len = length(r);
	return len > 1e-9 ? r / len : n;
}

void LightBaker::BakeFace(const bakeFace_t &f)
{
	const dvec3 &a = f.vecs[0], &b = f.vecs[1];
	if (fabs(dot(a, cross(b, f.plane.normal))) < 1e-9)
		return;		// texture axes along the face, no grid on it

	auto put = [&](int x, int y, const vec3 &c)
	{
		uint8_t *px = &pixels[((size_t)y * in.width + x) * 4];
		px[0] = (uint8_t)(c.x + 0.5f);
		px[1] = (uint8_t)(c.y + 0.5f);
		px[2] = (uint8_t)(c.z + 0.5f);
		px[3] = 255;
	};

	for (int j = 0; j < f.h; j++)
	{
		if (cancel)
			return;
		for (int i = 0; i < f.w; i++)
		{
			dvec3 p = SamplePoint(f, (double)(f.smin + i) * LIGHTMAP_SCALE, (double)(f.tmin + j) * LIGHTMAP_SCALE);
			if (InSolid(p + f.plane.normal * SURFACE_OFFSET))
				p = ClampToWinding(f.winding, f.plane.normal, p);
			dvec3 normal = f.vertexNormals.size() == f.winding.size() ? PhongNormal(f, p) : f.plane.normal;
			p = p + f.plane.normal * SURFACE_OFFSET;
			put(f.x + i, f.y + j, LightSample(p, normal, f.minlight));
		}
	}

	// a texel of padding all round, copies of the edge, so filtering doesn't
	// pull in the face next to it in the atlas
	auto copy = [&](int dx, int dy, int sx, int sy)
	{
		uint8_t *d = &pixels[((size_t)dy * in.width + dx) * 4];
		const uint8_t *s = &pixels[((size_t)sy * in.width + sx) * 4];
		d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
	};
	for (int i = 0; i < f.w; i++)
	{
		copy(f.x + i, f.y - 1, f.x + i, f.y);
		copy(f.x + i, f.y + f.h, f.x + i, f.y + f.h - 1);
	}
	for (int j = -1; j <= f.h; j++)
	{
		copy(f.x - 1, f.y + j, f.x, f.y + std::clamp(j, 0, f.h - 1));
		copy(f.x + f.w, f.y + j, f.x + f.w - 1, f.y + std::clamp(j, 0, f.h - 1));
	}
}
