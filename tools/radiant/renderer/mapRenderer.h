//==============================
//	mapRenderer.h
//
//	draws g_map in 3D: every brush face as triangles batched by texture, the
//	face edges as lines, point entities as boxes in their class color. the
//	draw modes are hammer's camera menu: wireframe, filled polygons (the
//	texture's average color), textured, shaded textured (trenchbroom's shading
//	by normal) and the lighting preview.
//
//	the lighting preview draws lightmaps baked by LightBaker the way
//	ericw-tools' light does it, shadows and all, packed in one atlas. the bake
//	starts the first time the preview is drawn after the map changes and runs
//	in the background; until it's done the view falls back to shaded textures.
//	sky, liquids and the tool textures get no lightmap, like in the BSP
//
//	texture coordinates are kept in texels and divided by the texture's size
//	when the batch is drawn, and batches hold texture names, not pointers, so
//	reloading the wads never needs a rebuild. the buffers are rebuilt when
//	g_map.revision changes
//==============================

#ifndef __MAP_RENDERER_H__
#define __MAP_RENDERER_H__

#include "renderer/shader.h"
#include "renderer/lightBaker.h"

#include <string>
#include <vector>

class Camera;

class MapRenderer
{
public:
	enum DrawMode { WIREFRAME, FLATSHADE, TEXTURED, TEXTURED_SHADED, LIGHTING, DRAWMODE_COUNT };

	static const char *DrawModeName(DrawMode mode);	// hammer's: "3D Wireframe"...

	MapRenderer() {}
	~MapRenderer();

	bool	Init();			// needs the GL context and GL_LoadFunctions
	void	Shutdown();

	void	Render(const Camera &camera, DrawMode mode);
	// g_selection over what Render drew, in red: in 3D tinted faces and edges (the
	// hidden ones faint), in 2D the edges on top of everything
	void	RenderSelection(const Camera &camera, bool view3D);

	// the lighting preview, as of the last Render
	inline int	LightCount() const		{ return lightCount; }
	inline int	SunCount() const		{ return sunCount; }
	float	BakeProgress() const;		// 0..1 while baking, -1 when it's not

private:
	struct Batch
	{
		std::string	texture;
		int			first, count;
		bool		lightmapped;
	};

	void	Validate();
	void	Build();
	void	GatherLights(bakeInput_t &bake);
	void	DrawFaces(DrawMode mode, bool translucent, bool lit);
	void	UpdateLightmap(bool wanted);
	void	BuildSelection();

	Shader			shader;
	unsigned int	faceVao = 0, faceVbo = 0;
	unsigned int	lineVao = 0, lineVbo = 0;
	std::vector<Batch>	batches;
	int				entityFirst = 0, entityCount = 0;	// point entity boxes, after the batches
	int				lineCount = 0;
	unsigned		builtRevision = 0;
	bool			built = false;

	// the selection, rebuilt when it or the map changes
	unsigned int	selFaceVao = 0, selFaceVbo = 0;
	unsigned int	selLineVao = 0, selLineVbo = 0;
	int				selFaceCount = 0, selLineCount = 0;
	unsigned		selRevision = 0, selMapRevision = 0;

	// lighting preview
	LightBaker		baker;
	bakeInput_t		pendingBake;		// built with the geometry, started when first needed
	bool			bakePending = false;
	unsigned int	lightmapTex = 0;
	int				lightmapWidth = 1, lightmapHeight = 1;
	bool			lightmapReady = false;
	int				lightCount = 0, sunCount = 0;
};

extern MapRenderer g_mapRenderer;

#endif
