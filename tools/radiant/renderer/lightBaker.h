//==============================
//	lightBaker.h
//
//	the lighting preview's lightmaps, baked on the CPU the way ericw-tools'
//	light does it (references/qbspserver/ericw-tools/light/ltface.cc), so the
//	editor shows what the game will:
//
//	-	one sample every 16 texture units, on the same grid the BSP lightmaps
//		use, read back with bilinear filtering like FTE does
//	-	GetLightValue's falloff formulas, _anglescale, the spotlight cone,
//		shadows from every brush that's solid in the BSP
//	-	_phong: normals smoothed across an entity's faces (phong.cc)
//	-	suns (_sunlight, _sunlight2's dome, "_sun" lights) only reach a sample
//		whose ray toward them hits a sky face
//	-	minlight, then _range, then _gamma, clamped to a byte
//
//	not done: bounce, dirt, _deviance, surface lights, light styles
//
//	the bake runs in its own threads over a copy of what it needs; the atlas
//	comes back as RGBA bytes, 128 being the texture as is (FTE's overbright)
//==============================

#ifndef __LIGHT_BAKER_H__
#define __LIGHT_BAKER_H__

#include "map/winding.h"

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

#define LIGHTMAP_SCALE	16		// texture units per lightmap sample, quake's

struct bakeLight_t
{
	vec3	origin;
	float	light;
	vec3	color;				// 0..1
	int		formula;			// "delay": ericw's light_formula_t
	float	atten;				// "wait" * worldspawn "_dist"
	float	falloff;			// "_falloff", linear only
	float	anglescale;
	float	reach;				// past it the light adds nothing visible
	bool	spot;
	vec3	spotDir;
	float	spotOuter, spotInner;	// cosines of the half angles
};

struct bakeSun_t
{
	vec3	toSun;				// unit, from the surface toward the sun
	float	light;
	vec3	color;				// 0..1
	float	anglescale;
};

struct bakeBrush_t
{
	std::vector<Plane>	planes;
	dvec3	mins, maxs;
	bool	sky;
};

struct bakeFace_t
{
	Winding	winding;
	Plane	plane;
	dvec3	vecs[2];			// texture axes, scale folded in
	double	shift[2];
	int		smin, tmin;			// the first sample, in LIGHTMAP_SCALE steps
	int		w, h;				// samples
	int		x, y;				// sample 0,0 in the atlas (a texel of padding around it)
	vec3	minlight;			// the entity's _minlight * _mincolor, light units
	std::vector<dvec3>	vertexNormals;	// _phong: one per winding point, empty without
};

struct bakeInput_t
{
	std::vector<bakeFace_t>		faces;
	std::vector<bakeBrush_t>	brushes;	// the ones that cast shadows
	std::vector<bakeLight_t>	lights;
	std::vector<bakeSun_t>		suns;
	vec3	minlight;			// worldspawn "light" * _minlight_color
	float	rangeScale = 0.5f;
	float	gamma = 1.0f;
	int		width = 0, height = 0;	// atlas
};

class LightBaker
{
public:
	LightBaker() {}
	~LightBaker();

	void	Start(bakeInput_t &&input);		// cancels a bake still running
	void	Cancel();
	bool	Running() const;
	float	Progress() const;				// 0..1
	// the atlas, once per finished bake
	bool	TakeResult(std::vector<uint8_t> &pixels, int &width, int &height);

private:
	struct node_t
	{
		dvec3	mins, maxs;
		int		left = -1, right = -1;	// children, -1 on a leaf
		int		first = 0, count = 0;	// leaf: range in order
	};

	void	Run();
	void	Worker();
	int		BuildNode(int first, int count);
	void	BakeFace(const bakeFace_t &f);
	vec3	LightSample(const dvec3 &point, const dvec3 &normal, const vec3 &faceMin) const;
	static dvec3	PhongNormal(const bakeFace_t &f, const dvec3 &point);
	bool	InSolid(const dvec3 &point) const;
	// the first brush the segment hits and where, -1 when it's clear
	int		Trace(const dvec3 &start, const dvec3 &dir, double maxDist, bool anyHit) const;

	bakeInput_t			in;
	std::vector<uint8_t>	pixels;
	std::vector<node_t>	nodes;
	std::vector<int>	order;

	std::thread			thread;
	std::atomic<bool>	cancel{ false }, finished{ false }, taken{ true };
	std::atomic<int>	nextFace{ 0 }, facesDone{ 0 };
};

#endif
